package uz.otves.feature.editor

import androidx.compose.runtime.Immutable
import androidx.lifecycle.ViewModel
import androidx.lifecycle.viewModelScope
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.asStateFlow
import kotlinx.coroutines.flow.update
import kotlinx.coroutines.launch
import uz.otves.core.model.ElementId
import uz.otves.core.model.Mm
import uz.otves.core.model.mm
import uz.otves.engine.CommandResult
import uz.otves.engine.Engine
import uz.otves.engine.EngineCommand
import uz.otves.engine.EngineError
import uz.otves.engine.EngineResponse
import uz.otves.engine.Level
import uz.otves.engine.PointMm
import uz.otves.engine.QuantityLine
import uz.otves.engine.Takeoff
import uz.otves.engine.Wall
import uz.otves.engine.WallOutline
import kotlin.math.abs
import kotlin.math.roundToLong

/** Wall thicknesses a builder picks from: half a brick to two bricks with 10 mm joints. */
internal val WallThicknesses: List<Mm> = listOf(120.mm, 250.mm, 380.mm, 510.mm)

@Immutable
internal data class EditorUiState(
    val ready: Boolean = false,
    val outlines: List<WallOutline> = emptyList(),
    val takeoff: List<QuantityLine> = emptyList(),
    val takeoffFailed: Boolean = false,
    /** The first tap of a wall still waiting for its second. */
    val wallStart: PointMm? = null,
    val thickness: Mm = 380.mm,
    val canUndo: Boolean = false,
    val canRedo: Boolean = false,
    /** Why the engine refused the last command, until the next one. */
    val refused: EngineError? = null,
)

/**
 * The plan of one storey. The engine owns the model; this keeps only what the screen draws and
 * the wall ends it snaps to, both taken from engine responses.
 */
internal class EditorViewModel(
    private val engine: Engine,
    loadContent: suspend () -> EngineCommand.LoadContent,
) : ViewModel() {
    private val state = MutableStateFlow(EditorUiState())
    val uiState: StateFlow<EditorUiState> = state.asStateFlow()

    private var level: ElementId? = null

    // The ground floor comes with a new job, so undo stops at the user's first step.
    private var userSteps = 0
    private val walls = mutableMapOf<ElementId, Wall>()

    init {
        viewModelScope.launch {
            val response = engine.send(listOf(loadContent(), EngineCommand.AddLevel(Ground)))
            level = (response.results.getOrNull(1) as? CommandResult.Done)?.created
            apply(response)
            state.update { it.copy(ready = level != null) }
        }
    }

    /** A tap on the plan: the first one starts a wall, the second one builds it. */
    fun tap(at: PointMm) {
        val onLevel = level ?: return
        val point = snap(at)
        val start = state.value.wallStart
        if (start == null) {
            state.update { it.copy(wallStart = point, refused = null) }
            return
        }
        state.update { it.copy(wallStart = null) }
        val wall = Wall(onLevel, start, point, thickness = state.value.thickness, height = WallHeight)
        send(EngineCommand.AddWall(wall))
    }

    fun cancelWall() = state.update { it.copy(wallStart = null) }

    fun selectThickness(thickness: Mm) = state.update { it.copy(thickness = thickness) }

    fun undo() = send(EngineCommand.Undo)

    fun redo() = send(EngineCommand.Redo)

    private fun send(command: EngineCommand) {
        viewModelScope.launch {
            val response = engine.send(listOf(command))
            if (response.results.singleOrNull() is CommandResult.Done) {
                userSteps +=
                    when (command) {
                        EngineCommand.Undo -> -1
                        EngineCommand.Redo, is EngineCommand.AddWall -> 1
                        else -> 0
                    }
            }
            apply(response)
        }
    }

    private fun apply(response: EngineResponse) {
        for (change in response.changes) {
            when (val element = change.element) {
                is Wall -> walls[change.id] = element
                null -> walls.remove(change.id)
                else -> Unit
            }
        }
        state.update { current ->
            current.copy(
                outlines = response.outlines ?: current.outlines,
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
            )
        }
    }

    // Ends join only where points are equal to the millimetre, so a tap near a wall end lands on
    // it; elsewhere it lands on the 100 mm grid.
    private fun snap(at: PointMm): PointMm {
        val end =
            walls.values
                .flatMap { listOf(it.start, it.end) }
                .minByOrNull { distance(it, at) }
        if (end != null && distance(end, at) <= SnapReach.value) {
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
        val Ground = Level(elevation = 0.mm, height = 3000.mm)
        val WallHeight = 2800.mm
        val Grid = 100.mm
        val SnapReach = 300.mm
    }
}
