package uz.otves.feature.editor

import androidx.compose.runtime.Immutable
import androidx.lifecycle.ViewModel
import androidx.lifecycle.viewModelScope
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.asStateFlow
import kotlinx.coroutines.flow.update
import kotlinx.coroutines.launch
import uz.otves.core.model.Currency
import uz.otves.core.model.ElementId
import uz.otves.core.model.Mm
import uz.otves.core.model.Money
import uz.otves.core.model.mm
import uz.otves.data.catalog.Prices
import uz.otves.data.project.JobSession
import uz.otves.data.project.JobStore
import uz.otves.data.project.Outcome
import uz.otves.engine.Aimed
import uz.otves.engine.Catalog
import uz.otves.engine.CommandResult
import uz.otves.engine.Condition
import uz.otves.engine.Conditions
import uz.otves.engine.Drawing
import uz.otves.engine.DrawnText
import uz.otves.engine.Engine
import uz.otves.engine.EngineCommand
import uz.otves.engine.EngineError
import uz.otves.engine.Footing
import uz.otves.engine.Hand
import uz.otves.engine.Hinge
import uz.otves.engine.Issue
import uz.otves.engine.IssueKind
import uz.otves.engine.LayerSpec
import uz.otves.engine.Level
import uz.otves.engine.LocationLine
import uz.otves.engine.MasonryUnitKind
import uz.otves.engine.MasonryUnitOption
import uz.otves.engine.Opening
import uz.otves.engine.OpeningKind
import uz.otves.engine.PointMm
import uz.otves.engine.QuantityLine
import uz.otves.engine.Room
import uz.otves.engine.RoomKind
import uz.otves.engine.Space
import uz.otves.engine.Swing
import uz.otves.engine.Takeoff
import uz.otves.engine.TextKind
import uz.otves.engine.Turn
import uz.otves.engine.UnitSize
import uz.otves.engine.Wall
import uz.otves.engine.WallLayer
import uz.otves.engine.WallOutline
import uz.otves.engine.WallRole
import uz.otves.engine.WallTypeTemplate
import kotlin.math.abs

/** What a touch on the plan does. */
internal enum class Tool { SELECT, PENCIL, WALL, WINDOW, DOOR }

/** A window or door size a builder picks from. */
internal data class OpeningSize(
    val width: Mm,
    val height: Mm,
)

internal val WindowSizes: List<OpeningSize> =
    listOf(OpeningSize(1200.mm, 1500.mm), OpeningSize(1500.mm, 1500.mm), OpeningSize(1800.mm, 1500.mm))

internal val DoorSizes: List<OpeningSize> =
    listOf(OpeningSize(800.mm, 2100.mm), OpeningSize(900.mm, 2100.mm), OpeningSize(1200.mm, 2100.mm))

/** The sizes offered for an opening of this kind; an empty opening is sized like a door. */
internal fun sizesFor(kind: OpeningKind): List<OpeningSize> = if (kind == OpeningKind.WINDOW) WindowSizes else DoorSizes

/** What the select tool picked, with its current value. */
internal sealed interface Selected {
    /** The element; null for a space no room names. */
    val id: ElementId?

    data class WallPart(
        override val id: ElementId,
        val wall: Wall,
    ) : Selected

    /** An opening, and how many openings of the job are of its kind and size: they share its mark. */
    data class OpeningPart(
        override val id: ElementId,
        val opening: Opening,
        val alike: Int = 1,
    ) : Selected

    /**
     * A space the walls close (docs/adr/0015), picked at [at]: the floor under that point, with
     * the room that names it, if one does. It stays selected while a floor lies under the point.
     */
    data class SpacePart(
        val at: PointMm,
        val space: Space,
    ) : Selected {
        override val id: ElementId?
            get() = space.room
    }
}

/**
 * A wall type of the job as the screen shows it (docs/adr/0014): what it is for and its layers
 * from the outer face in.
 */
internal data class WallKind(
    val id: ElementId,
    val role: WallRole,
    val layers: List<Pair<ElementId, WallLayer>>,
) {
    /** The wall's thickness: its layers one after another. */
    val thickness: Mm
        get() = Mm(layers.sumOf { it.second.thickness.value })

    /** Whether the catalog's [template] describes this very make-up. */
    fun matches(template: WallTypeTemplate): Boolean =
        role == template.role && layers.map { LayerSpec(it.second.function, it.second.thickness, it.second.unit) } == template.layers
}

/** A wall's length, written along it on the plan. */
internal data class Dimension(
    val id: ElementId,
    val wall: Wall,
    val length: Mm,
)

/** One storey of the job and where its walls lie, for a sheet that shows them all. */
internal data class Storey(
    val level: ElementId,
    val outlines: List<WallOutline>,
)

/**
 * A wall being drawn: it starts at [start] and, while the finger is down or a length is typed,
 * would end where the engine says. After each wall the next one starts where it ended.
 */
internal data class Draft(
    val start: PointMm,
    val to: Aimed? = null,
)

/** How many minor units make one unit of the currency: 100 tiyin in a sum. */
internal fun Currency.minorPerUnit(): Long = (1..minorDigits).fold(1L) { units, _ -> units * DECIMAL }

private const val DECIMAL = 10

/**
 * A wall being entered from a tape or laser reading: its length in millimetres as typed so far,
 * and which way it turns from the wall before it. The first wall of a run goes straight to the
 * right of the plan.
 */
internal data class Measure(
    val digits: String = "",
    val turn: Turn = Turn.STRAIGHT,
)

/**
 * Sizes in millimetres being typed, as digits so far, and which of them the keys go to: a masonry
 * unit the catalog does not list (length, width, height as laid), or a strip footing's section
 * (width, depth).
 */
internal data class SizeEntry(
    val digits: List<String> = listOf("", "", ""),
    val active: Int = 0,
    /** The sizes are a strip footing's width and depth, not a masonry unit's. */
    val footing: Boolean = false,
) {
    /** The footing's section once both sizes are typed. */
    val section: Footing?
        get() {
            val sizes = digits.map { it.toLongOrNull() ?: 0 }
            return if (footing && sizes.all { it > 0 }) Footing(Mm(sizes[0]), Mm(sizes[1])) else null
        }

    /** The size once all three are typed. */
    val size: UnitSize?
        get() {
            if (footing) return null
            val (length, width, height) = digits.map { it.toLongOrNull() ?: 0 }
            return if (length > 0 && width > 0 && height > 0) UnitSize(Mm(length), Mm(width), Mm(height)) else null
        }
}

/** The price of one unit of a take-off item being typed, in whole units of the currency. */
internal data class PriceEntry(
    val line: QuantityLine,
    val digits: String = "",
)

/**
 * A wall's exact length being typed: the digits so far, in millimetres. While the sketched walls of
 * a storey get their lengths one after another, this is wall [step] of [of]; no digits then keep
 * the length the sketch gave. With a [size], the digits are for that size of the plan instead
 * (docs/adr/0016) and [wall] is what it measures. A clear size lies between two walls; when
 * [swapped], the wall at its near end moves instead of the one at its far end.
 */
internal data class LengthEntry(
    val wall: ElementId,
    val was: Mm,
    val digits: String = "",
    val step: Int = 0,
    val of: Int = 0,
    val size: DrawnText? = null,
    val swapped: Boolean = false,
) {
    val refining: Boolean
        get() = of > 0
}

/**
 * A wall to put across the selected room, being typed (docs/adr/0016): how far it stands from
 * the wall that bounds the room on the [from] hand of [at], in millimetres. The engine
 * [refused] the distance typed last; the keys stay open for another.
 */
internal data class PartitionEntry(
    val at: PointMm,
    val from: Hand = Hand.TOP,
    val digits: String = "",
    val refused: Boolean = false,
)

/**
 * The selected opening's sizes being typed, in millimetres: its width, its height and, for one
 * that is no door, how high over the floor it starts. A size with no digits keeps what it [was].
 */
internal data class OpeningEntry(
    val opening: ElementId,
    val was: List<Mm>,
    val digits: List<String> = was.map { "" },
    val active: Int = 0,
) {
    val sizes: List<Mm>
        get() = digits.mapIndexed { index, typed -> typed.toLongOrNull()?.let(::Mm) ?: was[index] }
}

/** The thickness of a wall type's layer being typed, in millimetres; it [was] this thick. */
internal data class LayerEntry(
    val layer: ElementId,
    val was: Mm,
    val digits: String = "",
)

/**
 * A room to build from its clear size (docs/adr/0016): the sizes being typed in millimetres,
 * across the plan and then up it, which of them the keys go to, and which way the room lies
 * from the corner the builder touched [at]. The engine [refused] the room typed last; the keys
 * stay open for another.
 */
internal data class RoomEntry(
    val at: PointMm,
    val reach: Mm,
    val digits: List<String> = listOf("", ""),
    val active: Int = 0,
    val right: Boolean = true,
    val up: Boolean = true,
    val refused: Boolean = false,
) {
    /** The width and the depth once both are typed. */
    val sizes: Pair<Mm, Mm>?
        get() {
            val typed = digits.map { it.toLongOrNull() ?: 0 }
            return if (typed.all { it > 0 }) Mm(typed[0]) to Mm(typed[1]) else null
        }
}

/**
 * What the plan shows of the job: its storeys and, of the one the builder works on, the drawing,
 * the walls, what to fix and what is selected. All of it is what the engine last said.
 */
@Immutable
internal data class PlanState(
    /** Storeys from the bottom up; the user works on [level]. */
    val levels: List<ElementId> = emptyList(),
    val level: ElementId? = null,
    /** The current storey as the engine drew it (docs/adr/0013), over the storey below. */
    val drawing: Drawing = Drawing(),
    /** Where the current storey's walls lie, to show them all. */
    val outlines: List<WallOutline> = emptyList(),
    /** Every storey from the bottom up, for a sheet of the whole job. */
    val storeys: List<Storey> = emptyList(),
    /** What to fix on the current storey. */
    val issues: List<Issue> = emptyList(),
    val dimensions: List<Dimension> = emptyList(),
    /** The job's wall types, the oldest first. */
    val kinds: List<WallKind> = emptyList(),
    val selected: Selected? = null,
    val canUndo: Boolean = false,
    val canRedo: Boolean = false,
    /** Why the engine refused the last command, until the next one. */
    val refused: EngineError? = null,
) {
    /** Walls of the shown storey read from a sketch, whose lengths nobody has given yet. */
    val rough: List<ElementId>
        get() = dimensions.filterNot { it.wall.exact }.map { it.id }

    /** Walls of the shown storey that have no material yet. */
    val unchosen: Set<ElementId>
        get() = issues.filter { it.kind == IssueKind.MATERIAL_NOT_CHOSEN }.mapNotNull { it.element }.toSet()
}

/** What the builder draws with: the tool, what the walls and openings drawn next are like, and the run of walls being drawn. */
@Immutable
internal data class ToolState(
    val tool: Tool = Tool.WALL,
    /** The wall being drawn, from its first point on. */
    val draft: Draft? = null,
    /** Keeps new walls along the plan's axes. */
    val ortho: Boolean = true,
    /** The wall drawn last, whose length the user may now type. */
    val drawn: ElementId? = null,
    /** The wall tool waits for a touch where a room's corner goes. */
    val roomPlacing: Boolean = false,
    /** What new walls are laid from; null until the builder chooses. */
    val material: UnitSize? = null,
    /** What new walls are built as; null for walls with no type, laid from [material]. */
    val kind: ElementId? = null,
    val thickness: Mm = 0.mm,
    val windowSize: OpeningSize = WindowSizes[1],
    val doorSize: OpeningSize = DoorSizes[1],
    /** Walls are being entered by their measured lengths. */
    val measure: Measure? = null,
    /** Whether the measured run can close back onto its first point. */
    val canCloseRun: Boolean = false,
)

/** What is open over the plan: a list to choose from, or the keys with the number being typed. */
@Immutable
internal data class PanelState(
    val entry: LengthEntry? = null,
    val partition: PartitionEntry? = null,
    val room: RoomEntry? = null,
    val layerEntry: LayerEntry? = null,
    val openingEntry: OpeningEntry? = null,
    val priceEntry: PriceEntry? = null,
    /** A unit size of the builder's own is being typed. */
    val sizeEntry: SizeEntry? = null,
    val takeoffOpen: Boolean = false,
    /** The material list is open, for the selected wall or else for new walls. */
    val materialOpen: Boolean = false,
    /** The wall types are open, for the selected wall or else for new walls. */
    val kindsOpen: Boolean = false,
    /** The wall type whose layers are open to be changed (docs/adr/0014). */
    val kindEdited: ElementId? = null,
    /** The names a room can have are open, for the selected space. */
    val roomKindsOpen: Boolean = false,
    /** The conditions are open in place of the take-off. */
    val conditionsOpen: Boolean = false,
) {
    /** The keys are open for one of these. */
    val typing: Boolean
        get() =
            entry != null || partition != null || room != null || layerEntry != null || openingEntry != null || sizeEntry != null ||
                priceEntry != null
}

/** What the job's materials come to, and what they are counted under. */
@Immutable
internal data class EstimateState(
    val takeoff: List<QuantityLine> = emptyList(),
    val takeoffFailed: Boolean = false,
    /** The builder's prices by take-off item, for one unit of each. */
    val prices: Map<String, Money> = emptyMap(),
    /** What the job is estimated under; unset values follow the norms. */
    val conditions: Conditions = Conditions(),
    /** What the shown storey asks for: its ring beam's height, zero for none, and junction meshes. */
    val ringBeam: Mm = 0.mm,
    val footing: Footing? = null,
    val junctionMeshes: Boolean = false,
)

/** Everything the editor's screen shows, in four parts that change for different reasons. */
@Immutable
internal data class EditorUiState(
    val ready: Boolean = false,
    /** The job's name, as the user gave it. */
    val name: String = "",
    /** What the builder chooses from. */
    val catalog: Catalog = Catalog(),
    val plan: PlanState = PlanState(),
    val tools: ToolState = ToolState(),
    val panel: PanelState = PanelState(),
    val estimate: EstimateState = EstimateState(),
) {
    /** A number pad is open: until the number is entered, touches on the plan only move it. */
    val typing: Boolean
        get() = panel.typing || tools.measure != null

    /** How thick the walls drawn next are: as their type, or else as chosen. */
    val drawnThickness: Mm
        get() = plan.kinds.find { it.id == tools.kind }?.thickness ?: tools.thickness

    /** Walls of the shown storey read from a sketch, whose lengths nobody has given yet. */
    val rough: List<ElementId>
        get() = plan.rough

    /** Walls of the shown storey that have no material yet. */
    val unchosen: Set<ElementId>
        get() = plan.unchosen

    /**
     * The thicknesses a wall of [unit] is built in: the catalog's, or for a size of the builder's
     * own the unit laid along and across the wall; before a material is chosen, the first one's.
     */
    fun thicknessesOf(unit: UnitSize?): List<Mm> =
        catalog.units.find { it.size == unit }?.thicknesses
            ?: unit?.let { listOf(it.width, it.length).distinct().sortedBy { side -> side.value } }
            ?: catalog.units
                .firstOrNull()
                ?.thicknesses
                .orEmpty()
}

/**
 * The plan of one job, one storey at a time. The engine owns the model and answers every question
 * of geometry (docs/adr/0011); this keeps what the screen draws, the values of what can be
 * selected and the conversation with the builder: the tool, the run of walls being drawn, the
 * digits typed so far. Everything shown comes from engine responses. It speaks to the engine
 * through a [JobSession], which writes every response to the job file and mirrors the model, one
 * step at a time. The view model owns [job] and closes it when cleared.
 */
internal class EditorViewModel(
    engine: Engine,
    loadContent: suspend () -> EngineCommand.LoadContent,
    loadCatalog: suspend () -> Catalog,
    job: JobStore,
    private val prices: Prices,
) : ViewModel() {
    private val state = MutableStateFlow(EditorUiState())
    val uiState: StateFlow<EditorUiState> = state.asStateFlow()

    private val session = JobSession(engine, job)
    private val model get() = session.model

    // How big the plan's label text is at the zoom the canvas told last; the engine places the
    // labels by it.
    private var label = LabelSize(height = 400.mm, digit = 200.mm)

    // The walls drawn in a row: where the first one started and the walls built since. The next
    // wall starts where the last one ends, wherever an edit has since put that end, so undo and
    // typed lengths move the run without bookkeeping here.
    private data class Run(
        val first: PointMm,
        val built: List<ElementId> = emptyList(),
    )

    private var run: Run? = null

    // Whether the finger now on the plan put down the run's first point.
    private var touchStartedRun = false

    // Counts touches of the wall tool, so one that waited for the engine knows the finger moved on.
    private var aims = 0

    // The sketched walls already asked for a length in this pass; one the engine refused is not asked twice.
    private val refined = mutableSetOf<ElementId>()

    init {
        viewModelScope.launch {
            prices.all.collect { all ->
                updateEstimate { it.copy(prices = all) }
            }
        }
        act {
            apply(session.open(loadContent(), Ground))
            show(model.levelsFromBottom().firstOrNull())
            val catalog = loadCatalog()
            state.update { current ->
                // New walls go on as the job's walls are: the material and thickness of one of them.
                val like = model.walls.values.lastOrNull { it.unit != null } ?: model.walls.values.lastOrNull()
                val thickness =
                    like?.thickness ?: catalog.units
                        .firstOrNull()
                        ?.thicknesses
                        ?.let(::usual) ?: current.tools.thickness
                current.copy(
                    ready = model.levels.isNotEmpty(),
                    name = session.name,
                    catalog = catalog,
                    tools =
                        current.tools.copy(
                            material = like?.unit,
                            thickness = thickness,
// A job built from wall types goes on with the one used last.
                            kind =
                                model.walls.values
                                    .lastOrNull()
                                    ?.type,
                        ),
                )
            }
        }
    }

    override fun onCleared() = session.close()

    /**
     * A tap on the plan. The select tool picks what is under it; with the wall tool it is a point
     * of the wall, as [aim] and [commit]; a window or door goes into the wall the user tapped.
     * [reach] is how far from the finger, in plan millimetres, something still counts as touched.
     */
    fun tap(
        at: PointMm,
        reach: Mm,
    ) {
        val current = state.value
        val onLevel = current.plan.level ?: return
        // A touch beside an open list closes it and does nothing else.
        if (current.panel.takeoffOpen || current.panel.materialOpen || current.panel.kindsOpen || current.panel.roomKindsOpen) {
            updatePanel {
                it.copy(
                    takeoffOpen = false,
                    materialOpen = false,
                    kindsOpen = false,
                    kindEdited = null,
                    roomKindsOpen = false,
                    conditionsOpen = false,
                    priceEntry = null,
                )
            }
            return
        }
        if (current.typing) return
        when (current.tools.tool) {
            Tool.SELECT -> {
                pick(onLevel, at, reach)
            }

            Tool.PENCIL -> {
                // The pencil draws strokes; a touch that went nowhere is not one.
            }

            Tool.WALL -> {
                aim(at, reach)
                commit()
            }

            Tool.WINDOW -> {
                place(onLevel, at, reach, OpeningKind.WINDOW, current.tools.windowSize, WindowSill)
            }

            Tool.DOOR -> {
                place(onLevel, at, reach, OpeningKind.DOOR, current.tools.doorSize, 0.mm)
            }
        }
    }

    /**
     * The finger is on the plan with the wall tool. The first point starts a run of walls; from
     * then on the engine says where a wall to the finger would end.
     */
    fun aim(
        at: PointMm,
        reach: Mm,
    ) {
        val touch = ++aims
        act {
            val current = state.value
            val level = current.plan.level ?: return@act
            if (current.tools.tool != Tool.WALL || current.typing) return@act
            // A touch that gives a room's corner starts no wall: the keys for its size open.
            if (current.tools.roomPlacing) {
                state.update {
                    it.copy(
                        tools =
                            it.tools.copy(
                                roomPlacing = false,
                            ),
                        panel =
                            it.panel.copy(
                                room = RoomEntry(at, reach),
                            ),
                    )
                }
                return@act
            }
            val from = run?.tip()
            // While this waited the finger moved on, and only where it is now matters.
            if (from != null && touch != aims) return@act
            val aimed = ask(EngineCommand.Aim(level, from, at, reach, Grid, square = current.tools.ortho)) ?: return@act
            if (from == null) {
                run = Run(aimed.point)
                touchStartedRun = true
                state.update {
                    it
                        .copy(
                            plan =
                                it.plan.copy(
                                    refused = null,
                                ),
                        ).withRun()
                }
            } else {
                updateTools { it.copy(draft = Draft(from, aimed)) }
            }
        }
    }

    /** A second finger came down: the plan moves instead, and the point is not placed. */
    fun cancelAim() =
        act {
            if (touchStartedRun) run = null
            touchStartedRun = false
            state.update { it.withRun() }
        }

    /** The finger left the plan: builds the aimed wall and starts the next one at its end. */
    fun commit() =
        act {
            touchStartedRun = false
            val to =
                state.value.tools.draft
                    ?.to
            state.update { it.withRun() }
            if (to != null) build(to.point, closes = to.meets)
        }

    /**
     * A stroke drawn with the pencil, as the finger's path in plan millimetres. The engine reads it
     * as walls (docs/adr/0012): built like the walls the wall tool draws, in one undo step, with
     * lengths that are not exact until they are typed.
     */
    fun sketch(
        points: List<PointMm>,
        reach: Mm,
    ) = act {
        val current = state.value
        val level = current.plan.level ?: return@act
        if (current.tools.tool != Tool.PENCIL || current.typing) return@act
        val stroke =
            EngineCommand.AddSketch(
                level = level,
                points = points,
                reach = reach,
                grid = Grid,
                square = current.tools.ortho,
                // A wall of a type has no thickness or unit of its own.
                thickness = if (current.tools.kind == null) current.tools.thickness else 0.mm,
                height = WallHeight,
                unit = current.tools.material.takeIf { current.tools.kind == null },
                type = current.tools.kind,
            )
        execute(listOf(stroke))
    }

    /** Asks the length of each sketched wall of the storey, one after another. */
    fun refine() =
        act {
            refined.clear()
            state.update {
                it.copy(
                    panel =
                        it.panel.copy(
                            entry = it.nextRough(step = 1, of = it.rough.size),
                        ),
                )
            }
        }

    // The next sketched wall of the pass, if one is left.
    private fun EditorUiState.nextRough(
        step: Int,
        of: Int,
    ): LengthEntry? =
        plan.dimensions
            .firstOrNull { !it.wall.exact && it.id !in refined }
            ?.let { LengthEntry(it.id, was = it.length, step = step, of = of) }

    /** Ends the run of walls being drawn. */
    fun finishWall() =
        act {
            run = null
            state.update { it.withRun() }
        }

    /** Goes on from the run's last point by measured lengths instead of touches. */
    fun startMeasure() =
        act {
            if (run != null) {
                state.update {
                    it
                        .copy(
                            tools =
                                it.tools.copy(
                                    measure = Measure(),
                                ),
                        ).withRun()
                }
            }
        }

    fun stopMeasure() =
        act {
            state.update {
                it
                    .copy(
                        tools =
                            it.tools.copy(
                                measure = null,
                            ),
                    ).withRun()
            }
        }

    fun setTurn(turn: Turn) = measured { it.copy(turn = turn) }

    /** Builds the measured wall and goes on to the next from its end, turning the same way. */
    fun nextMeasured() =
        act {
            val to =
                state.value.tools.draft
                    ?.to
            if (to != null && state.value.tools.measure != null) build(to.point, closes = to.meets)
        }

    /** Closes the measured run with a wall back to its first point. */
    fun closeMeasured() =
        act {
            val first = run?.first ?: return@act
            updateTools { it.copy(measure = null) }
            build(first, closes = true)
        }

    // Changes the measured wall and asks the engine where it would end.
    private fun measured(change: (Measure) -> Measure) =
        act {
            val current = state.value
            val level = current.plan.level ?: return@act
            val run = run ?: return@act
            val measure = change(current.tools.measure ?: return@act)
            updateTools { it.copy(measure = measure) }
            val length = measure.digits.toLongOrNull() ?: 0
            val from = run.tip()
            val aimed =
                if (length > 0) ask(EngineCommand.AimMeasured(level, from, run.before(), measure.turn, Mm(length))) else null
            updateTools { it.copy(draft = Draft(from, aimed)) }
        }

    // Builds a wall from the run's last point to [to]. The next one starts at its end, unless it
    // [closes] the run by meeting the plan; a wall the engine refuses leaves the run as it was.
    private suspend fun build(
        to: PointMm,
        closes: Boolean,
    ) {
        val current = state.value
        val level = current.plan.level ?: return
        val from = run?.tip() ?: return
        if (to == from) return
        val wall =
            Wall(
                level,
                from,
                to,
                // A wall of a type has no thickness or unit of its own.
                thickness = if (current.tools.kind == null) current.tools.thickness else 0.mm,
                height = WallHeight,
                unit = current.tools.material.takeIf { current.tools.kind == null },
                type = current.tools.kind,
            )
        val created = execute(listOf(EngineCommand.AddWall(wall))) ?: return
        run = run?.takeUnless { closes }?.let { it.copy(built = it.built + created) }
        state.update {
            it
                .copy(
                    tools =
                        it.tools.copy(
                            drawn = created,
                        ),
                ).withRun()
        }
    }

    // Where the run's next wall starts.
    private fun Run.tip(): PointMm = built.lastOrNull()?.let { model.walls[it]?.end } ?: first

    // Where the run's last wall started: a measured wall turns from that wall's direction.
    private fun Run.before(): PointMm? = built.lastOrNull()?.let { model.walls[it]?.start }

    // The run as the screen shows it: where its next wall starts, with nothing aimed or typed yet.
    private fun EditorUiState.withRun(): EditorUiState {
        val start = run?.tip()
        return copy(
            tools =
                tools.copy(
                    draft = start?.let(::Draft),
                    measure = tools.measure?.takeIf { start != null }?.copy(digits = ""),
                    canCloseRun = (run?.built?.size ?: 0) >= 2,
                ),
        )
    }

    fun toggleOrtho() = updateTools { it.copy(ortho = !it.ortho) }

    /** The canvas zoomed: the labels that fit now are not the ones that fitted before. */
    fun setLabelSize(size: LabelSize) =
        act {
            if (size == label) return@act
            label = size
            redraw()
        }

    /**
     * Every storey's drawing for a sheet, from the bottom up, with label text as big as [sizes]
     * say for each; null for a storey that has no size. For a sheet of [plans] every room is drawn
     * with its sizes.
     */
    suspend fun sheets(
        sizes: List<LabelSize?>,
        plans: Boolean = false,
    ): List<Drawing?> =
        session.inTurn {
            // A sheet of plans has every room's sizes; the estimate's small plans do not.
            val asked =
                model.levelsFromBottom().zip(sizes).mapNotNull { (level, size) ->
                    size?.let { EngineCommand.Draw(level, it.height, it.digit, sheet = plans) }
                }
            val drawings = session.ask(asked).map { (it as? CommandResult.Done)?.drawing }.iterator()
            sizes.map { size -> if (size == null || !drawings.hasNext()) null else drawings.next() }
        }

    // The selected space is drawn with its sizes in place of its name, and the selected opening
    // with the chain of the side it is in.
    private fun draw(level: ElementId): EngineCommand.Draw {
        val selected = state.value.plan.selected
        return EngineCommand.Draw(
            level,
            label.height,
            label.digit,
            focus = (selected as? Selected.SpacePart)?.at,
            selected = (selected as? Selected.OpeningPart)?.id,
        )
    }

    // Asks the shown storey's drawing again. The model does not change, so nothing is recorded.
    private suspend fun redraw() {
        val drawing =
            state.value.plan.level?.let { level ->
                (session.ask(listOf(draw(level))).singleOrNull() as? CommandResult.Done)?.drawing
            }
        updatePlan { it.copy(drawing = drawing ?: Drawing()) }
    }

    fun showTakeoff(open: Boolean) = updatePanel { it.copy(takeoffOpen = open, conditionsOpen = false) }

    /** Opens the job's estimating conditions in place of the take-off, or goes back to it. */
    fun showConditions(open: Boolean) = updatePanel { it.copy(conditionsOpen = open) }

    fun setCondition(
        condition: Condition,
        value: Int,
    ) = send(listOf(EngineCommand.SetCondition(condition, value)))

    fun showMaterials(open: Boolean) = updatePanel { it.copy(materialOpen = open) }

    /** Opens the wall types, for the selected wall or else for the walls drawn next. */
    fun showKinds(open: Boolean) = updatePanel { it.copy(kindsOpen = open, kindEdited = null, materialOpen = false) }

    /** Opens the layers of a wall type to change them; null goes back to the types. */
    fun editKind(kind: ElementId?) = updatePanel { it.copy(kindEdited = kind?.takeIf { id -> id in model.wallTypes }) }

    /**
     * Gives a layer of a wall type another thickness. Every wall of that type changes with it,
     * and the take-off follows.
     */
    fun setLayerThickness(
        layer: ElementId,
        thickness: Mm,
    ) = act {
        val now = model.wallLayers[layer] ?: return@act
        if (thickness != now.thickness) execute(listOf(EngineCommand.SetWallLayer(layer, thickness, now.unit)))
    }

    /** Opens the keys for a layer's thickness: insulation and a gap are as thick as the builder says. */
    fun editLayer(layer: ElementId) {
        val now = model.wallLayers[layer] ?: return
        updatePanel { it.copy(layerEntry = LayerEntry(layer, now.thickness)) }
    }

    fun cancelLayer() = updatePanel { it.copy(layerEntry = null) }

    fun confirmLayer() {
        val entry = state.value.panel.layerEntry ?: return
        val thickness = entry.digits.toLongOrNull()?.takeIf { it > 0 } ?: return
        updatePanel { it.copy(layerEntry = null) }
        setLayerThickness(entry.layer, Mm(thickness))
    }

    // Changes the digits of the layer's thickness being typed.
    private fun layered(change: (String) -> String) =
        updatePanel { current -> current.copy(layerEntry = current.layerEntry?.let { it.copy(digits = change(it.digits)) }) }

    /**
     * Makes the selected wall one of this type, or else the walls drawn from now on; null draws
     * walls with no type again. A choice for one wall is also the choice for new walls only while
     * the builder has made none for them.
     */
    fun chooseKind(kind: ElementId?) =
        act {
            updatePanel { it.copy(kindsOpen = false) }
            assign(kind)
        }

    /**
     * Starts a wall type from the catalog's [template] and chooses it, as [chooseKind] does. A
     * make-up the job already has is not made twice.
     */
    fun addKind(template: WallTypeTemplate) =
        act {
            updatePanel { it.copy(kindsOpen = false) }
            val known =
                state.value.plan.kinds
                    .find { it.matches(template) }
                    ?.id
            assign(known ?: execute(listOf(EngineCommand.AddWallType(template.role, template.layers))) ?: return@act)
        }

    private suspend fun assign(kind: ElementId?) {
        val wall = state.value.plan.selected as? Selected.WallPart
        state.update { current ->
            if (wall == null ||
                (current.tools.kind == null && current.tools.material == null)
            ) {
                current.copy(
                    tools =
                        current.tools.copy(
                            kind = kind,
                        ),
                )
            } else {
                current
            }
        }
        if (wall != null && kind != null && wall.wall.type != kind) execute(listOf(EngineCommand.SetWallType(wall.id, kind)))
    }

    /** Opens the names a room can have, for the selected space. */
    fun showRoomKinds(open: Boolean) = updatePanel { it.copy(roomKindsOpen = open) }

    /**
     * Names the selected space, with the point it was picked at as the room's anchor, or gives the
     * room that names it another name.
     */
    fun nameRoom(kind: RoomKind) {
        val part = state.value.plan.selected as? Selected.SpacePart ?: return
        val level = state.value.plan.level ?: return
        updatePanel { it.copy(roomKindsOpen = false) }
        val room = part.space.room
        send(listOf(if (room == null) EngineCommand.AddRoom(Room(level, part.at, kind)) else EngineCommand.SetRoomKind(room, kind)))
    }

    /** Which line of the selected wall its points lie on. */
    fun setLocationLine(line: LocationLine) {
        val wall = state.value.plan.selected as? Selected.WallPart ?: return
        send(listOf(EngineCommand.SetWallLocationLine(wall.id, line)))
    }

    /** Turns the selected wall's layers round: what was its outer face is now its inner one. */
    fun flipWall() {
        val wall = state.value.plan.selected as? Selected.WallPart ?: return
        send(listOf(EngineCommand.SetWallMirrored(wall.id, !wall.wall.mirrored)))
    }

    /**
     * Lays the selected wall, or else the walls drawn from now on, from [option]. A thickness the
     * unit cannot build becomes the nearest one it can, in the same undo step. A choice for one
     * wall is also the choice for new walls only while the builder has made none for them.
     */
    fun chooseMaterial(option: MasonryUnitOption) {
        val wall = state.value.plan.selected as? Selected.WallPart
        val fits = { thickness: Mm -> option.thicknesses.minByOrNull { abs(it.value - thickness.value) } ?: thickness }
        state.update { current ->
            val forNewWalls = wall == null || current.tools.material == null
            current.copy(
                tools =
                    current.tools.copy(
                        material = if (forNewWalls) option.size else current.tools.material,
                        thickness = if (forNewWalls) fits(wall?.wall?.thickness ?: current.tools.thickness) else current.tools.thickness,
                    ),
                panel =
                    current.panel.copy(
                        materialOpen = false,
                    ),
            )
        }
        if (wall == null) return
        val thickness = fits(wall.wall.thickness)
        send(
            gesture(
                listOfNotNull(
                    EngineCommand.SetWallUnit(wall.id, option.size),
                    EngineCommand.SetWallThickness(wall.id, thickness).takeIf { thickness != wall.wall.thickness },
                ),
            ),
        )
    }

    /** Opens the keys for a unit size the catalog does not list. */
    fun startSize() = updatePanel { it.copy(sizeEntry = SizeEntry()) }

    fun selectSizeField(index: Int) = updatePanel { current -> current.copy(sizeEntry = current.sizeEntry?.copy(active = index)) }

    fun cancelSize() = updatePanel { it.copy(sizeEntry = null) }

    /** Opens the keys for the section of the shown storey's strip footing. */
    fun startFooting() = updatePanel { it.copy(sizeEntry = SizeEntry(digits = listOf("", ""), footing = true)) }

    /** The strip footing under the shown storey's load-bearing walls; null removes it. */
    fun setFooting(footing: Footing?) {
        val level = state.value.plan.level ?: return
        send(listOf(EngineCommand.SetLevelFooting(level, footing)))
    }

    /** Chooses the typed size as the material, as [chooseMaterial] does a catalog unit, or sets the typed footing. */
    fun confirmSize() {
        state.value.panel.sizeEntry?.section?.let { section ->
            updatePanel { it.copy(sizeEntry = null) }
            return setFooting(section)
        }
        val size =
            state.value.panel.sizeEntry
                ?.size ?: return
        updatePanel { it.copy(sizeEntry = null) }
        val listed =
            state.value.catalog.units
                .find { it.size == size }
        chooseMaterial(listed ?: MasonryUnitOption(OWN_UNIT, MasonryUnitKind.BLOCK, mark = "", size, state.value.thicknessesOf(size)))
    }

    /** The ring beam on the shown storey's load-bearing walls; zero for none. */
    fun setRingBeam(height: Mm) {
        val level = state.value.plan.level ?: return
        send(listOf(EngineCommand.SetLevelRingBeam(level, height)))
    }

    fun setJunctionMeshes(meshes: Boolean) {
        val level = state.value.plan.level ?: return
        send(listOf(EngineCommand.SetLevelJunctionMeshes(level, meshes)))
    }

    fun selectTool(tool: Tool) =
        act {
            run = null
            state.update {
                it
                    .copy(
                        tools = it.tools.copy(tool = tool, roomPlacing = false, drawn = null),
                        panel = it.panel.copy(sizeEntry = null, materialOpen = false, kindsOpen = false, roomKindsOpen = false),
                    ).withRun()
            }
            select(null)
        }

    /** Opens the number pad for a wall's exact length. */
    fun editLength(wall: ElementId) {
        val dimension =
            state.value.plan.dimensions
                .find { it.id == wall } ?: return
        updatePanel { it.copy(entry = LengthEntry(wall, was = dimension.length)) }
    }

    /**
     * Opens the number pad for a size written on the plan (docs/adr/0016): the typed value goes
     * to what the size measures. A wall's length is typed as [editLength] has it.
     */
    fun editSize(size: DrawnText) {
        val element = size.element?.takeIf { size.isHandle } ?: return
        if (size.kind == TextKind.LENGTH || size.kind == TextKind.ABOUT_LENGTH) return editLength(element)
        updatePanel { it.copy(entry = LengthEntry(element, was = Mm(size.value), size = size)) }
    }

    /** Which of a clear size's two walls a typed value moves: the one at its near end when [swapped]. */
    fun chooseSizeWall(swapped: Boolean) =
        state.update { current ->
            // The wall that will move is the one the plan marks.
            current.copy(
                panel =
                    current.panel.copy(
                        entry =
                            current.panel.entry?.let { entry ->
                                val moves = if (swapped) entry.size?.other else entry.size?.element
                                entry.copy(swapped = swapped, wall = moves ?: entry.wall)
                            },
                    ),
            )
        }

    /** A key of the number pad: for the length being corrected, else for the measured wall. */
    fun typeDigit(digit: Int) {
        val typed = { digits: String -> (digits + digit).trimStart('0').takeIf { it.length <= MAX_LENGTH_DIGITS } ?: digits }
        if (state.value.panel.partition != null) return partitioned(typed)
        if (state.value.panel.room != null) return roomed(typed)
        if (state.value.panel.layerEntry != null) return layered(typed)
        // A window may start at the floor: a lone zero stays.
        if (state.value.panel.openingEntry != null) {
            return opened { digits -> (digits + digit).let { if (it.all { key -> key == '0' }) "0" else typed(digits) } }
        }
        if (state.value.panel.sizeEntry != null) return sized(typed)
        if (state.value.panel.priceEntry != null) {
            return priced { digits -> (digits + digit).trimStart('0').takeIf { it.length <= MAX_PRICE_DIGITS } ?: digits }
        }
        if (state.value.panel.entry == null) return measured { it.copy(digits = typed(it.digits)) }
        updatePanel { current -> current.copy(entry = current.entry?.let { it.copy(digits = typed(it.digits)) }) }
    }

    private fun priced(change: (String) -> String) =
        updatePanel { current -> current.copy(priceEntry = current.priceEntry?.let { it.copy(digits = change(it.digits)) }) }

    /** Opens the keys for what the builder pays for one unit of the line's item. */
    fun editPrice(line: QuantityLine) = updatePanel { it.copy(priceEntry = PriceEntry(line)) }

    fun cancelPrice() = updatePanel { it.copy(priceEntry = null) }

    /** Keeps the typed price in the builder's price list, for this job and every other; none typed forgets it. */
    fun confirmPrice() {
        val entry = state.value.panel.priceEntry ?: return
        updatePanel { it.copy(priceEntry = null) }
        val whole = entry.digits.toLongOrNull() ?: 0
        val price = if (whole > 0) Money(whole * PriceCurrency.minorPerUnit(), PriceCurrency) else null
        viewModelScope.launch { prices.set(entry.line.item, price) }
    }

    // Changes the digits of the unit size being typed.
    private fun sized(change: (String) -> String) =
        state.update { current ->
            val entry = current.panel.sizeEntry ?: return@update current
            current.copy(
                panel =
                    current.panel.copy(
                        sizeEntry =
                            entry.copy(
                                digits =
                                    entry.digits.mapIndexed { index, digits ->
                                        if (index ==
                                            entry.active
                                        ) {
                                            change(digits)
                                        } else {
                                            digits
                                        }
                                    },
                            ),
                    ),
            )
        }

    fun eraseDigit() {
        if (state.value.panel.partition != null) return partitioned { it.dropLast(1) }
        if (state.value.panel.room != null) return roomed { it.dropLast(1) }
        if (state.value.panel.layerEntry != null) return layered { it.dropLast(1) }
        if (state.value.panel.openingEntry != null) return opened { it.dropLast(1) }
        if (state.value.panel.sizeEntry != null) return sized { it.dropLast(1) }
        if (state.value.panel.priceEntry != null) return priced { it.dropLast(1) }
        if (state.value.panel.entry == null) return measured { it.copy(digits = it.digits.dropLast(1)) }
        updatePanel { current -> current.copy(entry = current.entry?.let { it.copy(digits = it.digits.dropLast(1)) }) }
    }

    fun cancelLength() = updatePanel { it.copy(entry = null) }

    /** The wall tool's next touch says where a room's corner goes, or no longer does. */
    fun toggleRoom() = updateTools { it.copy(roomPlacing = !it.roomPlacing && it.tool == Tool.WALL && it.draft == null) }

    /** Which of the room's two sizes the keys go to. */
    fun chooseRoomSize(index: Int) = updatePanel { current -> current.copy(room = current.room?.copy(active = index)) }

    /** Which way the room lies from its corner. */
    fun turnRoom(
        right: Boolean,
        up: Boolean,
    ) = updatePanel { current -> current.copy(room = current.room?.copy(right = right, up = up, refused = false)) }

    fun cancelRoom() = updatePanel { it.copy(room = null) }

    // Changes the digits of the room's size being typed.
    private fun roomed(change: (String) -> String) =
        state.update { current ->
            val entry = current.panel.room ?: return@update current
            val digits = entry.digits.mapIndexed { index, typed -> if (index == entry.active) change(typed) else typed }
            current.copy(
                panel =
                    current.panel.copy(
                        room = entry.copy(digits = digits, refused = false),
                    ),
            )
        }

    /**
     * After the width, goes on to the depth; with both, builds the room's walls like the walls
     * the wall tool draws. The engine says where they run and which walls already there the
     * room shares; a room it refuses stays on the keys to be corrected.
     */
    fun confirmRoom() =
        act {
            val current = state.value
            val entry = current.panel.room ?: return@act
            val level = current.plan.level ?: return@act
            val (width, depth) =
                entry.sizes ?: run {
                    updatePanel { it.copy(room = entry.copy(active = entry.digits.indexOfFirst(String::isEmpty).coerceAtLeast(0))) }
                    return@act
                }
            val walls =
                EngineCommand.AddRoomWalls(
                    level = level,
                    at = entry.at,
                    reach = entry.reach,
                    grid = Grid,
                    width = width,
                    depth = depth,
                    right = entry.right,
                    up = entry.up,
                    // A wall of a type has no thickness or unit of its own.
                    thickness = if (current.tools.kind == null) current.tools.thickness else 0.mm,
                    height = WallHeight,
                    unit = current.tools.material.takeIf { current.tools.kind == null },
                    type = current.tools.kind,
                )
            execute(listOf(walls))
            val refused = state.value.plan.refused != null
            state.update {
                it.copy(
                    plan =
                        it.plan.copy(
                            refused = null,
                        ),
                    panel =
                        it.panel.copy(
                            room = if (refused) entry.copy(refused = true) else null,
                        ),
                )
            }
        }

    /** Opens the keys for a wall across the selected room, a typed distance from one of its walls. */
    fun startPartition() {
        val room = state.value.plan.selected as? Selected.SpacePart ?: return
        updatePanel { it.copy(partition = PartitionEntry(room.at)) }
    }

    /** Which of the room's walls the distance is measured from. */
    fun choosePartitionWall(from: Hand) =
        updatePanel { current -> current.copy(partition = current.partition?.copy(from = from, refused = false)) }

    fun cancelPartition() = updatePanel { it.copy(partition = null) }

    // Changes the digits of the partition's distance.
    private fun partitioned(change: (String) -> String) =
        updatePanel { current -> current.copy(partition = current.partition?.let { it.copy(digits = change(it.digits), refused = false) }) }

    /**
     * Builds the wall across the room, like the walls the wall tool draws. The engine says where
     * it runs; a distance it refuses stays on the keys to be corrected.
     */
    fun confirmPartition() =
        act {
            val current = state.value
            val entry = current.panel.partition ?: return@act
            val level = current.plan.level ?: return@act
            val distance = entry.digits.toLongOrNull()?.let(::Mm) ?: return@act
            val across =
                EngineCommand.AddWallAcross(
                    level = level,
                    at = entry.at,
                    from = entry.from,
                    distance = distance,
                    // A wall of a type has no thickness or unit of its own.
                    thickness = if (current.tools.kind == null) current.tools.thickness else 0.mm,
                    height = WallHeight,
                    unit = current.tools.material.takeIf { current.tools.kind == null },
                    type = current.tools.kind,
                )
            val built = execute(listOf(across)) != null
            state.update {
                it.copy(
                    plan =
                        it.plan.copy(
                            refused = null,
                        ),
                    panel =
                        it.panel.copy(
                            partition = if (built) null else entry.copy(refused = true),
                        ),
                )
            }
        }

    /**
     * Gives the wall the typed length, which makes it exact. The engine says which end stays, a
     * partition's being the one on the wall it stands against, and carries what is joined to the
     * other, as one undo step. In a pass over sketched walls the next one is asked.
     */
    fun confirmLength() {
        val entry = state.value.panel.entry ?: return
        updatePanel { it.copy(entry = null) }
        val length = entry.digits.toLongOrNull()?.let(::Mm) ?: entry.was.takeIf { entry.refining } ?: return
        // The wall that moves is the size's element; the other of its two walls stays.
        val size = entry.size?.let { if (entry.swapped) it.copy(element = it.other, other = it.element) else it }
        val sized = size?.let { EngineCommand.SetDimension(it, length) } ?: EngineCommand.SetWallLength(entry.wall, length)
        act {
            execute(listOf(sized))
            if (entry.refining) {
                refined += entry.wall
                state.update {
                    it.copy(
                        panel =
                            it.panel.copy(
                                entry = it.nextRough(entry.step + 1, entry.of),
                            ),
                    )
                }
            }
        }
    }

    fun selectThickness(thickness: Mm) = updateTools { it.copy(thickness = thickness) }

    fun selectWindowSize(size: OpeningSize) = updateTools { it.copy(windowSize = size) }

    fun selectDoorSize(size: OpeningSize) = updateTools { it.copy(doorSize = size) }

    fun selectLevel(level: ElementId) =
        act {
            if (level in model.levels) {
                run = null
                state.update {
                    it.copy(
                        plan =
                            it.plan.copy(
                                refused = null,
                                selected = null,
                            ),
                        tools =
                            it.tools.copy(
                                drawn = null,
                            ),
                    )
                }
                show(level)
            }
        }

    /** A storey on top of the highest one, which the user then works on. */
    fun addLevel() {
        val top = model.levels.values.maxOfOrNull { it.elevation + it.height } ?: 0.mm
        send(listOf(EngineCommand.AddLevel(Level(elevation = top, height = StoreyHeight))), showCreated = true)
    }

    /** Takes one step back; a run goes on from its last wall still standing. */
    fun undo() =
        act {
            updateTools { it.copy(drawn = null) }
            execute(listOf(EngineCommand.Undo))
        }

    fun redo() = send(listOf(EngineCommand.Redo))

    fun deselect() =
        act {
            updatePanel { it.copy(materialOpen = false, kindsOpen = false, roomKindsOpen = false) }
            select(null)
        }

    fun setWallThickness(thickness: Mm) {
        val wall = state.value.plan.selected as? Selected.WallPart ?: return
        send(listOf(EngineCommand.SetWallThickness(wall.id, thickness)))
    }

    /**
     * Turns the selected opening into another kind, with that kind's usual size and sill; an empty
     * opening keeps its width. One undo step, centred where the opening was.
     */
    fun setOpeningKind(kind: OpeningKind) {
        val part = state.value.plan.selected as? Selected.OpeningPart ?: return
        val opening = part.opening
        if (opening.kind == kind) return
        val size =
            when (kind) {
                OpeningKind.WINDOW -> WindowSizes[1]
                OpeningKind.DOOR -> DoorSizes[1]
                OpeningKind.EMPTY -> OpeningSize(opening.width, DoorSizes[1].height)
            }
        val sill = if (kind == OpeningKind.WINDOW) WindowSill else 0.mm
        send(
            gesture(
                listOf(EngineCommand.SetOpeningKind(part.id, kind), EngineCommand.SetOpeningSill(part.id, sill)) +
                    resize(part.id, opening, size),
            ),
        )
    }

    /** Hangs the selected door on its other edge. */
    fun flipHinge() {
        val part = state.value.plan.selected as? Selected.OpeningPart ?: return
        val hinge = if (part.opening.hinge == Hinge.NEAR) Hinge.FAR else Hinge.NEAR
        send(listOf(EngineCommand.SetOpeningHinge(part.id, hinge)))
    }

    /** Makes the selected door open to the other side of its wall. */
    fun flipSwing() {
        val part = state.value.plan.selected as? Selected.OpeningPart ?: return
        val swing = if (part.opening.swing == Swing.LEFT) Swing.RIGHT else Swing.LEFT
        send(listOf(EngineCommand.SetOpeningSwing(part.id, swing)))
    }

    /** Puts the selected opening in the middle of the side of the room it is in. */
    fun centerOpening() {
        val part = state.value.plan.selected as? Selected.OpeningPart ?: return
        send(listOf(EngineCommand.CenterOpening(part.id)))
    }

    /** Resizes the selected opening about its centre, as one undo step. */
    fun setOpeningSize(size: OpeningSize) {
        val part = state.value.plan.selected as? Selected.OpeningPart ?: return
        send(gesture(resize(part.id, part.opening, size)))
    }

    /** Opens the keys for the selected opening's own sizes: a door has no height over the floor. */
    fun editOpening() {
        val part = state.value.plan.selected as? Selected.OpeningPart ?: return
        val opening = part.opening
        val was = listOfNotNull(opening.width, opening.height, opening.sill.takeIf { opening.kind != OpeningKind.DOOR })
        updatePanel { it.copy(openingEntry = OpeningEntry(part.id, was)) }
    }

    /** Which of the opening's sizes the keys go to. */
    fun chooseOpeningSize(index: Int) = updatePanel { current -> current.copy(openingEntry = current.openingEntry?.copy(active = index)) }

    fun cancelOpening() = updatePanel { it.copy(openingEntry = null) }

    /** Gives the opening the typed sizes about its centre, as one undo step. */
    fun confirmOpening() {
        val entry = state.value.panel.openingEntry ?: return
        updatePanel { it.copy(openingEntry = null) }
        val part = (state.value.plan.selected as? Selected.OpeningPart)?.takeIf { it.id == entry.opening } ?: return
        val sizes = entry.sizes
        if (sizes == entry.was) return
        val sill = sizes.getOrNull(2)?.takeIf { it != part.opening.sill }
        send(
            gesture(
                resize(part.id, part.opening, OpeningSize(sizes[0], sizes[1])) +
                    listOfNotNull(sill?.let { EngineCommand.SetOpeningSill(part.id, it) }),
            ),
        )
    }

    // Changes the digits of the opening's size being typed.
    private fun opened(change: (String) -> String) =
        state.update { current ->
            val entry = current.panel.openingEntry ?: return@update current
            val digits = entry.digits.mapIndexed { index, typed -> if (index == entry.active) change(typed) else typed }
            current.copy(
                panel =
                    current.panel.copy(
                        openingEntry = entry.copy(digits = digits),
                    ),
            )
        }

    fun removeSelected() {
        val command =
            when (val selected = state.value.plan.selected) {
                is Selected.WallPart -> EngineCommand.RemoveWall(selected.id)

                is Selected.OpeningPart -> EngineCommand.RemoveOpening(selected.id)

                // A space is not removed; the room that names it is.
                is Selected.SpacePart -> EngineCommand.RemoveRoom(selected.space.room ?: return)

                null -> return
            }
        send(listOf(command))
    }

    private fun pick(
        onLevel: ElementId,
        at: PointMm,
        reach: Mm,
    ) {
        act {
            val result = session.ask(listOf(EngineCommand.Pick(onLevel, at, reach))).singleOrNull() as? CommandResult.Done
            // An element first; where nothing built is near, the floor under the finger.
            val picked = result?.picked?.let(::selection) ?: result?.space?.let { Selected.SpacePart(at, it) }
            select(picked)
        }
    }

    // Selects [picked]. The plan is drawn again when what it shows of the selection changes: a
    // selected space has its sizes and one let go its name; a selected opening has its chain.
    private suspend fun select(picked: Selected?) {
        // What the drawing depends on: the point of a space, or an opening.
        val shown = { selected: Selected? -> (selected as? Selected.SpacePart)?.at to (selected as? Selected.OpeningPart)?.id }
        val before = shown(state.value.plan.selected)
        updatePlan { it.copy(selected = picked, refused = null) }
        if (shown(picked) != before) redraw()
    }

    // Asks the engine where a wall would end. The model does not change, so nothing is recorded.
    private suspend fun ask(aim: EngineCommand): Aimed? = (session.ask(listOf(aim)).singleOrNull() as? CommandResult.Done)?.aimed

    // The near edge moves by half the change in width, so the centre stays; it never passes the
    // wall's start. The engine reports an opening that ends past the wall's end.
    private fun resize(
        id: ElementId,
        opening: Opening,
        size: OpeningSize,
    ): List<EngineCommand> {
        val offset = maxOf(0.mm, Mm(opening.offset.value + (opening.width.value - size.width.value) / 2))
        return listOf(EngineCommand.MoveOpening(id, offset), EngineCommand.ResizeOpening(id, size.width, size.height))
    }

    private fun gesture(commands: List<EngineCommand>): List<EngineCommand> =
        listOf(EngineCommand.BeginGesture) + commands + EngineCommand.EndGesture

    private fun place(
        onLevel: ElementId,
        at: PointMm,
        reach: Mm,
        kind: OpeningKind,
        size: OpeningSize,
        sill: Mm,
    ) = send(listOf(EngineCommand.PlaceOpening(onLevel, at, reach, size.width, size.height, sill, kind)))

    private fun send(
        commands: List<EngineCommand>,
        showCreated: Boolean = false,
    ) = act { execute(commands, showCreated) }

    // Each changes one part of what the screen shows and leaves the others as they are.
    private fun updatePlan(change: (PlanState) -> PlanState) = state.update { it.copy(plan = change(it.plan)) }

    private fun updateTools(change: (ToolState) -> ToolState) = state.update { it.copy(tools = change(it.tools)) }

    private fun updatePanel(change: (PanelState) -> PanelState) = state.update { it.copy(panel = change(it.panel)) }

    private fun updateEstimate(change: (EstimateState) -> EstimateState) = state.update { it.copy(estimate = change(it.estimate)) }

    // Runs [step] in its turn: after the steps the builder made before it.
    private fun act(step: suspend () -> Unit) {
        viewModelScope.launch { session.inTurn(step) }
    }

    // Returns what the first command created. The shown storey's drawing is asked in the same
    // request, so the plan changes in one step.
    private suspend fun execute(
        commands: List<EngineCommand>,
        showCreated: Boolean = false,
    ): ElementId? {
        val level = state.value.plan.level
        // A selected space is found again under its point: the walls round it may have moved.
        val space = (state.value.plan.selected as? Selected.SpacePart)?.takeIf { level != null }
        val questions = listOfNotNull(level?.let(::draw), space?.let { EngineCommand.Pick(checkNotNull(level), it.at, 0.mm) })
        val outcome = session.execute(commands, questions)
        val answers = outcome.answers.filterIsInstance<CommandResult.Done>()
        val drawing = answers.firstNotNullOfOrNull { it.drawing }
        apply(
            outcome,
            show = outcome.created.takeIf { showCreated },
            drawn = if (level != null && drawing != null) level to drawing else null,
            space = answers.firstNotNullOfOrNull { it.space },
        )
        return outcome.created
    }

    // Shows what a step left: [show], or else the current storey while it exists; when undo
    // removed it, the top storey left. [drawn] is a storey's drawing that came with the answer,
    // and [space] the floor now under the selected space's point.
    private suspend fun apply(
        outcome: Outcome,
        show: ElementId? = null,
        drawn: Pair<ElementId, Drawing>? = null,
        space: Space? = null,
    ) {
        // Undo and removal take walls off the run: it goes on from the last one still standing.
        run = run?.let { it.copy(built = it.built.takeWhile { wall -> wall in model.walls }) }
        state.update { current ->
            current.copy(
                plan =
                    current.plan.copy(
                        canUndo = model.canUndo,
                        canRedo = model.canRedo,
                        refused = outcome.refused,
// The selection follows its element's new value and goes when the element does; a
// space goes when no floor lies under its point any more.
                        selected =
                            when (val selected = current.plan.selected) {
                                is Selected.SpacePart -> space?.let { selected.copy(space = it) }
                                is Selected.WallPart, is Selected.OpeningPart -> selection(checkNotNull(selected.id))
                                null -> null
                            },
                        kinds = kinds(),
                    ),
                tools =
                    current.tools.copy(
                        drawn = current.tools.drawn?.takeIf { it in model.walls },
// Undo may take a type away; walls are then drawn with no type again.
                        kind = current.tools.kind?.takeIf { it in model.wallTypes },
                    ),
                panel =
                    current.panel.copy(
                        roomKindsOpen = current.panel.roomKindsOpen && space != null,
                        kindEdited = current.panel.kindEdited?.takeIf { it in model.wallTypes },
                    ),
                estimate =
                    current.estimate.copy(
                        takeoff =
                            when (val takeoff = outcome.takeoff) {
                                is Takeoff.Lines -> merged(takeoff.lines)
                                is Takeoff.Failed -> emptyList()
                                null -> current.estimate.takeoff
                            },
                        takeoffFailed = outcome.takeoff?.let { it is Takeoff.Failed } ?: current.estimate.takeoffFailed,
                        conditions = model.conditions ?: Conditions(),
                    ),
            )
        }
        show(
            show?.takeIf { it in model.levels } ?: state.value.plan.level
                ?.takeIf { it in model.levels },
            drawn,
        )
    }

    // The job's wall types with their layers from the outer face in. Ids grow with time, so the
    // oldest type comes first.
    private fun kinds(): List<WallKind> =
        model.wallTypes.entries.sortedBy { it.key.uuid.toString() }.map { (id, type) ->
            val layers =
                model.wallLayers.entries
                    .filter { it.value.type == id }
                    .sortedWith(compareBy({ it.value.position }, { it.key.uuid.toString() }))
                    .map { it.key to it.value }
            WallKind(id, type.role, layers)
        }

    // Two rules may ask for the same material, as the footing and the ring beam do for bars of one
    // diameter; the builder buys it once, so such lines add up where the first one stands.
    private fun merged(lines: List<QuantityLine>): List<QuantityLine> =
        lines
            .groupBy { it.item to it.unit }
            .values
            .map { same -> same.first().copy(amount = same.sumOf { it.amount }) }

    private fun selection(id: ElementId): Selected? =
        model.walls[id]?.let { Selected.WallPart(id, it) }
            ?: model.openings[id]?.let { opening ->
                val alike =
                    model.openings.values.count {
                        it.kind == opening.kind && it.width == opening.width &&
                            it.height == opening.height
                    }
                Selected.OpeningPart(id, opening, alike)
            }

    // Shows a storey: [drawn] is its drawing if one came with the response, else the engine is
    // asked for it.
    private suspend fun show(
        level: ElementId?,
        drawn: Pair<ElementId, Drawing>? = null,
    ) {
        val ordered = model.levelsFromBottom()
        val shown = level ?: ordered.lastOrNull()
        if (shown != state.value.plan.level) run = null
        val storeys = ordered.map { storey -> Storey(storey, model.outlines.filter { it.level == storey }) }
        val outlines = storeys.find { it.level == shown }?.outlines.orEmpty()
        state.update { current ->
            val sameLevel = shown == current.plan.level
            current
                .copy(
                    plan =
                        current.plan.copy(
                            levels = ordered,
                            level = shown,
                            storeys = storeys,
                            outlines = outlines,
                            dimensions =
                                outlines.mapNotNull { outline ->
                                    model.walls[outline.wall]?.let { Dimension(outline.wall, it, outline.length) }
                                },
                            drawing = drawn?.second?.takeIf { drawn.first == shown } ?: current.plan.drawing,
                            issues = model.issues.filter { it.level == shown },
                            selected = if (sameLevel) current.plan.selected else null,
                        ),
                    estimate =
                        current.estimate.copy(
                            ringBeam = model.levels[shown]?.ringBeam ?: 0.mm,
                            footing = model.levels[shown]?.footing,
                            junctionMeshes = model.levels[shown]?.junctionMeshes ?: false,
                        ),
                ).withRun()
        }
        if (drawn?.first != shown) redraw()
    }

    // The thickness walls start in: the one nearest the middle of what the unit builds.
    private fun usual(thicknesses: List<Mm>): Mm? = thicknesses.getOrNull(thicknesses.size / 2)

    private companion object {
        val StoreyHeight = 3000.mm
        val Ground = Level(elevation = 0.mm, height = StoreyHeight)
        val WallHeight = 2800.mm

        // Walls are drawn to whole decimetres unless something on the plan catches them.
        val Grid = 100.mm

        // Up to 99 999 mm, more than any wall of a house.
        const val MAX_LENGTH_DIGITS = 5

        const val OWN_UNIT = "own"

        // Up to 999 999 999 a unit.
        const val MAX_PRICE_DIGITS = 9

        // Prices are in sum until a job chooses its currency.
        val PriceCurrency = Currency.UZS

        // A common sill height for living rooms; the user changes it per window later.
        val WindowSill = 900.mm
    }
}
