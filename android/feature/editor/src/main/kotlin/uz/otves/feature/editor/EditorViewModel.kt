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
import uz.otves.engine.CommandResult
import uz.otves.engine.Engine
import uz.otves.engine.EngineCommand
import uz.otves.engine.EngineError
import uz.otves.engine.EngineResponse
import uz.otves.engine.Issue
import uz.otves.engine.Level
import uz.otves.engine.Opening
import uz.otves.engine.OpeningKind
import uz.otves.engine.OpeningOutline
import uz.otves.engine.PointMm
import uz.otves.engine.QuantityLine
import uz.otves.engine.Takeoff
import uz.otves.engine.Wall
import uz.otves.engine.WallOutline
import kotlin.math.abs
import kotlin.math.roundToLong

/** Wall thicknesses a builder picks from: half a brick to two bricks with 10 mm joints. */
internal val WallThicknesses: List<Mm> = listOf(120.mm, 250.mm, 380.mm, 510.mm)

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

@Immutable
internal data class EditorUiState(
    val ready: Boolean = false,
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
    val takeoff: List<QuantityLine> = emptyList(),
    val takeoffFailed: Boolean = false,
    /** The first tap of a wall still waiting for its second. */
    val wallStart: PointMm? = null,
    val tool: Tool = Tool.WALL,
    val selected: Selected? = null,
    val thickness: Mm = 380.mm,
    val windowSize: OpeningSize = WindowSizes[1],
    val doorSize: OpeningSize = DoorSizes[1],
    val canUndo: Boolean = false,
    val canRedo: Boolean = false,
    /** Why the engine refused the last command, until the next one. */
    val refused: EngineError? = null,
)

/**
 * The plan of one job, one storey at a time. The engine owns the model; this keeps only what the
 * screen draws, the wall ends it snaps to and the values of what can be selected, all taken from
 * engine responses, and writes every response to the job file. The view model owns [job] and
 * closes it when cleared.
 */
internal class EditorViewModel(
    private val engine: Engine,
    loadContent: suspend () -> EngineCommand.LoadContent,
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
                state.update { it.copy(ready = levels.isNotEmpty()) }
            }
        }
    }

    override fun onCleared() = job.close()

    /**
     * A tap on the plan. The select tool picks what is under it; with the wall tool the first tap
     * starts a wall and the second builds it; a window or door goes into the wall the user tapped.
     * [reach] is how far from the finger, in plan millimetres, something still counts as touched.
     */
    fun tap(
        at: PointMm,
        reach: Mm,
    ) {
        val current = state.value
        val onLevel = current.level ?: return
        when (current.tool) {
            Tool.SELECT -> pick(onLevel, at, reach)
            Tool.WALL -> tapWall(onLevel, at, reach)
            Tool.WINDOW -> place(onLevel, at, reach, OpeningKind.WINDOW, current.windowSize, WindowSill)
            Tool.DOOR -> place(onLevel, at, reach, OpeningKind.DOOR, current.doorSize, 0.mm)
        }
    }

    fun cancelWall() = state.update { it.copy(wallStart = null) }

    fun selectTool(tool: Tool) = state.update { it.copy(tool = tool, wallStart = null, selected = null, refused = null) }

    fun selectThickness(thickness: Mm) = state.update { it.copy(thickness = thickness) }

    fun selectWindowSize(size: OpeningSize) = state.update { it.copy(windowSize = size) }

    fun selectDoorSize(size: OpeningSize) = state.update { it.copy(doorSize = size) }

    fun selectLevel(level: ElementId) {
        if (level in levels) {
            state.update { it.copy(refused = null, selected = null) }
            show(level)
        }
    }

    /** A storey on top of the highest one, which the user then works on. */
    fun addLevel() {
        val top = levels.values.maxOfOrNull { it.elevation + it.height } ?: 0.mm
        send(listOf(EngineCommand.AddLevel(Level(elevation = top, height = StoreyHeight))), showCreated = true)
    }

    fun undo() = send(listOf(EngineCommand.Undo))

    fun redo() = send(listOf(EngineCommand.Redo))

    fun deselect() = state.update { it.copy(selected = null) }

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

    private fun tapWall(
        onLevel: ElementId,
        at: PointMm,
        reach: Mm,
    ) {
        val point = snap(at, reach)
        val start = state.value.wallStart
        if (start == null) {
            state.update { it.copy(wallStart = point, refused = null) }
            return
        }
        state.update { it.copy(wallStart = null) }
        val wall = Wall(onLevel, start, point, thickness = state.value.thickness, height = WallHeight)
        send(listOf(EngineCommand.AddWall(wall)))
    }

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
    ) {
        viewModelScope.launch {
            turn.withLock { execute(commands, showCreated) }
        }
    }

    private suspend fun execute(
        commands: List<EngineCommand>,
        showCreated: Boolean,
    ) {
        val response = engine.send(commands)
        job.record(response)
        // Each group in the log is one step: undo takes one back, anything else adds one.
        userSteps += response.log.sumOf { if (it.isUndo) -1 else 1 }
        val created = (response.results.firstOrNull() as? CommandResult.Done)?.created
        apply(response, show = if (showCreated) created else null)
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
        state.update { current ->
            val sameLevel = shown == current.level
            current.copy(
                levels = ordered,
                level = shown,
                outlines = wallOutlines.filter { it.level == shown },
                openings = openingOutlines.filter { it.level == shown },
                below = if (below == null) emptyList() else wallOutlines.filter { it.level == below },
                issues = allIssues.filter { it.level == shown },
                wallStart = if (sameLevel) current.wallStart else null,
                selected = if (sameLevel) current.selected else null,
            )
        }
    }

    // By elevation; storeys at one elevation keep the order of their ids, which grow with time.
    private fun orderedLevels(): List<ElementId> =
        levels.entries
            .sortedWith(compareBy<Map.Entry<ElementId, Level>> { it.value.elevation }.thenBy { it.key.uuid.toString() })
            .map { it.key }

    // Ends join only where points are equal to the millimetre, so a tap within reach of a wall
    // end lands on it; elsewhere it lands on the 100 mm grid. Ends on the storey below count too,
    // so walls stand on walls.
    private fun snap(
        at: PointMm,
        reach: Mm,
    ): PointMm {
        val current = state.value
        val below = current.levels.getOrNull(current.levels.indexOf(current.level) - 1)
        val end =
            walls.values
                .filter { it.level == current.level || it.level == below }
                .flatMap { listOf(it.start, it.end) }
                .minByOrNull { distance(it, at) }
        if (end != null && distance(end, at) <= reach.value) {
            return end
        }
        return PointMm(onGrid(at.x), onGrid(at.y))
    }

    private fun distance(
        a: PointMm,
        b: PointMm,
    ): Long = maxOf(abs(a.x.value - b.x.value), abs(a.y.value - b.y.value))

    private fun onGrid(value: Mm): Mm = Mm((value.value.toDouble() / Grid.value).roundToLong() * Grid.value)

    private companion object {
        val StoreyHeight = 3000.mm
        val Ground = Level(elevation = 0.mm, height = StoreyHeight)
        val WallHeight = 2800.mm
        val Grid = 100.mm

        // A common sill height for living rooms; the user changes it per window later.
        val WindowSill = 900.mm
    }
}
