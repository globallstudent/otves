package uz.otves.feature.editor

import com.google.common.truth.Truth.assertThat
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.ExperimentalCoroutinesApi
import kotlinx.coroutines.test.UnconfinedTestDispatcher
import kotlinx.coroutines.test.resetMain
import kotlinx.coroutines.test.setMain
import org.junit.After
import org.junit.Before
import org.junit.Test
import uz.otves.core.model.ElementId
import uz.otves.core.model.mm
import uz.otves.engine.CommandResult
import uz.otves.engine.ContentScript
import uz.otves.engine.ElementChange
import uz.otves.engine.Engine
import uz.otves.engine.EngineCommand
import uz.otves.engine.EngineError
import uz.otves.engine.EngineResponse
import uz.otves.engine.Opening
import uz.otves.engine.OpeningKind
import uz.otves.engine.OpeningOutline
import uz.otves.engine.PointM
import uz.otves.engine.PointMm
import uz.otves.engine.QuantityLine
import uz.otves.engine.QuantityUnit
import uz.otves.engine.Takeoff
import uz.otves.engine.WallOutline
import kotlin.uuid.Uuid

@OptIn(ExperimentalCoroutinesApi::class)
class EditorViewModelTest {
    private val level = ElementId(Uuid.parse("019a5f00-0000-7000-8000-000000000001"))
    private val reach = 300.mm
    private val content = EngineCommand.LoadContent(listOf(ContentScript("practice", "return {}")), emptyList())

    /** Answers like the engine: the level on start, a new wall for every AddWall. */
    private class FakeEngine(
        private val level: ElementId,
    ) : Engine {
        val sent = mutableListOf<List<EngineCommand>>()
        var refuseNext: EngineError? = null
        private var walls = 0L

        // Like the engine's: the ground floor is on the stack too.
        private var undoable = 0

        override suspend fun send(commands: List<EngineCommand>): EngineResponse {
            sent += commands
            val refusal = refuseNext
            refuseNext = null
            val results = mutableListOf<CommandResult>()
            val changes = mutableListOf<ElementChange>()
            for (command in commands) {
                when {
                    refusal != null -> {
                        results += CommandResult.Failed(refusal, "refused")
                    }

                    command is EngineCommand.AddLevel -> {
                        undoable++
                        results += CommandResult.Done(level)
                    }

                    command is EngineCommand.Undo -> {
                        undoable--
                        results += CommandResult.Done(null)
                    }

                    command is EngineCommand.PlaceOpening -> {
                        undoable++
                        val id = ElementId(Uuid.fromLongs(++walls, 1))
                        results += CommandResult.Done(id)
                        changes += ElementChange(id, Opening(level, 0.mm, command.width, command.height, command.sill, command.kind))
                    }

                    command is EngineCommand.AddWall -> {
                        undoable++
                        val id = ElementId(Uuid.fromLongs(++walls, 0))
                        results += CommandResult.Done(id)
                        changes += ElementChange(id, command.wall)
                    }

                    else -> {
                        results += CommandResult.Done(null)
                    }
                }
            }
            return EngineResponse(
                results = results,
                changes = changes,
                canUndo = undoable > 0,
                canRedo = false,
                outlines = if (changes.isEmpty()) null else listOf(WallOutline(changes.first().id, level, listOf(PointM(0.0, 0.0)))),
                openings =
                    changes
                        .filter { it.element is Opening }
                        .map { OpeningOutline(it.id, level, (it.element as Opening).kind, listOf(PointM(0.0, 0.0))) },
                takeoff =
                    if (refusal == EngineError.RULE_FAILED) {
                        Takeoff.Failed(refusal, "boom")
                    } else {
                        Takeoff.Lines(listOf(QuantityLine("masonry", "masonry_unit", QuantityUnit.PIECE, 100.0)))
                    },
            )
        }
    }

    private val engine = FakeEngine(level)

    @Before
    fun setUp() = Dispatchers.setMain(UnconfinedTestDispatcher())

    @After
    fun tearDown() = Dispatchers.resetMain()

    @Test
    fun startLoadsTheContentAndAGroundFloor() {
        val viewModel = EditorViewModel(engine) { content }

        assertThat(engine.sent.single().first()).isEqualTo(content)
        assertThat(engine.sent.single()[1]).isInstanceOf(EngineCommand.AddLevel::class.java)
        assertThat(viewModel.uiState.value.ready).isTrue()
    }

    @Test
    fun twoTapsBuildAWallOnTheGridWithTheChosenThickness() {
        val viewModel = EditorViewModel(engine) { content }
        viewModel.selectThickness(250.mm)

        viewModel.tap(PointMm(30.mm, (-40).mm), reach)
        assertThat(viewModel.uiState.value.wallStart).isEqualTo(PointMm(0.mm, 0.mm))
        viewModel.tap(PointMm(6_420.mm, 60.mm), reach)

        val add = engine.sent.last().single() as EngineCommand.AddWall
        assertThat(add.wall.level).isEqualTo(level)
        assertThat(add.wall.start).isEqualTo(PointMm(0.mm, 0.mm))
        assertThat(add.wall.end).isEqualTo(PointMm(6_400.mm, 100.mm))
        assertThat(add.wall.thickness).isEqualTo(250.mm)
        val state = viewModel.uiState.value
        assertThat(state.wallStart).isNull()
        assertThat(state.outlines).hasSize(1)
        assertThat(state.takeoff.single().amount).isEqualTo(100.0)
        assertThat(state.canUndo).isTrue()
    }

    @Test
    fun undoStopsBeforeTheGroundFloor() {
        val viewModel = EditorViewModel(engine) { content }
        assertThat(viewModel.uiState.value.canUndo).isFalse()

        viewModel.tap(PointMm(0.mm, 0.mm), reach)
        viewModel.tap(PointMm(4_000.mm, 0.mm), reach)
        assertThat(viewModel.uiState.value.canUndo).isTrue()

        viewModel.undo()
        assertThat(viewModel.uiState.value.canUndo).isFalse()
    }

    @Test
    fun aTapNearAWallEndLandsOnIt() {
        val viewModel = EditorViewModel(engine) { content }
        viewModel.tap(PointMm(0.mm, 0.mm), reach)
        viewModel.tap(PointMm(6_437.mm, 0.mm), reach)

        viewModel.tap(PointMm(6_250.mm, 180.mm), reach)

        assertThat(viewModel.uiState.value.wallStart).isEqualTo(PointMm(6_400.mm, 0.mm))
    }

    @Test
    fun aWindowGoesIntoTheTappedWallWithTheChosenSize() {
        val viewModel = EditorViewModel(engine) { content }
        viewModel.selectTool(Tool.WINDOW)
        viewModel.selectWindowSize(WindowSizes[2])

        viewModel.tap(PointMm(3_000.mm, 150.mm), reach)

        val place = engine.sent.last().single() as EngineCommand.PlaceOpening
        assertThat(place.level).isEqualTo(level)
        assertThat(place.at).isEqualTo(PointMm(3_000.mm, 150.mm))
        assertThat(place.reach).isEqualTo(reach)
        assertThat(place.width).isEqualTo(1_800.mm)
        assertThat(place.height).isEqualTo(1_500.mm)
        assertThat(place.sill).isEqualTo(900.mm)
        assertThat(place.kind).isEqualTo(OpeningKind.WINDOW)
        assertThat(
            viewModel.uiState.value.openings
                .single()
                .kind,
        ).isEqualTo(OpeningKind.WINDOW)
        assertThat(viewModel.uiState.value.canUndo).isTrue()
    }

    @Test
    fun aDoorStandsOnTheFloor() {
        val viewModel = EditorViewModel(engine) { content }
        viewModel.selectTool(Tool.DOOR)

        viewModel.tap(PointMm(1_000.mm, 0.mm), reach)

        val place = engine.sent.last().single() as EngineCommand.PlaceOpening
        assertThat(place.sill).isEqualTo(0.mm)
        assertThat(place.width).isEqualTo(900.mm)
        assertThat(place.kind).isEqualTo(OpeningKind.DOOR)
    }

    @Test
    fun changingTheToolDropsAWallStart() {
        val viewModel = EditorViewModel(engine) { content }
        viewModel.tap(PointMm(0.mm, 0.mm), reach)

        viewModel.selectTool(Tool.WINDOW)

        assertThat(viewModel.uiState.value.wallStart).isNull()
    }

    @Test
    fun refusalAndFailedTakeoffAreShown() {
        val viewModel = EditorViewModel(engine) { content }
        viewModel.tap(PointMm(0.mm, 0.mm), reach)

        engine.refuseNext = EngineError.ZERO_LENGTH
        viewModel.tap(PointMm(0.mm, 0.mm), reach)
        assertThat(viewModel.uiState.value.refused).isEqualTo(EngineError.ZERO_LENGTH)

        engine.refuseNext = EngineError.RULE_FAILED
        viewModel.undo()
        assertThat(viewModel.uiState.value.takeoffFailed).isTrue()
        assertThat(viewModel.uiState.value.takeoff).isEmpty()
    }
}
