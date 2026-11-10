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
import uz.otves.core.model.ElementId
import uz.otves.core.model.Mm
import uz.otves.core.model.mm
import uz.otves.data.project.JobStore
import uz.otves.engine.Catalog
import uz.otves.engine.CommandResult
import uz.otves.engine.Engine
import uz.otves.engine.EngineCommand
import uz.otves.engine.EngineError
import uz.otves.engine.EngineResponse
import uz.otves.engine.Issue
import uz.otves.engine.IssueKind
import uz.otves.engine.Level
import uz.otves.engine.MasonryUnitKind
import uz.otves.engine.MasonryUnitOption
import uz.otves.engine.Opening
import uz.otves.engine.OpeningKind
import uz.otves.engine.OpeningOutline
import uz.otves.engine.PointMm
import uz.otves.engine.QuantityLine
import uz.otves.engine.Takeoff
import uz.otves.engine.UnitSize
import uz.otves.engine.Wall
import uz.otves.engine.WallEnd
import uz.otves.engine.WallOutline
import kotlin.math.abs
import kotlin.math.hypot
import kotlin.math.roundToLong

/** What a tap on the plan does. */
internal enum class Tool { SELECT, WALL, WINDOW, DOOR }

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

/** A line through an existing wall end that the aimed point lined up with. */
internal data class Guide(
    val from: PointMm,
    val to: PointMm,
)

/**
 * A wall being drawn: it starts at [start] and, while the finger is down, would end at [end].
 * After each wall the next one starts where it ended.
 */
internal data class Draft(
    val start: PointMm,
    val end: PointMm? = null,
    val guides: List<Guide> = emptyList(),
    /** [end] lies on another wall, between its ends: the wall will meet it there. */
    val meets: Boolean = false,
) {
    /** Along the wall's axis, as the engine will report it. */
    val length: Mm?
        get() = end?.let { Mm(hypot((it.x.value - start.x.value).toDouble(), (it.y.value - start.y.value).toDouble()).roundToLong()) }
}

/** Which way a measured wall goes from where the last one ended. */
internal enum class Turn { LEFT, STRAIGHT, RIGHT }

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
 * A masonry unit the catalog does not list, being typed: its size as laid in millimetres, as
 * digits so far, and which of the three the keys go to.
 */
internal data class SizeEntry(
    val digits: List<String> = listOf("", "", ""),
    val active: Int = 0,
) {
    /** The size once all three are typed. */
    val size: UnitSize?
        get() {
            val (length, width, height) = digits.map { it.toLongOrNull() ?: 0 }
            return if (length > 0 && width > 0 && height > 0) UnitSize(Mm(length), Mm(width), Mm(height)) else null
        }
}

/** A wall's exact length being typed: the digits so far, in millimetres. */
internal data class LengthEntry(
    val wall: ElementId,
    val was: Mm,
    val digits: String = "",
)

@Immutable
internal data class EditorUiState(
    val ready: Boolean = false,
    /** The job's name, as the user gave it. */
    val name: String = "",
    /** Storeys from the bottom up; the user works on [level]. */
    val levels: List<ElementId> = emptyList(),
    val level: ElementId? = null,
    /** The current storey's walls and openings. */
    val outlines: List<WallOutline> = emptyList(),
    val openings: List<OpeningOutline> = emptyList(),
    /** The walls of the storey below, drawn faintly to build on. */
    val below: List<WallOutline> = emptyList(),
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
    /** What the shown storey asks for: its ring beam's height, zero for none, and junction meshes. */
    val ringBeam: Mm = 0.mm,
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
 * The plan of one job, one storey at a time. The engine owns the model; this keeps only what the
 * screen draws, the wall ends it snaps to and the values of what can be selected, all taken from
 * engine responses, and writes every response to the job file. The view model owns [job] and
 * closes it when cleared.
 */
internal class EditorViewModel(
    private val engine: Engine,
    loadContent: suspend () -> EngineCommand.LoadContent,
    loadCatalog: suspend () -> Catalog,
    private val job: JobStore,
) : ViewModel() {
    private val state = MutableStateFlow(EditorUiState())
    val uiState: StateFlow<EditorUiState> = state.asStateFlow()

    // Undo stops at the user's first step, so it never removes the ground floor of a new job.
    private var userSteps = 0
    private val levels = mutableMapOf<ElementId, Level>()
    private val walls = mutableMapOf<ElementId, Wall>()
    private val openings = mutableMapOf<ElementId, Opening>()
    private var wallOutlines: List<WallOutline> = emptyList()
    private var openingOutlines: List<OpeningOutline> = emptyList()
    private var allIssues: List<Issue> = emptyList()

    // The points of the walls drawn in a row, so undo steps back along them.
    private val chain = mutableListOf<PointMm>()

    // Whether the finger now on the plan put down the draft's first point.
    private var aimStartedDraft = false

    // One request at a time from send to file, so the log keeps the engine's order.
    private val turn = Mutex()

    init {
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
            state.update { it.copy(takeoffOpen = false, materialOpen = false) }
            return
        }
        when (current.tool) {
            Tool.SELECT -> {
                pick(onLevel, at, reach)
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
     * The finger is on the plan with the wall tool. The first point starts a wall; from then on
     * the wall's end follows the finger, caught by wall ends, the axes and lines through other ends.
     */
    fun aim(
        at: PointMm,
        reach: Mm,
    ) {
        val current = state.value
        if (current.tool != Tool.WALL || current.level == null || current.entry != null || current.measure != null) return
        val draft = current.draft
        if (draft == null) {
            aimStartedDraft = true
            state.update { it.copy(draft = Draft(snap(at, reach, from = null).point), refused = null) }
            return
        }
        val snapped = snap(at, reach, from = draft.start)
        state.update { it.copy(draft = draft.copy(end = snapped.point, guides = snapped.guides, meets = snapped.onWall)) }
    }

    /** A second finger came down: the plan moves instead, and the point is not placed. */
    fun cancelAim() {
        val started = aimStartedDraft
        aimStartedDraft = false
        state.update { current ->
            current.copy(draft = if (started) null else current.draft?.let { Draft(it.start) })
        }
    }

    /** The finger left the plan: builds the aimed wall and starts the next one at its end. */
    fun commit() {
        aimStartedDraft = false
        val current = state.value
        val draft = current.draft ?: return
        val onLevel = current.level ?: return
        val end = draft.end
        if (end == null || end == draft.start) {
            state.update { it.copy(draft = Draft(draft.start)) }
            return
        }
        // A wall that ends on another wall closes the run.
        val closes = draft.meets || end in chain || walls.values.any { it.level == onLevel && (it.start == end || it.end == end) }
        if (chain.isEmpty()) chain += draft.start
        chain += end
        if (closes) chain.clear()
        state.update { it.copy(draft = if (closes) null else Draft(end)) }
        val wall = Wall(onLevel, draft.start, end, thickness = current.thickness, height = WallHeight, unit = current.material)
        send(listOf(EngineCommand.AddWall(wall)), drawing = true)
    }

    /** Ends the run of walls being drawn. */
    fun finishWall() {
        chain.clear()
        state.update { it.copy(draft = null, measure = null) }
    }

    /** Goes on from the run's last point by measured lengths instead of touches. */
    fun startMeasure() = state.update { if (it.draft == null) it else it.copy(measure = Measure(), draft = Draft(it.draft.start)) }

    fun stopMeasure() = state.update { it.copy(measure = null, draft = it.draft?.let { draft -> Draft(draft.start) }) }

    fun setTurn(turn: Turn) = measured { it.copy(turn = turn) }

    /** Builds the measured wall and goes on to the next from its end, turning the same way. */
    fun nextMeasured() {
        val measure = state.value.measure ?: return
        if (state.value.draft?.end == null) return
        commit()
        state.update { it.copy(measure = if (it.draft == null) null else Measure(turn = measure.turn), canCloseRun = chain.size > 2) }
    }

    /** Closes the measured run with a wall back to its first point. */
    fun closeMeasured() {
        val first = chain.firstOrNull() ?: return
        val draft = state.value.draft ?: return
        state.update { it.copy(draft = Draft(draft.start, first), measure = null) }
        commit()
    }

    // Changes the measured wall and shows where it would end.
    private fun measured(change: (Measure) -> Measure) =
        state.update { current ->
            val measure = change(current.measure ?: return@update current)
            val start = current.draft?.start ?: return@update current
            val length = measure.digits.toLongOrNull() ?: 0
            // The wall before gives the direction; the first wall of a run goes along +x.
            val before = chain.getOrNull(chain.lastIndex - 1)?.takeIf { chain.last() == start }
            val bx = if (before == null) 1.0 else (start.x.value - before.x.value).toDouble()
            val by = if (before == null) 0.0 else (start.y.value - before.y.value).toDouble()
            val scale = length / hypot(bx, by)
            // The plan's y runs up, so a left turn is counter-clockwise.
            val (dx, dy) =
                when (measure.turn) {
                    Turn.LEFT -> -by to bx
                    Turn.STRAIGHT -> bx to by
                    Turn.RIGHT -> by to -bx
                }
            val end = PointMm(Mm(start.x.value + (dx * scale).roundToLong()), Mm(start.y.value + (dy * scale).roundToLong()))
            current.copy(measure = measure, draft = Draft(start, end.takeIf { length > 0 }))
        }

    fun toggleOrtho() = state.update { it.copy(ortho = !it.ortho) }

    fun showTakeoff(open: Boolean) = state.update { it.copy(takeoffOpen = open) }

    fun showMaterials(open: Boolean) = state.update { it.copy(materialOpen = open) }

    /**
     * Lays the selected wall, or else the walls drawn from now on, from [option]. A thickness the
     * unit cannot build becomes the nearest one it can, in the same undo step.
     */
    fun chooseMaterial(option: MasonryUnitOption) {
        val current = state.value
        val wall = current.selected as? Selected.WallPart
        val fits = { thickness: Mm -> option.thicknesses.minByOrNull { abs(it.value - thickness.value) } ?: thickness }
        state.update { it.copy(materialOpen = false, material = option.size, thickness = fits(wall?.wall?.thickness ?: it.thickness)) }
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

    /** Chooses the typed size as the material, as [chooseMaterial] does a catalog unit. */
    fun confirmSize() {
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

    fun selectTool(tool: Tool) {
        chain.clear()
        state.update {
            it.copy(
                tool = tool,
                sizeEntry = null,
                draft = null,
                drawn = null,
                measure = null,
                selected = null,
                materialOpen = false,
                refused = null,
            )
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
        if (state.value.entry == null) return measured { it.copy(digits = typed(it.digits)) }
        state.update { current -> current.copy(entry = current.entry?.let { it.copy(digits = typed(it.digits)) }) }
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
        if (state.value.entry == null) return measured { it.copy(digits = it.digits.dropLast(1)) }
        state.update { current -> current.copy(entry = current.entry?.let { it.copy(digits = it.digits.dropLast(1)) }) }
    }

    fun cancelLength() = state.update { it.copy(entry = null) }

    /**
     * Gives the wall the typed length: its start stays, its end moves along the wall, and the ends
     * of other walls that met it there move with it. One undo step.
     */
    fun confirmLength() {
        val entry = state.value.entry ?: return
        state.update { it.copy(entry = null) }
        val wall = walls[entry.wall] ?: return
        val length = entry.digits.toLongOrNull() ?: return
        val dx = (wall.end.x.value - wall.start.x.value).toDouble()
        val dy = (wall.end.y.value - wall.start.y.value).toDouble()
        val current = hypot(dx, dy)
        if (current == 0.0) return
        val to =
            PointMm(
                Mm(wall.start.x.value + (dx * length / current).roundToLong()),
                Mm(wall.start.y.value + (dy * length / current).roundToLong()),
            )
        if (to == wall.end) return
        val followers =
            walls
                .filter { (id, other) -> id != entry.wall && other.level == wall.level }
                .flatMap { (id, other) ->
                    listOfNotNull(
                        EngineCommand.MoveWallEnd(id, WallEnd.START, to).takeIf { other.start == wall.end },
                        EngineCommand.MoveWallEnd(id, WallEnd.END, to).takeIf { other.end == wall.end },
                    )
                }
        // The next wall of the run starts at the moved end.
        if (chain.lastOrNull() == wall.end) chain[chain.lastIndex] = to
        state.update { current -> if (current.draft?.start == wall.end) current.copy(draft = Draft(to)) else current }
        send(gesture(listOf(EngineCommand.MoveWallEnd(entry.wall, WallEnd.END, to)) + followers))
    }

    fun selectThickness(thickness: Mm) = state.update { it.copy(thickness = thickness) }

    fun selectWindowSize(size: OpeningSize) = state.update { it.copy(windowSize = size) }

    fun selectDoorSize(size: OpeningSize) = state.update { it.copy(doorSize = size) }

    fun selectLevel(level: ElementId) {
        if (level in levels) {
            chain.clear()
            state.update { it.copy(refused = null, selected = null, draft = null, drawn = null) }
            show(level)
        }
    }

    /** A storey on top of the highest one, which the user then works on. */
    fun addLevel() {
        val top = levels.values.maxOfOrNull { it.elevation + it.height } ?: 0.mm
        send(listOf(EngineCommand.AddLevel(Level(elevation = top, height = StoreyHeight))), showCreated = true)
    }

    /** Takes one step back; while walls are drawn in a row, the next wall starts one point back. */
    fun undo() {
        if (chain.size >= 2 && state.value.draft?.start == chain.last()) {
            chain.removeAt(chain.lastIndex)
            val back = chain.last()
            if (chain.size == 1) chain.clear()
            state.update {
                it.copy(
                    draft = Draft(back),
                    drawn = null,
                    measure = it.measure?.copy(digits = ""),
                    canCloseRun = chain.size > 2,
                )
            }
        } else {
            state.update { it.copy(drawn = null) }
        }
        send(listOf(EngineCommand.Undo))
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
        viewModelScope.launch {
            turn.withLock {
                val result = engine.send(listOf(EngineCommand.Pick(onLevel, at, reach))).results.singleOrNull()
                val picked = (result as? CommandResult.Done)?.picked
                state.update { it.copy(selected = picked?.let(::selection), refused = null) }
            }
        }
    }

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
        drawing: Boolean = false,
    ) {
        viewModelScope.launch {
            turn.withLock { execute(commands, showCreated, drawing) }
        }
    }

    private suspend fun execute(
        commands: List<EngineCommand>,
        showCreated: Boolean,
        drawing: Boolean,
    ) {
        val response = engine.send(commands)
        job.record(response)
        // Each group in the log is one step: undo takes one back, anything else adds one.
        userSteps += response.log.sumOf { if (it.isUndo) -1 else 1 }
        val created = (response.results.firstOrNull() as? CommandResult.Done)?.created
        apply(response, show = if (showCreated) created else null)
        if (drawing && created != null) state.update { it.copy(drawn = created) }
    }

    // Mirrors the engine's changes, then shows [show], or else stays on the current storey while
    // it exists; when undo removed it, the top storey left.
    private fun apply(
        response: EngineResponse,
        show: ElementId? = null,
    ) {
        for (change in response.changes) {
            when (val element = change.element) {
                is Level -> levels[change.id] = element
                is Wall -> walls[change.id] = element
                is Opening -> openings[change.id] = element
                null -> levels.remove(change.id) ?: walls.remove(change.id) ?: openings.remove(change.id)
            }
        }
        wallOutlines = response.outlines ?: wallOutlines
        openingOutlines = response.openings ?: openingOutlines
        allIssues = response.issues ?: allIssues
        state.update { current ->
            current.copy(
                takeoff =
                    when (val takeoff = response.takeoff) {
                        is Takeoff.Lines -> takeoff.lines
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
            )
        }
        show(show?.takeIf { it in levels } ?: state.value.level?.takeIf { it in levels })
    }

    private fun selection(id: ElementId): Selected? =
        walls[id]?.let { Selected.WallPart(id, it) } ?: openings[id]?.let { Selected.OpeningPart(id, it) }

    private fun show(level: ElementId?) {
        val ordered = orderedLevels()
        val shown = level ?: ordered.lastOrNull()
        val below = ordered.getOrNull(ordered.indexOf(shown) - 1)
        if (shown != state.value.level) chain.clear()
        state.update { current ->
            val sameLevel = shown == current.level
            current.copy(
                levels = ordered,
                level = shown,
                outlines = wallOutlines.filter { it.level == shown },
                dimensions =
                    wallOutlines
                        .filter { it.level == shown }
                        .mapNotNull { outline -> walls[outline.wall]?.let { Dimension(outline.wall, it, outline.length) } },
                openings = openingOutlines.filter { it.level == shown },
                below = if (below == null) emptyList() else wallOutlines.filter { it.level == below },
                issues = allIssues.filter { it.level == shown },
                ringBeam = levels[shown]?.ringBeam ?: 0.mm,
                junctionMeshes = levels[shown]?.junctionMeshes ?: false,
                draft = if (sameLevel) current.draft else null,
                measure = if (sameLevel) current.measure else null,
                selected = if (sameLevel) current.selected else null,
            )
        }
    }

    // By elevation; storeys at one elevation keep the order of their ids, which grow with time.
    private fun orderedLevels(): List<ElementId> =
        levels.entries
            .sortedWith(compareBy<Map.Entry<ElementId, Level>> { it.value.elevation }.thenBy { it.key.uuid.toString() })
            .map { it.key }

    private data class Snapped(
        val point: PointMm,
        val guides: List<Guide> = emptyList(),
        val onWall: Boolean = false,
    )

    // Where a touch lands. Ends join only where points are equal to the millimetre, so a touch
    // within reach of a wall end lands on it; ends on the storey below count too, so walls stand
    // on walls. Otherwise a wall from [from] turns onto the nearer axis (always with the 90° lock,
    // else within a few degrees of it), and each free coordinate lines up with a wall end within
    // reach or falls on the grid. A point within reach of another wall's axis then lands on it,
    // so a partition meets the wall along its length.
    private fun snap(
        at: PointMm,
        reach: Mm,
        from: PointMm?,
    ): Snapped {
        val current = state.value
        val below = current.levels.getOrNull(current.levels.indexOf(current.level) - 1)
        val ends =
            (
                walls.values
                    .filter { it.level == current.level || it.level == below }
                    .flatMap { listOf(it.start, it.end) } + chain
            ).filter { it != from }
        val end = ends.minByOrNull { distance(it, at) }
        if (end != null && distance(end, at) <= reach.value) {
            return Snapped(end)
        }
        val dx = abs(at.x.value - (from?.x?.value ?: 0))
        val dy = abs(at.y.value - (from?.y?.value ?: 0))
        val alongX = (current.ortho || dy * AXIS_CATCH <= dx) && dx >= dy
        val alongY = (current.ortho || dx * AXIS_CATCH <= dy) && dy > dx
        val guides = mutableListOf<Guide>()

        fun line(
            value: Mm,
            of: (PointMm) -> Mm,
        ): Mm {
            val near = ends.minByOrNull { abs(of(it).value - value.value) }
            if (near == null || abs(of(near).value - value.value) > reach.value) return onGrid(value)
            guides += Guide(near, near)
            return of(near)
        }
        val point =
            when {
                from != null && alongX -> PointMm(line(at.x) { it.x }, from.y)
                from != null && alongY -> PointMm(from.x, line(at.y) { it.y })
                else -> PointMm(line(at.x) { it.x }, line(at.y) { it.y })
            }
        val constraint =
            when {
                from != null && alongX -> Axis.X
                from != null && alongY -> Axis.Y
                else -> null
            }
        // A finger on the wall's body is on the wall: the reach counts from its faces.
        val met =
            walls.values
                .filter { it.level == current.level }
                .mapNotNull { wall ->
                    onAxis(wall, at, point, constraint)?.takeIf { distance(it, at) <= reach.value + wall.thickness.value / 2 }
                }.minByOrNull { distance(it, at) }
        if (met != null) {
            return Snapped(met, guides.filter { it.from.x == met.x || it.from.y == met.y }.map { it.copy(to = met) }, onWall = true)
        }
        return Snapped(point, guides.map { it.copy(to = point) })
    }

    private enum class Axis { X, Y }

    // The point of [wall]'s axis, strictly between its ends, where a touch [at] already snapped
    // to [snapped] would meet it: along the [constraint] axis through [snapped] when the wall
    // being drawn is held to one, else the nearest point, keeping the snapped coordinate along a
    // wall that runs along an axis. Null when there is no such point.
    private fun onAxis(
        wall: Wall,
        at: PointMm,
        snapped: PointMm,
        constraint: Axis?,
    ): PointMm? {
        val ux = wall.end.x.value - wall.start.x.value
        val uy = wall.end.y.value - wall.start.y.value
        val inside = { t: Double -> t > 0.0 && t < 1.0 }
        val along = { t: Double ->
            PointMm(Mm(wall.start.x.value + (t * ux).roundToLong()), Mm(wall.start.y.value + (t * uy).roundToLong()))
        }
        return when (constraint) {
            Axis.X -> {
                val t = if (uy == 0L) -1.0 else (snapped.y.value - wall.start.y.value).toDouble() / uy
                if (inside(t)) along(t).copy(y = snapped.y) else null
            }

            Axis.Y -> {
                val t = if (ux == 0L) -1.0 else (snapped.x.value - wall.start.x.value).toDouble() / ux
                if (inside(t)) along(t).copy(x = snapped.x) else null
            }

            null -> {
                val length = (ux * ux + uy * uy).toDouble()
                val keepY = ux == 0L && inside((snapped.y.value - wall.start.y.value).toDouble() / uy)
                val keepX = uy == 0L && inside((snapped.x.value - wall.start.x.value).toDouble() / ux)
                val t = ((at.x.value - wall.start.x.value) * ux + (at.y.value - wall.start.y.value) * uy) / length
                when {
                    keepY -> PointMm(wall.start.x, snapped.y)
                    keepX -> PointMm(snapped.x, wall.start.y)
                    inside(t) -> along(t)
                    else -> null
                }
            }
        }
    }

    private fun distance(
        a: PointMm,
        b: PointMm,
    ): Long = maxOf(abs(a.x.value - b.x.value), abs(a.y.value - b.y.value))

    // The thickness walls start in: the one nearest the middle of what the unit builds.
    private fun usual(thicknesses: List<Mm>): Mm? = thicknesses.getOrNull(thicknesses.size / 2)

    private fun onGrid(value: Mm): Mm = Mm((value.value.toDouble() / Grid.value).roundToLong() * Grid.value)

    private companion object {
        val StoreyHeight = 3000.mm
        val Ground = Level(elevation = 0.mm, height = StoreyHeight)
        val WallHeight = 2800.mm
        val Grid = 100.mm

        // A free wall within about 6° of an axis turns onto it.
        const val AXIS_CATCH = 10

        // Up to 99 999 mm, more than any wall of a house.
        const val MAX_LENGTH_DIGITS = 5

        const val OWN_UNIT = "own"

        // A common sill height for living rooms; the user changes it per window later.
        val WindowSill = 900.mm
    }
}
