package uz.otves.feature.editor

import androidx.lifecycle.ViewModelProvider
import androidx.lifecycle.ViewModelStore
import androidx.lifecycle.viewmodel.initializer
import androidx.lifecycle.viewmodel.viewModelFactory
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
import uz.otves.data.project.JobStore
import uz.otves.data.project.SavedJob
import uz.otves.engine.CommandResult
import uz.otves.engine.ContentScript
import uz.otves.engine.ElementChange
import uz.otves.engine.Engine
import uz.otves.engine.EngineCommand
import uz.otves.engine.EngineElement
import uz.otves.engine.EngineError
import uz.otves.engine.EngineResponse
import uz.otves.engine.Issue
import uz.otves.engine.IssueKind
import uz.otves.engine.Level
import uz.otves.engine.LogEntry
import uz.otves.engine.Opening
import uz.otves.engine.OpeningKind
import uz.otves.engine.OpeningOutline
import uz.otves.engine.PointM
import uz.otves.engine.PointMm
import uz.otves.engine.QuantityLine
import uz.otves.engine.QuantityUnit
import uz.otves.engine.Takeoff
import uz.otves.engine.Wall
import uz.otves.engine.WallOutline
import uz.otves.schema.CommandKind
import kotlin.uuid.Uuid

@OptIn(ExperimentalCoroutinesApi::class)
class EditorViewModelTest {
    private val level = ElementId(Uuid.parse("019a5f00-0000-7000-8000-000000000001"))
    private val reach = 300.mm
    private val content = EngineCommand.LoadContent(listOf(ContentScript("practice", "return {}")), emptyList())

    /**
     * A small engine: keeps the elements, reports changes, outlines and one log group per request
     * that changed the model, and undoes the last element created. Picks what the test chose.
     */
    private class FakeEngine(
        private val ground: ElementId,
    ) : Engine {
        val sent = mutableListOf<List<EngineCommand>>()
        var refuseNext: EngineError? = null
        var pickNext: ElementId? = null
        var issues: List<Issue> = emptyList()
        private var next = 1L
        private val elements = linkedMapOf<ElementId, EngineElement>()
        private val steps = ArrayDeque<ElementId>()

        override suspend fun send(commands: List<EngineCommand>): EngineResponse {
            sent += commands
            val refusal = refuseNext
            refuseNext = null
            val results = mutableListOf<CommandResult>()
            val changes = mutableListOf<ElementChange>()

            var undone = false

            fun create(
                element: EngineElement,
                id: ElementId = ElementId(Uuid.fromLongs(++next, 0)),
            ) {
                elements[id] = element
                steps.addLast(id)
                results += CommandResult.Done(id)
                changes += ElementChange(id, element)
            }

            fun edit(
                id: ElementId,
                change: (EngineElement) -> EngineElement,
            ) {
                val element = change(elements.getValue(id))
                elements[id] = element
                changes += ElementChange(id, element)
                results += CommandResult.Done(null)
            }

            fun remove(id: ElementId) {
                val hosted = elements.filterValues { (it as? Opening)?.wall == id }.keys
                for (gone in hosted + id) {
                    elements.remove(gone)
                    changes += ElementChange(gone, null)
                }
                results += CommandResult.Done(null)
            }
            for (command in commands) {
                when {
                    refusal != null -> {
                        results += CommandResult.Failed(refusal, "refused")
                    }

                    command is EngineCommand.OpenModel -> {
                        changes += elements.keys.map { ElementChange(it, null) }
                        elements.clear()
                        steps.clear()
                        elements += command.elements
                        changes += command.elements.map { (id, element) -> ElementChange(id, element) }
                        results += CommandResult.Done(null)
                    }

                    command is EngineCommand.AddLevel -> {
                        if (elements.isEmpty()) create(command.level, ground) else create(command.level)
                    }

                    command is EngineCommand.AddWall -> {
                        create(command.wall)
                    }

                    command is EngineCommand.PlaceOpening -> {
                        val wall = elements.entries.firstOrNull { (it.value as? Wall)?.level == command.level }?.key
                        if (wall == null) {
                            results += CommandResult.Failed(EngineError.NO_WALL, "no wall")
                        } else {
                            create(Opening(wall, 0.mm, command.width, command.height, command.sill, command.kind))
                        }
                    }

                    command is EngineCommand.Pick -> {
                        results += CommandResult.Done(null, picked = pickNext)
                    }

                    command is EngineCommand.SetWallThickness -> {
                        edit(command.wall) { (it as Wall).copy(thickness = command.thickness) }
                    }

                    command is EngineCommand.SetOpeningKind -> {
                        edit(command.opening) { (it as Opening).copy(kind = command.kind) }
                    }

                    command is EngineCommand.SetOpeningSill -> {
                        edit(command.opening) { (it as Opening).copy(sill = command.sill) }
                    }

                    command is EngineCommand.MoveOpening -> {
                        edit(command.opening) { (it as Opening).copy(offset = command.offset) }
                    }

                    command is EngineCommand.ResizeOpening -> {
                        edit(command.opening) { (it as Opening).copy(width = command.width, height = command.height) }
                    }

                    command is EngineCommand.RemoveWall -> {
                        remove(command.wall)
                    }

                    command is EngineCommand.RemoveOpening -> {
                        remove(command.opening)
                    }

                    command is EngineCommand.Undo && steps.isNotEmpty() -> {
                        val id = steps.removeLast()
                        elements.remove(id)
                        results += CommandResult.Done(null)
                        changes += ElementChange(id, null)
                        undone = true
                    }

                    else -> {
                        results += CommandResult.Done(null)
                    }
                }
            }
            val changed = changes.isNotEmpty()
            val opened = commands.any { it is EngineCommand.OpenModel }
            val log =
                when {
                    undone -> listOf(group(CommandKind.Undo))
                    changed && !opened -> listOf(group(CommandKind.AddWall))
                    else -> emptyList()
                }
            return EngineResponse(
                results = results,
                changes = changes,
                canUndo = steps.isNotEmpty(),
                canRedo = false,
                outlines = if (changed) wallOutlines() else null,
                openings = if (changed) openingOutlines() else null,
                takeoff =
                    if (refusal == EngineError.RULE_FAILED) {
                        Takeoff.Failed(refusal, "boom")
                    } else {
                        Takeoff.Lines(listOf(QuantityLine("masonry", "masonry_unit", QuantityUnit.PIECE, 100.0)))
                    },
                log = log,
                issues = if (changed) issues else null,
            )
        }

        private fun group(kind: Int) = LogEntry(Uuid.random(), kind, hlc = 0L, device = Uuid.NIL, origin = null, ops = byteArrayOf())

        private fun wallOutlines(): List<WallOutline> =
            elements.mapNotNull { (id, element) ->
                (element as? Wall)?.let { WallOutline(id, it.level, listOf(PointM(0.0, 0.0)), length = it.end.x - it.start.x) }
            }

        private fun openingOutlines(): List<OpeningOutline> =
            elements.mapNotNull { (id, element) ->
                (element as? Opening)?.let {
                    val level = (elements.getValue(it.wall) as Wall).level
                    OpeningOutline(id, level, it.kind, listOf(PointM(0.0, 0.0)))
                }
            }
    }

    /** A job file in memory: what it holds when opened, and every response written to it. */
    private class FakeStore(
        private val saved: SavedJob = SavedJob(emptyMap(), clock = 0L),
    ) : JobStore {
        val recorded = mutableListOf<EngineResponse>()
        var closed = false

        override suspend fun load(): SavedJob = saved

        override suspend fun record(response: EngineResponse) {
            recorded += response
        }

        override fun close() {
            closed = true
        }
    }

    private val engine = FakeEngine(level)
    private var store = FakeStore()

    @Before
    fun setUp() = Dispatchers.setMain(UnconfinedTestDispatcher())

    @After
    fun tearDown() = Dispatchers.resetMain()

    @Test
    fun aNewJobOpensEmptyAndGetsAGroundFloor() {
        val viewModel = EditorViewModel(engine, { content }, store)

        val start = engine.sent.single()
        assertThat(start[0]).isEqualTo(content)
        assertThat(start[1]).isEqualTo(EngineCommand.OpenModel(emptyMap(), clock = 0L))
        assertThat(start[2]).isInstanceOf(EngineCommand.AddLevel::class.java)
        assertThat(store.recorded).hasSize(1)
        assertThat(viewModel.uiState.value.ready).isTrue()
        assertThat(viewModel.uiState.value.level).isEqualTo(level)
    }

    @Test
    fun aSavedJobOpensAsSavedOnItsGroundFloor() {
        val upper = ElementId(Uuid.parse("019a5f00-0000-7000-8000-000000000010"))
        val wall = ElementId(Uuid.parse("019a5f00-0000-7000-8000-000000000011"))
        val saved =
            mapOf(
                level to Level(0.mm, 3_000.mm),
                upper to Level(3_000.mm, 3_000.mm),
                wall to Wall(level, PointMm(0.mm, 0.mm), PointMm(6_000.mm, 0.mm), 380.mm, 2_800.mm),
            )
        store = FakeStore(SavedJob(saved, clock = 42L, name = "Дом на Чиланзаре"))

        val viewModel = EditorViewModel(engine, { content }, store)

        assertThat(engine.sent.single()).containsExactly(content, EngineCommand.OpenModel(saved, clock = 42L)).inOrder()
        val state = viewModel.uiState.value
        assertThat(state.levels).containsExactly(level, upper).inOrder()
        assertThat(state.level).isEqualTo(level)
        assertThat(state.outlines).hasSize(1)
        assertThat(state.canUndo).isFalse()
        assertThat(state.name).isEqualTo("Дом на Чиланзаре")
    }

    @Test
    fun leavingTheEditorClosesTheJob() {
        val owner = ViewModelStore()
        val factory = viewModelFactory { initializer { EditorViewModel(engine, { content }, store) } }
        ViewModelProvider.create(owner, factory)[EditorViewModel::class]

        owner.clear()

        assertThat(store.closed).isTrue()
    }

    @Test
    fun everyResponseGoesToTheJobFile() {
        val viewModel = EditorViewModel(engine, { content }, store)

        viewModel.tap(PointMm(0.mm, 0.mm), reach)
        viewModel.tap(PointMm(6_000.mm, 0.mm), reach)
        viewModel.undo()

        assertThat(store.recorded).hasSize(3)
        assertThat(
            store.recorded
                .last()
                .changes
                .single()
                .element,
        ).isNull()
    }

    @Test
    fun twoTapsBuildAWallOnTheGridWithTheChosenThickness() {
        val viewModel = EditorViewModel(engine, { content }, store)
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
        val viewModel = EditorViewModel(engine, { content }, store)
        assertThat(viewModel.uiState.value.canUndo).isFalse()

        viewModel.tap(PointMm(0.mm, 0.mm), reach)
        viewModel.tap(PointMm(4_000.mm, 0.mm), reach)
        assertThat(viewModel.uiState.value.canUndo).isTrue()

        viewModel.undo()
        assertThat(viewModel.uiState.value.canUndo).isFalse()
    }

    @Test
    fun aTapNearAWallEndLandsOnIt() {
        val viewModel = EditorViewModel(engine, { content }, store)
        viewModel.tap(PointMm(0.mm, 0.mm), reach)
        viewModel.tap(PointMm(6_437.mm, 0.mm), reach)

        viewModel.tap(PointMm(6_250.mm, 180.mm), reach)

        assertThat(viewModel.uiState.value.wallStart).isEqualTo(PointMm(6_400.mm, 0.mm))
    }

    @Test
    fun aWindowGoesIntoTheTappedWallWithTheChosenSize() {
        val viewModel = EditorViewModel(engine, { content }, store)
        viewModel.tap(PointMm(0.mm, 0.mm), reach)
        viewModel.tap(PointMm(6_000.mm, 0.mm), reach)
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
        val viewModel = EditorViewModel(engine, { content }, store)
        viewModel.selectTool(Tool.DOOR)

        viewModel.tap(PointMm(1_000.mm, 0.mm), reach)

        val place = engine.sent.last().single() as EngineCommand.PlaceOpening
        assertThat(place.sill).isEqualTo(0.mm)
        assertThat(place.width).isEqualTo(900.mm)
        assertThat(place.kind).isEqualTo(OpeningKind.DOOR)
    }

    @Test
    fun changingTheToolDropsAWallStart() {
        val viewModel = EditorViewModel(engine, { content }, store)
        viewModel.tap(PointMm(0.mm, 0.mm), reach)

        viewModel.selectTool(Tool.WINDOW)

        assertThat(viewModel.uiState.value.wallStart).isNull()
    }

    @Test
    fun aNewStoreyStandsOnTheTopOneAndIsShownOverIt() {
        val viewModel = EditorViewModel(engine, { content }, store)
        viewModel.tap(PointMm(0.mm, 0.mm), reach)
        viewModel.tap(PointMm(6_000.mm, 0.mm), reach)

        viewModel.addLevel()

        val add = engine.sent.last().single() as EngineCommand.AddLevel
        assertThat(add.level).isEqualTo(Level(elevation = 3_000.mm, height = 3_000.mm))
        val state = viewModel.uiState.value
        assertThat(state.levels).hasSize(2)
        assertThat(state.levels.first()).isEqualTo(level)
        assertThat(state.level).isEqualTo(state.levels.last())
        assertThat(state.outlines).isEmpty()
        assertThat(state.below).hasSize(1)
        assertThat(state.canUndo).isTrue()
    }

    @Test
    fun choosingAStoreyShowsItsWalls() {
        val viewModel = EditorViewModel(engine, { content }, store)
        viewModel.tap(PointMm(0.mm, 0.mm), reach)
        viewModel.tap(PointMm(6_000.mm, 0.mm), reach)
        viewModel.addLevel()

        viewModel.selectLevel(level)

        val state = viewModel.uiState.value
        assertThat(state.level).isEqualTo(level)
        assertThat(state.outlines).hasSize(1)
        assertThat(state.below).isEmpty()
    }

    @Test
    fun undoingAStoreyGoesBackDown() {
        val viewModel = EditorViewModel(engine, { content }, store)
        viewModel.addLevel()

        viewModel.undo()

        assertThat(viewModel.uiState.value.levels).containsExactly(level)
        assertThat(viewModel.uiState.value.level).isEqualTo(level)
        assertThat(viewModel.uiState.value.canUndo).isFalse()
    }

    @Test
    fun aWallEndOnTheStoreyBelowCatchesTaps() {
        val viewModel = EditorViewModel(engine, { content }, store)
        viewModel.tap(PointMm(0.mm, 0.mm), reach)
        viewModel.tap(PointMm(6_437.mm, 0.mm), reach)
        viewModel.addLevel()

        viewModel.tap(PointMm(6_250.mm, 180.mm), reach)

        assertThat(viewModel.uiState.value.wallStart).isEqualTo(PointMm(6_400.mm, 0.mm))
    }

    // A 6 m wall on the ground floor; returns its id.
    private fun EditorViewModel.buildWall(): ElementId {
        tap(PointMm(0.mm, 0.mm), reach)
        tap(PointMm(6_000.mm, 0.mm), reach)
        return uiState.value.outlines
            .last()
            .wall
    }

    @Test
    fun theSelectToolShowsWhatTheEnginePicked() {
        val viewModel = EditorViewModel(engine, { content }, store)
        val wall = viewModel.buildWall()
        viewModel.selectTool(Tool.SELECT)

        engine.pickNext = wall
        viewModel.tap(PointMm(3_000.mm, 100.mm), reach)

        assertThat(engine.sent.last().single()).isEqualTo(EngineCommand.Pick(level, PointMm(3_000.mm, 100.mm), reach))
        val selected = viewModel.uiState.value.selected as Selected.WallPart
        assertThat(selected.id).isEqualTo(wall)
        assertThat(selected.wall.thickness).isEqualTo(380.mm)

        engine.pickNext = null
        viewModel.tap(PointMm(3_000.mm, 3_000.mm), reach)
        assertThat(viewModel.uiState.value.selected).isNull()
    }

    @Test
    fun theInspectorChangesTheSelectedWall() {
        val viewModel = EditorViewModel(engine, { content }, store)
        val wall = viewModel.buildWall()
        viewModel.selectTool(Tool.SELECT)
        engine.pickNext = wall
        viewModel.tap(PointMm(3_000.mm, 0.mm), reach)

        viewModel.setWallThickness(250.mm)

        assertThat(engine.sent.last().single()).isEqualTo(EngineCommand.SetWallThickness(wall, 250.mm))
        assertThat((viewModel.uiState.value.selected as Selected.WallPart).wall.thickness).isEqualTo(250.mm)
    }

    @Test
    fun anotherKindIsOneStepWithItsUsualSizeAndSillAboutTheSameCentre() {
        val viewModel = EditorViewModel(engine, { content }, store)
        viewModel.buildWall()
        viewModel.selectTool(Tool.WINDOW)
        viewModel.tap(PointMm(3_000.mm, 0.mm), reach)
        val window =
            viewModel.uiState.value.openings
                .single()
                .opening
        viewModel.selectTool(Tool.SELECT)
        engine.pickNext = window
        viewModel.tap(PointMm(3_000.mm, 0.mm), reach)

        viewModel.setOpeningKind(OpeningKind.DOOR)

        // The window was 1 500 mm wide at offset 0; a 900 mm door keeps its centre at 750 mm.
        assertThat(engine.sent.last())
            .containsExactly(
                EngineCommand.BeginGesture,
                EngineCommand.SetOpeningKind(window, OpeningKind.DOOR),
                EngineCommand.SetOpeningSill(window, 0.mm),
                EngineCommand.MoveOpening(window, 300.mm),
                EngineCommand.ResizeOpening(window, 900.mm, 2_100.mm),
                EngineCommand.EndGesture,
            ).inOrder()
        val door = (viewModel.uiState.value.selected as Selected.OpeningPart).opening
        assertThat(door.kind).isEqualTo(OpeningKind.DOOR)
        assertThat(door.width).isEqualTo(900.mm)
    }

    @Test
    fun removingTheSelectionDropsIt() {
        val viewModel = EditorViewModel(engine, { content }, store)
        val wall = viewModel.buildWall()
        viewModel.selectTool(Tool.SELECT)
        engine.pickNext = wall
        viewModel.tap(PointMm(3_000.mm, 0.mm), reach)

        viewModel.removeSelected()

        assertThat(engine.sent.last().single()).isEqualTo(EngineCommand.RemoveWall(wall))
        assertThat(viewModel.uiState.value.selected).isNull()
        assertThat(viewModel.uiState.value.outlines).isEmpty()
        assertThat(viewModel.uiState.value.canUndo).isTrue()
    }

    @Test
    fun eachWallOfTheStoreyGetsItsLength() {
        val viewModel = EditorViewModel(engine, { content }, store)

        viewModel.buildWall()

        val dimension =
            viewModel.uiState.value.dimensions
                .single()
        assertThat(dimension.wall.end).isEqualTo(PointMm(6_000.mm, 0.mm))
        assertThat(dimension.length).isEqualTo(6_000.mm)
    }

    @Test
    fun onlyTheIssuesOfTheShownStoreyAreDrawn() {
        val viewModel = EditorViewModel(engine, { content }, store)
        val here = Issue(IssueKind.WALL_OVERLAP, element = null, level, PointMm(0.mm, 0.mm))
        val upstairs = Issue(IssueKind.SHARP_ANGLE, element = null, ElementId(Uuid.fromLongs(9, 9)), PointMm(0.mm, 0.mm))
        engine.issues = listOf(here, upstairs)

        viewModel.buildWall()

        assertThat(viewModel.uiState.value.issues).containsExactly(here)
    }

    @Test
    fun refusalAndFailedTakeoffAreShown() {
        val viewModel = EditorViewModel(engine, { content }, store)
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
