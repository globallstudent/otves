package uz.otves.feature.editor

import androidx.compose.runtime.Immutable
import androidx.lifecycle.ViewModel
import androidx.lifecycle.viewModelScope
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.asStateFlow
import kotlinx.coroutines.flow.update
import kotlinx.coroutines.launch
import kotlinx.coroutines.sync.Mutex
import kotlinx.coroutines.sync.withLock
import uz.otves.core.model.Currency
import uz.otves.core.model.ElementId
import uz.otves.core.model.Mm
import uz.otves.core.model.Money
import uz.otves.core.model.mm
import uz.otves.data.catalog.Prices
import uz.otves.data.project.JobStore
import uz.otves.engine.Aimed
import uz.otves.engine.Catalog
import uz.otves.engine.CommandResult
import uz.otves.engine.Condition
import uz.otves.engine.Conditions
import uz.otves.engine.Drawing
import uz.otves.engine.Engine
import uz.otves.engine.EngineCommand
import uz.otves.engine.EngineError
import uz.otves.engine.EngineResponse
import uz.otves.engine.Footing
import uz.otves.engine.Issue
import uz.otves.engine.IssueKind
import uz.otves.engine.Level
import uz.otves.engine.MasonryUnitKind
import uz.otves.engine.MasonryUnitOption
import uz.otves.engine.Opening
import uz.otves.engine.OpeningKind
import uz.otves.engine.PointMm
import uz.otves.engine.QuantityLine
import uz.otves.engine.Takeoff
import uz.otves.engine.Turn
import uz.otves.engine.UnitSize
import uz.otves.engine.Wall
import uz.otves.engine.WallOutline
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

/** The element the select tool picked, with its current value. */
internal sealed interface Selected {
    val id: ElementId

    data class WallPart(
        override val id: ElementId,
        val wall: Wall,
    ) : Selected

    data class OpeningPart(
        override val id: ElementId,
        val opening: Opening,
    ) : Selected
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
 * the length the sketch gave.
 */
internal data class LengthEntry(
    val wall: ElementId,
    val was: Mm,
    val digits: String = "",
    val step: Int = 0,
    val of: Int = 0,
) {
    val refining: Boolean
        get() = of > 0
}

@Immutable
internal data class EditorUiState(
    val ready: Boolean = false,
    /** The job's name, as the user gave it. */
    val name: String = "",
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
    val takeoff: List<QuantityLine> = emptyList(),
    val takeoffFailed: Boolean = false,
    /** The wall being drawn, from its first point on. */
    val draft: Draft? = null,
    /** Keeps new walls along the plan's axes. */
    val ortho: Boolean = true,
    /** The wall drawn last, whose length the user may now type. */
    val drawn: ElementId? = null,
    val entry: LengthEntry? = null,
    /** The builder's prices by take-off item, for one unit of each. */
    val prices: Map<String, Money> = emptyMap(),
    val priceEntry: PriceEntry? = null,
    /** A unit size of the builder's own is being typed. */
    val sizeEntry: SizeEntry? = null,
    /** Walls are being entered by their measured lengths. */
    val measure: Measure? = null,
    /** Whether the measured run can close back onto its first point. */
    val canCloseRun: Boolean = false,
    val takeoffOpen: Boolean = false,
    /** What the builder chooses from. */
    val catalog: Catalog = Catalog(),
    /** What new walls are laid from; null until the builder chooses. */
    val material: UnitSize? = null,
    /** The material list is open, for the selected wall or else for new walls. */
    val materialOpen: Boolean = false,
    /** What the job is estimated under; unset values follow the norms. */
    val conditions: Conditions = Conditions(),
    /** The conditions are open in place of the take-off. */
    val conditionsOpen: Boolean = false,
    /** What the shown storey asks for: its ring beam's height, zero for none, and junction meshes. */
    val ringBeam: Mm = 0.mm,
    val footing: Footing? = null,
    val junctionMeshes: Boolean = false,
    val tool: Tool = Tool.WALL,
    val selected: Selected? = null,
    val thickness: Mm = 0.mm,
    val windowSize: OpeningSize = WindowSizes[1],
    val doorSize: OpeningSize = DoorSizes[1],
    val canUndo: Boolean = false,
    val canRedo: Boolean = false,
    /** Why the engine refused the last command, until the next one. */
    val refused: EngineError? = null,
) {
    /** A number pad is open: until the number is entered, touches on the plan only move it. */
    val typing: Boolean
        get() = entry != null || sizeEntry != null || priceEntry != null || measure != null

    /** Walls of the shown storey read from a sketch, whose lengths nobody has given yet. */
    val rough: List<ElementId>
        get() = dimensions.filterNot { it.wall.exact }.map { it.id }

    /** Walls of the shown storey that have no material yet. */
    val unchosen: Set<ElementId>
        get() = issues.filter { it.kind == IssueKind.MATERIAL_NOT_CHOSEN }.mapNotNull { it.element }.toSet()

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
 * digits typed so far. Everything shown comes from engine responses, and every response is
 * written to the job file. The view model owns [job] and closes it when cleared.
 */
internal class EditorViewModel(
    private val engine: Engine,
    loadContent: suspend () -> EngineCommand.LoadContent,
    loadCatalog: suspend () -> Catalog,
    private val job: JobStore,
    private val prices: Prices,
) : ViewModel() {
    private val state = MutableStateFlow(EditorUiState())
    val uiState: StateFlow<EditorUiState> = state.asStateFlow()

    // Undo stops at the user's first step, so it never removes the ground floor of a new job.
    private var userSteps = 0
    private val levels = mutableMapOf<ElementId, Level>()
    private val walls = mutableMapOf<ElementId, Wall>()
    private val openings = mutableMapOf<ElementId, Opening>()
    private var conditions: Pair<ElementId, Conditions>? = null
    private var wallOutlines: List<WallOutline> = emptyList()
    private var allIssues: List<Issue> = emptyList()

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

    // One step at a time, in the order the builder made them: each sees what the ones before it
    // left, and the log keeps the engine's order.
    private val turn = Mutex()

    init {
        viewModelScope.launch { prices.all.collect { all -> state.update { it.copy(prices = all) } } }
        viewModelScope.launch {
            turn.withLock {
                val saved = job.load()
                val commands =
                    buildList {
                        add(loadContent())
                        add(EngineCommand.OpenModel(saved.elements, saved.clock))
                        // A new job comes with its ground floor.
                        if (saved.elements.values.none { it is Level }) add(EngineCommand.AddLevel(Ground))
                    }
                val response = engine.send(commands)
                job.record(response)
                apply(response)
                show(orderedLevels().firstOrNull())
                val catalog = loadCatalog()
                state.update { current ->
                    // New walls go on as the job's walls are: the material and thickness of one of them.
                    val like = walls.values.lastOrNull { it.unit != null } ?: walls.values.lastOrNull()
                    val thickness =
                        like?.thickness ?: catalog.units
                            .firstOrNull()
                            ?.thicknesses
                            ?.let(::usual) ?: current.thickness
                    current.copy(
                        ready = levels.isNotEmpty(),
                        name = saved.name,
                        catalog = catalog,
                        material = like?.unit,
                        thickness = thickness,
                    )
                }
            }
        }
    }

    override fun onCleared() = job.close()

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
        val onLevel = current.level ?: return
        // A touch beside an open list closes it and does nothing else.
        if (current.takeoffOpen || current.materialOpen) {
            state.update { it.copy(takeoffOpen = false, materialOpen = false, conditionsOpen = false, priceEntry = null) }
            return
        }
        if (current.typing) return
        when (current.tool) {
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
                place(onLevel, at, reach, OpeningKind.WINDOW, current.windowSize, WindowSill)
            }

            Tool.DOOR -> {
                place(onLevel, at, reach, OpeningKind.DOOR, current.doorSize, 0.mm)
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
            val level = current.level ?: return@act
            if (current.tool != Tool.WALL || current.typing) return@act
            val from = run?.tip()
            // While this waited the finger moved on, and only where it is now matters.
            if (from != null && touch != aims) return@act
            val aimed = ask(EngineCommand.Aim(level, from, at, reach, Grid, square = current.ortho)) ?: return@act
            if (from == null) {
                run = Run(aimed.point)
                touchStartedRun = true
                state.update { it.copy(refused = null).withRun() }
            } else {
                state.update { it.copy(draft = Draft(from, aimed)) }
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
            val to = state.value.draft?.to
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
        val level = current.level ?: return@act
        if (current.tool != Tool.PENCIL || current.typing) return@act
        val stroke =
            EngineCommand.AddSketch(
                level = level,
                points = points,
                reach = reach,
                grid = Grid,
                square = current.ortho,
                thickness = current.thickness,
                height = WallHeight,
                unit = current.material,
            )
        execute(listOf(stroke))
    }

    /** Asks the length of each sketched wall of the storey, one after another. */
    fun refine() =
        act {
            refined.clear()
            state.update { it.copy(entry = it.nextRough(step = 1, of = it.rough.size)) }
        }

    // The next sketched wall of the pass, if one is left.
    private fun EditorUiState.nextRough(
        step: Int,
        of: Int,
    ): LengthEntry? =
        dimensions
            .firstOrNull { !it.wall.exact && it.id !in refined }
            ?.let { LengthEntry(it.id, was = it.length, step = step, of = of) }

    /** Ends the run of walls being drawn. */
    fun finishWall() =
        act {
            run = null
            state.update { it.withRun() }
        }

    /** Goes on from the run's last point by measured lengths instead of touches. */
    fun startMeasure() = act { if (run != null) state.update { it.copy(measure = Measure()).withRun() } }

    fun stopMeasure() = act { state.update { it.copy(measure = null).withRun() } }

    fun setTurn(turn: Turn) = measured { it.copy(turn = turn) }

    /** Builds the measured wall and goes on to the next from its end, turning the same way. */
    fun nextMeasured() =
        act {
            val to = state.value.draft?.to
            if (to != null && state.value.measure != null) build(to.point, closes = to.meets)
        }

    /** Closes the measured run with a wall back to its first point. */
    fun closeMeasured() =
        act {
            val first = run?.first ?: return@act
            state.update { it.copy(measure = null) }
            build(first, closes = true)
        }

    // Changes the measured wall and asks the engine where it would end.
    private fun measured(change: (Measure) -> Measure) =
        act {
            val current = state.value
            val level = current.level ?: return@act
            val run = run ?: return@act
            val measure = change(current.measure ?: return@act)
            state.update { it.copy(measure = measure) }
            val length = measure.digits.toLongOrNull() ?: 0
            val from = run.tip()
            val aimed =
                if (length > 0) ask(EngineCommand.AimMeasured(level, from, run.before(), measure.turn, Mm(length))) else null
            state.update { it.copy(draft = Draft(from, aimed)) }
        }

    // Builds a wall from the run's last point to [to]. The next one starts at its end, unless it
    // [closes] the run by meeting the plan; a wall the engine refuses leaves the run as it was.
    private suspend fun build(
        to: PointMm,
        closes: Boolean,
    ) {
        val current = state.value
        val level = current.level ?: return
        val from = run?.tip() ?: return
        if (to == from) return
        val wall = Wall(level, from, to, thickness = current.thickness, height = WallHeight, unit = current.material)
        val created = execute(listOf(EngineCommand.AddWall(wall))) ?: return
        run = run?.takeUnless { closes }?.let { it.copy(built = it.built + created) }
        state.update { it.copy(drawn = created).withRun() }
    }

    // Where the run's next wall starts.
    private fun Run.tip(): PointMm = built.lastOrNull()?.let { walls[it]?.end } ?: first

    // Where the run's last wall started: a measured wall turns from that wall's direction.
    private fun Run.before(): PointMm? = built.lastOrNull()?.let { walls[it]?.start }

    // The run as the screen shows it: where its next wall starts, with nothing aimed or typed yet.
    private fun EditorUiState.withRun(): EditorUiState {
        val start = run?.tip()
        return copy(
            draft = start?.let(::Draft),
            measure = measure?.takeIf { start != null }?.copy(digits = ""),
            canCloseRun = (run?.built?.size ?: 0) >= 2,
        )
    }

    fun toggleOrtho() = state.update { it.copy(ortho = !it.ortho) }

    /** The canvas zoomed: the labels that fit now are not the ones that fitted before. */
    fun setLabelSize(size: LabelSize) =
        act {
            if (size == label) return@act
            label = size
            redraw()
        }

    /**
     * Every storey's drawing for a sheet, from the bottom up, with label text as big as [sizes]
     * say for each; null for a storey that has no size.
     */
    suspend fun sheets(sizes: List<LabelSize?>): List<Drawing?> =
        turn.withLock {
            val asked =
                orderedLevels().zip(sizes).mapNotNull { (level, size) ->
                    size?.let { EngineCommand.Draw(level, it.height, it.digit) }
                }
            val drawings =
                engine
                    .send(asked)
                    .results
                    .map { (it as? CommandResult.Done)?.drawing }
                    .iterator()
            sizes.map { size -> if (size == null || !drawings.hasNext()) null else drawings.next() }
        }

    private fun draw(level: ElementId) = EngineCommand.Draw(level, label.height, label.digit)

    // Asks the shown storey's drawing again. The model does not change, so nothing is recorded.
    private suspend fun redraw() {
        val drawing =
            state.value.level?.let { level ->
                (engine.send(listOf(draw(level))).results.singleOrNull() as? CommandResult.Done)?.drawing
            }
        state.update { it.copy(drawing = drawing ?: Drawing()) }
    }

    fun showTakeoff(open: Boolean) = state.update { it.copy(takeoffOpen = open, conditionsOpen = false) }

    /** Opens the job's estimating conditions in place of the take-off, or goes back to it. */
    fun showConditions(open: Boolean) = state.update { it.copy(conditionsOpen = open) }

    fun setCondition(
        condition: Condition,
        value: Int,
    ) = send(listOf(EngineCommand.SetCondition(condition, value)))

    fun showMaterials(open: Boolean) = state.update { it.copy(materialOpen = open) }

    /**
     * Lays the selected wall, or else the walls drawn from now on, from [option]. A thickness the
     * unit cannot build becomes the nearest one it can, in the same undo step. A choice for one
     * wall is also the choice for new walls only while the builder has made none for them.
     */
    fun chooseMaterial(option: MasonryUnitOption) {
        val wall = state.value.selected as? Selected.WallPart
        val fits = { thickness: Mm -> option.thicknesses.minByOrNull { abs(it.value - thickness.value) } ?: thickness }
        state.update { current ->
            val forNewWalls = wall == null || current.material == null
            current.copy(
                materialOpen = false,
                material = if (forNewWalls) option.size else current.material,
                thickness = if (forNewWalls) fits(wall?.wall?.thickness ?: current.thickness) else current.thickness,
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
    fun startSize() = state.update { it.copy(sizeEntry = SizeEntry()) }

    fun selectSizeField(index: Int) = state.update { current -> current.copy(sizeEntry = current.sizeEntry?.copy(active = index)) }

    fun cancelSize() = state.update { it.copy(sizeEntry = null) }

    /** Opens the keys for the section of the shown storey's strip footing. */
    fun startFooting() = state.update { it.copy(sizeEntry = SizeEntry(digits = listOf("", ""), footing = true)) }

    /** The strip footing under the shown storey's load-bearing walls; null removes it. */
    fun setFooting(footing: Footing?) {
        val level = state.value.level ?: return
        send(listOf(EngineCommand.SetLevelFooting(level, footing)))
    }

    /** Chooses the typed size as the material, as [chooseMaterial] does a catalog unit, or sets the typed footing. */
    fun confirmSize() {
        state.value.sizeEntry?.section?.let { section ->
            state.update { it.copy(sizeEntry = null) }
            return setFooting(section)
        }
        val size = state.value.sizeEntry?.size ?: return
        state.update { it.copy(sizeEntry = null) }
        val listed =
            state.value.catalog.units
                .find { it.size == size }
        chooseMaterial(listed ?: MasonryUnitOption(OWN_UNIT, MasonryUnitKind.BLOCK, mark = "", size, state.value.thicknessesOf(size)))
    }

    /** The ring beam on the shown storey's load-bearing walls; zero for none. */
    fun setRingBeam(height: Mm) {
        val level = state.value.level ?: return
        send(listOf(EngineCommand.SetLevelRingBeam(level, height)))
    }

    fun setJunctionMeshes(meshes: Boolean) {
        val level = state.value.level ?: return
        send(listOf(EngineCommand.SetLevelJunctionMeshes(level, meshes)))
    }

    fun selectTool(tool: Tool) =
        act {
            run = null
            state.update {
                it
                    .copy(
                        tool = tool,
                        sizeEntry = null,
                        drawn = null,
                        selected = null,
                        materialOpen = false,
                        refused = null,
                    ).withRun()
            }
        }

    /** Opens the number pad for a wall's exact length. */
    fun editLength(wall: ElementId) {
        val dimension = state.value.dimensions.find { it.id == wall } ?: return
        state.update { it.copy(entry = LengthEntry(wall, was = dimension.length)) }
    }

    /** A key of the number pad: for the length being corrected, else for the measured wall. */
    fun typeDigit(digit: Int) {
        val typed = { digits: String -> (digits + digit).trimStart('0').takeIf { it.length <= MAX_LENGTH_DIGITS } ?: digits }
        if (state.value.sizeEntry != null) return sized(typed)
        if (state.value.priceEntry != null) {
            return priced { digits -> (digits + digit).trimStart('0').takeIf { it.length <= MAX_PRICE_DIGITS } ?: digits }
        }
        if (state.value.entry == null) return measured { it.copy(digits = typed(it.digits)) }
        state.update { current -> current.copy(entry = current.entry?.let { it.copy(digits = typed(it.digits)) }) }
    }

    private fun priced(change: (String) -> String) =
        state.update { current -> current.copy(priceEntry = current.priceEntry?.let { it.copy(digits = change(it.digits)) }) }

    /** Opens the keys for what the builder pays for one unit of the line's item. */
    fun editPrice(line: QuantityLine) = state.update { it.copy(priceEntry = PriceEntry(line)) }

    fun cancelPrice() = state.update { it.copy(priceEntry = null) }

    /** Keeps the typed price in the builder's price list, for this job and every other; none typed forgets it. */
    fun confirmPrice() {
        val entry = state.value.priceEntry ?: return
        state.update { it.copy(priceEntry = null) }
        val whole = entry.digits.toLongOrNull() ?: 0
        val price = if (whole > 0) Money(whole * PriceCurrency.minorPerUnit(), PriceCurrency) else null
        viewModelScope.launch { prices.set(entry.line.item, price) }
    }

    // Changes the digits of the unit size being typed.
    private fun sized(change: (String) -> String) =
        state.update { current ->
            val entry = current.sizeEntry ?: return@update current
            current.copy(
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
            )
        }

    fun eraseDigit() {
        if (state.value.sizeEntry != null) return sized { it.dropLast(1) }
        if (state.value.priceEntry != null) return priced { it.dropLast(1) }
        if (state.value.entry == null) return measured { it.copy(digits = it.digits.dropLast(1)) }
        state.update { current -> current.copy(entry = current.entry?.let { it.copy(digits = it.digits.dropLast(1)) }) }
    }

    fun cancelLength() = state.update { it.copy(entry = null) }

    /**
     * Gives the wall the typed length, which makes it exact. The engine says which end stays, a
     * partition's being the one on the wall it stands against, and carries what is joined to the
     * other, as one undo step. In a pass over sketched walls the next one is asked.
     */
    fun confirmLength() {
        val entry = state.value.entry ?: return
        state.update { it.copy(entry = null) }
        val length = entry.digits.toLongOrNull()?.let(::Mm) ?: entry.was.takeIf { entry.refining } ?: return
        act {
            execute(listOf(EngineCommand.SetWallLength(entry.wall, length)))
            if (entry.refining) {
                refined += entry.wall
                state.update { it.copy(entry = it.nextRough(entry.step + 1, entry.of)) }
            }
        }
    }

    fun selectThickness(thickness: Mm) = state.update { it.copy(thickness = thickness) }

    fun selectWindowSize(size: OpeningSize) = state.update { it.copy(windowSize = size) }

    fun selectDoorSize(size: OpeningSize) = state.update { it.copy(doorSize = size) }

    fun selectLevel(level: ElementId) =
        act {
            if (level in levels) {
                run = null
                state.update { it.copy(refused = null, selected = null, drawn = null) }
                show(level)
            }
        }

    /** A storey on top of the highest one, which the user then works on. */
    fun addLevel() {
        val top = levels.values.maxOfOrNull { it.elevation + it.height } ?: 0.mm
        send(listOf(EngineCommand.AddLevel(Level(elevation = top, height = StoreyHeight))), showCreated = true)
    }

    /** Takes one step back; a run goes on from its last wall still standing. */
    fun undo() =
        act {
            state.update { it.copy(drawn = null) }
            execute(listOf(EngineCommand.Undo))
        }

    fun redo() = send(listOf(EngineCommand.Redo))

    fun deselect() = state.update { it.copy(selected = null, materialOpen = false) }

    fun setWallThickness(thickness: Mm) {
        val wall = state.value.selected as? Selected.WallPart ?: return
        send(listOf(EngineCommand.SetWallThickness(wall.id, thickness)))
    }

    /**
     * Turns the selected opening into another kind, with that kind's usual size and sill; an empty
     * opening keeps its width. One undo step, centred where the opening was.
     */
    fun setOpeningKind(kind: OpeningKind) {
        val part = state.value.selected as? Selected.OpeningPart ?: return
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

    /** Resizes the selected opening about its centre, as one undo step. */
    fun setOpeningSize(size: OpeningSize) {
        val part = state.value.selected as? Selected.OpeningPart ?: return
        send(gesture(resize(part.id, part.opening, size)))
    }

    fun removeSelected() {
        val command =
            when (val selected = state.value.selected) {
                is Selected.WallPart -> EngineCommand.RemoveWall(selected.id)
                is Selected.OpeningPart -> EngineCommand.RemoveOpening(selected.id)
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
            val result = engine.send(listOf(EngineCommand.Pick(onLevel, at, reach))).results.singleOrNull()
            val picked = (result as? CommandResult.Done)?.picked
            state.update { it.copy(selected = picked?.let(::selection), refused = null) }
        }
    }

    // Asks the engine where a wall would end. The model does not change, so nothing is recorded.
    private suspend fun ask(aim: EngineCommand): Aimed? = (engine.send(listOf(aim)).results.singleOrNull() as? CommandResult.Done)?.aimed

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

    // Runs [step] in its turn.
    private fun act(step: suspend () -> Unit) {
        viewModelScope.launch { turn.withLock { step() } }
    }

    // Returns what the first command created. The shown storey's drawing is asked in the same
    // request, so the plan changes in one step.
    private suspend fun execute(
        commands: List<EngineCommand>,
        showCreated: Boolean = false,
    ): ElementId? {
        val level = state.value.level
        val response = engine.send(commands + listOfNotNull(level?.let(::draw)))
        job.record(response)
        // Each group in the log is one step: undo takes one back, anything else adds one.
        userSteps += response.log.sumOf { if (it.isUndo) -1 else 1 }
        val created = (response.results.firstOrNull() as? CommandResult.Done)?.created
        val drawing = (response.results.getOrNull(commands.size) as? CommandResult.Done)?.drawing
        apply(
            response.copy(results = response.results.take(commands.size)),
            show = created.takeIf { showCreated },
            drawn = if (level != null && drawing != null) level to drawing else null,
        )
        return created
    }

    // Mirrors the engine's changes, then shows [show], or else stays on the current storey while
    // it exists; when undo removed it, the top storey left. [drawn] is a storey's drawing that
    // came with the response.
    private suspend fun apply(
        response: EngineResponse,
        show: ElementId? = null,
        drawn: Pair<ElementId, Drawing>? = null,
    ) {
        for (change in response.changes) {
            when (val element = change.element) {
                is Level -> {
                    levels[change.id] = element
                }

                is Wall -> {
                    walls[change.id] = element
                }

                is Opening -> {
                    openings[change.id] = element
                }

                is Conditions -> {
                    conditions = change.id to element
                }

                null -> {
                    if (conditions?.first == change.id) conditions = null
                    levels.remove(change.id) ?: walls.remove(change.id) ?: openings.remove(change.id)
                }
            }
        }
        // Undo and removal take walls off the run: it goes on from the last one still standing.
        run = run?.let { it.copy(built = it.built.takeWhile { wall -> wall in walls }) }
        wallOutlines = response.outlines ?: wallOutlines
        allIssues = response.issues ?: allIssues
        state.update { current ->
            current.copy(
                takeoff =
                    when (val takeoff = response.takeoff) {
                        is Takeoff.Lines -> merged(takeoff.lines)
                        is Takeoff.Failed -> emptyList()
                        null -> current.takeoff
                    },
                takeoffFailed = response.takeoff?.let { it is Takeoff.Failed } ?: current.takeoffFailed,
                canUndo = response.canUndo && userSteps > 0,
                canRedo = response.canRedo,
                refused =
                    response.results
                        .filterIsInstance<CommandResult.Failed>()
                        .firstOrNull()
                        ?.error,
                // The selection follows its element's new value and goes when the element does.
                selected = current.selected?.let { selection(it.id) },
                drawn = current.drawn?.takeIf { it in walls },
                conditions = conditions?.second ?: Conditions(),
            )
        }
        show(show?.takeIf { it in levels } ?: state.value.level?.takeIf { it in levels }, drawn)
    }

    // Two rules may ask for the same material, as the footing and the ring beam do for bars of one
    // diameter; the builder buys it once, so such lines add up where the first one stands.
    private fun merged(lines: List<QuantityLine>): List<QuantityLine> =
        lines
            .groupBy { it.item to it.unit }
            .values
            .map { same -> same.first().copy(amount = same.sumOf { it.amount }) }

    private fun selection(id: ElementId): Selected? =
        walls[id]?.let { Selected.WallPart(id, it) } ?: openings[id]?.let { Selected.OpeningPart(id, it) }

    // Shows a storey: [drawn] is its drawing if one came with the response, else the engine is
    // asked for it.
    private suspend fun show(
        level: ElementId?,
        drawn: Pair<ElementId, Drawing>? = null,
    ) {
        val ordered = orderedLevels()
        val shown = level ?: ordered.lastOrNull()
        if (shown != state.value.level) run = null
        val storeys = ordered.map { storey -> Storey(storey, wallOutlines.filter { it.level == storey }) }
        val outlines = storeys.find { it.level == shown }?.outlines.orEmpty()
        state.update { current ->
            val sameLevel = shown == current.level
            current
                .copy(
                    levels = ordered,
                    level = shown,
                    storeys = storeys,
                    outlines = outlines,
                    dimensions =
                        outlines.mapNotNull { outline ->
                            walls[outline.wall]?.let { Dimension(outline.wall, it, outline.length) }
                        },
                    drawing = drawn?.second?.takeIf { drawn.first == shown } ?: current.drawing,
                    issues = allIssues.filter { it.level == shown },
                    ringBeam = levels[shown]?.ringBeam ?: 0.mm,
                    footing = levels[shown]?.footing,
                    junctionMeshes = levels[shown]?.junctionMeshes ?: false,
                    selected = if (sameLevel) current.selected else null,
                ).withRun()
        }
        if (drawn?.first != shown) redraw()
    }

    // By elevation; storeys at one elevation keep the order of their ids, which grow with time.
    private fun orderedLevels(): List<ElementId> =
        levels.entries
            .sortedWith(compareBy<Map.Entry<ElementId, Level>> { it.value.elevation }.thenBy { it.key.uuid.toString() })
            .map { it.key }

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
