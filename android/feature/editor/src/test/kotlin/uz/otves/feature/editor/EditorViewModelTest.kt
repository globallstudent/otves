package uz.otves.feature.editor

import androidx.lifecycle.ViewModelProvider
import androidx.lifecycle.ViewModelStore
import androidx.lifecycle.viewmodel.initializer
import androidx.lifecycle.viewmodel.viewModelFactory
import com.google.common.truth.Truth.assertThat
import kotlinx.coroutines.CompletableDeferred
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.ExperimentalCoroutinesApi
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.test.UnconfinedTestDispatcher
import kotlinx.coroutines.test.resetMain
import kotlinx.coroutines.test.setMain
import org.junit.After
import org.junit.Before
import org.junit.Test
import uz.otves.core.model.Currency
import uz.otves.core.model.ElementId
import uz.otves.core.model.Mm
import uz.otves.core.model.Money
import uz.otves.core.model.mm
import uz.otves.data.catalog.Prices
import uz.otves.data.project.JobStore
import uz.otves.data.project.SavedJob
import uz.otves.engine.Aimed
import uz.otves.engine.Catalog
import uz.otves.engine.CommandResult
import uz.otves.engine.Condition
import uz.otves.engine.Conditions
import uz.otves.engine.ContentScript
import uz.otves.engine.ElementChange
import uz.otves.engine.Engine
import uz.otves.engine.EngineCommand
import uz.otves.engine.EngineElement
import uz.otves.engine.EngineError
import uz.otves.engine.EngineResponse
import uz.otves.engine.Footing
import uz.otves.engine.Guide
import uz.otves.engine.Issue
import uz.otves.engine.IssueKind
import uz.otves.engine.Level
import uz.otves.engine.LogEntry
import uz.otves.engine.MasonryUnitKind
import uz.otves.engine.MasonryUnitOption
import uz.otves.engine.Opening
import uz.otves.engine.OpeningKind
import uz.otves.engine.OpeningOutline
import uz.otves.engine.PointM
import uz.otves.engine.PointMm
import uz.otves.engine.QuantityLine
import uz.otves.engine.QuantityUnit
import uz.otves.engine.Takeoff
import uz.otves.engine.Turn
import uz.otves.engine.UnitSize
import uz.otves.engine.Wall
import uz.otves.engine.WallEnd
import uz.otves.engine.WallOutline
import uz.otves.schema.CommandKind
import kotlin.math.abs
import kotlin.math.sign
import kotlin.uuid.Uuid

@OptIn(ExperimentalCoroutinesApi::class)
class EditorViewModelTest {
    private val level = ElementId(Uuid.parse("019a5f00-0000-7000-8000-000000000001"))
    private val reach = 300.mm
    private val content = EngineCommand.LoadContent(listOf(ContentScript("practice", "return {}")), emptyList())
    private val brick = UnitSize(250.mm, 120.mm, 65.mm)
    private val block = UnitSize(390.mm, 190.mm, 188.mm)
    private val catalog =
        Catalog(
            units =
                listOf(
                    MasonryUnitOption("brick.1nf", MasonryUnitKind.BRICK, "1НФ", brick, listOf(120.mm, 250.mm, 380.mm, 510.mm, 640.mm)),
                    MasonryUnitOption("block", MasonryUnitKind.BLOCK, "", block, listOf(190.mm, 390.mm)),
                ),
            ringBeamHeights = listOf(150.mm, 200.mm),
        )

    /**
     * A small engine: keeps the elements, reports changes, outlines and one log group per request
     * that changed the model, and undoes its last step. Picks what the test chose. It knows no
     * geometry (engine/geom has the real thing and its tests): an aim answers the finger's own
     * point unless the test says otherwise, and measured walls and typed lengths run along the
     * axes only.
     */
    private class FakeEngine(
        private val ground: ElementId,
    ) : Engine {
        val sent = mutableListOf<List<EngineCommand>>()
        var refuseNext: EngineError? = null
        var pickNext: ElementId? = null
        var issues: List<Issue> = emptyList()

        /** What the next aim answers instead of the finger's own point. */
        var aimNext: Aimed? = null

        /** Requests wait for it, as they wait for the engine thread. */
        var hold: CompletableDeferred<Unit>? = null
        private var next = 1L
        private val elements = linkedMapOf<ElementId, EngineElement>()

        // What undoes each step, returning the changes it makes.
        private val steps = ArrayDeque<() -> List<ElementChange>>()

        private fun endsAt(point: PointMm): Boolean = elements.values.any { it is Wall && (it.start == point || it.end == point) }

        private fun along(
            from: PointMm,
            dx: Int,
            dy: Int,
            length: Long,
        ) = PointMm(from.x + Mm(dx * length), from.y + Mm(dy * length))

        override suspend fun send(commands: List<EngineCommand>): EngineResponse {
            sent += commands
            hold?.await()
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
                steps.addLast {
                    elements.remove(id)
                    listOf(ElementChange(id, null))
                }
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

                    command is EngineCommand.Aim -> {
                        val from = command.from ?: command.at
                        val length = abs((command.at.x - from.x).value) + abs((command.at.y - from.y).value)
                        results += CommandResult.Done(null, aimed = aimNext ?: Aimed(command.at, Mm(length), meets = endsAt(command.at)))
                        aimNext = null
                    }

                    command is EngineCommand.AimMeasured -> {
                        // The wall before gives the heading; a left turn is counter-clockwise.
                        val hx = command.before?.let { (command.from.x - it.x).value.sign } ?: 1
                        val hy = command.before?.let { (command.from.y - it.y).value.sign } ?: 0
                        val point =
                            when (command.turn) {
                                Turn.LEFT -> along(command.from, -hy, hx, command.length.value)
                                Turn.STRAIGHT -> along(command.from, hx, hy, command.length.value)
                                Turn.RIGHT -> along(command.from, hy, -hx, command.length.value)
                            }
                        results += CommandResult.Done(null, aimed = Aimed(point, command.length, meets = endsAt(point)))
                    }

                    command is EngineCommand.SetWallLength -> {
                        // The wall's end moves, with the ends of other walls that meet it there.
                        val wall = elements.getValue(command.wall) as Wall
                        val to =
                            along(
                                wall.start,
                                (wall.end.x - wall.start.x).value.sign,
                                (wall.end.y - wall.start.y).value.sign,
                                command.length.value,
                            )
                        val before = elements.filterValues { it is Wall && (it.end == wall.end || it.start == wall.end) }
                        for ((id, element) in before) {
                            val joined = element as Wall
                            val moved =
                                joined.copy(
                                    start = if (joined.start == wall.end) to else joined.start,
                                    end = if (joined.end == wall.end) to else joined.end,
                                )
                            elements[id] = moved
                            changes += ElementChange(id, moved)
                        }
                        steps.addLast {
                            elements += before
                            before.map { (id, element) -> ElementChange(id, element) }
                        }
                        results += CommandResult.Done(null)
                    }

                    command is EngineCommand.MoveWallEnd -> {
                        edit(command.wall) {
                            val wall = it as Wall
                            if (command.end == WallEnd.START) wall.copy(start = command.to) else wall.copy(end = command.to)
                        }
                    }

                    command is EngineCommand.SetWallUnit -> {
                        edit(command.wall) { (it as Wall).copy(unit = command.unit) }
                    }

                    command is EngineCommand.SetLevelRingBeam -> {
                        edit(command.level) { (it as Level).copy(ringBeam = command.height) }
                    }

                    command is EngineCommand.SetLevelFooting -> {
                        edit(command.level) { (it as Level).copy(footing = command.footing) }
                    }

                    command is EngineCommand.SetLevelJunctionMeshes -> {
                        edit(command.level) { (it as Level).copy(junctionMeshes = command.meshes) }
                    }

                    command is EngineCommand.SetCondition -> {
                        val id = elements.entries.firstOrNull { it.value is Conditions }?.key
                        val now = (id?.let { elements[it] } as? Conditions ?: Conditions()).copy(cementGrade = command.value)
                        if (id == null) create(now) else edit(id) { now }
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
                        changes += steps.removeLast()()
                        results += CommandResult.Done(null)
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

    /** A price list in memory. */
    private class FakePrices : Prices {
        override val all = MutableStateFlow(emptyMap<String, Money>())

        override suspend fun set(
            item: String,
            price: Money?,
        ) {
            all.value = if (price == null) all.value - item else all.value + (item to price)
        }
    }

    private val prices = FakePrices()
    private val engine = FakeEngine(level)
    private var store = FakeStore()

    @Before
    fun setUp() = Dispatchers.setMain(UnconfinedTestDispatcher())

    @After
    fun tearDown() = Dispatchers.resetMain()

    @Test
    fun aNewJobOpensEmptyAndGetsAGroundFloor() {
        val viewModel = EditorViewModel(engine, { content }, { catalog }, store, prices)

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

        val viewModel = EditorViewModel(engine, { content }, { catalog }, store, prices)

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
        val factory = viewModelFactory { initializer { EditorViewModel(engine, { content }, { catalog }, store, prices) } }
        ViewModelProvider.create(owner, factory)[EditorViewModel::class]

        owner.clear()

        assertThat(store.closed).isTrue()
    }

    @Test
    fun everyResponseGoesToTheJobFile() {
        val viewModel = EditorViewModel(engine, { content }, { catalog }, store, prices)

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
    fun twoTapsBuildAWallWhereTheEngineAimsAndStartTheNextAtItsEnd() {
        val viewModel = EditorViewModel(engine, { content }, { catalog }, store, prices)
        viewModel.selectThickness(250.mm)

        viewModel.tap(PointMm(0.mm, 0.mm), reach)
        assertThat(viewModel.uiState.value.draft).isEqualTo(Draft(PointMm(0.mm, 0.mm)))
        engine.aimNext = Aimed(PointMm(6_400.mm, 0.mm), length = 6_400.mm)
        viewModel.tap(PointMm(6_420.mm, 260.mm), reach)

        assertThat(engine.sent.flatten().filterIsInstance<EngineCommand.Aim>())
            .containsExactly(
                EngineCommand.Aim(level, from = null, at = PointMm(0.mm, 0.mm), reach = reach, grid = 100.mm, square = true),
                EngineCommand.Aim(level, PointMm(0.mm, 0.mm), PointMm(6_420.mm, 260.mm), reach, grid = 100.mm, square = true),
            ).inOrder()
        val add = engine.sent.last().single() as EngineCommand.AddWall
        assertThat(add.wall.level).isEqualTo(level)
        assertThat(add.wall.start).isEqualTo(PointMm(0.mm, 0.mm))
        assertThat(add.wall.end).isEqualTo(PointMm(6_400.mm, 0.mm))
        assertThat(add.wall.thickness).isEqualTo(250.mm)
        val state = viewModel.uiState.value
        assertThat(state.draft).isEqualTo(Draft(PointMm(6_400.mm, 0.mm)))
        assertThat(state.drawn).isEqualTo(state.outlines.single().wall)
        assertThat(state.outlines).hasSize(1)
        assertThat(state.takeoff.single().amount).isEqualTo(100.0)
        assertThat(state.canUndo).isTrue()
    }

    @Test
    fun undoStopsBeforeTheGroundFloor() {
        val viewModel = EditorViewModel(engine, { content }, { catalog }, store, prices)
        assertThat(viewModel.uiState.value.canUndo).isFalse()

        viewModel.tap(PointMm(0.mm, 0.mm), reach)
        viewModel.tap(PointMm(4_000.mm, 0.mm), reach)
        assertThat(viewModel.uiState.value.canUndo).isTrue()

        viewModel.undo()
        assertThat(viewModel.uiState.value.canUndo).isFalse()
    }

    @Test
    fun theWallBeingAimedIsShownAsTheEngineSeesIt() {
        val viewModel = EditorViewModel(engine, { content }, { catalog }, store, prices)
        viewModel.toggleOrtho()
        viewModel.tap(PointMm(0.mm, 0.mm), reach)
        val corner = PointMm(4_000.mm, 3_000.mm)
        val aimed = Aimed(corner, length = 5_000.mm, guides = listOf(Guide(PointMm(4_000.mm, 0.mm), corner)))
        engine.aimNext = aimed

        viewModel.aim(PointMm(4_020.mm, 2_980.mm), reach)

        assertThat(viewModel.uiState.value.draft).isEqualTo(Draft(PointMm(0.mm, 0.mm), aimed))
        // Without the 90° lock the engine is asked for a wall at any angle.
        assertThat((engine.sent.last().single() as EngineCommand.Aim).square).isFalse()
        assertThat(viewModel.uiState.value.outlines).isEmpty()
    }

    @Test
    fun aMovingFingerAsksOnlyForWhereItIsNow() {
        val viewModel = EditorViewModel(engine, { content }, { catalog }, store, prices)
        viewModel.tap(PointMm(0.mm, 0.mm), reach)
        val busy = CompletableDeferred<Unit>()
        engine.hold = busy

        viewModel.aim(PointMm(1_000.mm, 0.mm), reach)
        viewModel.aim(PointMm(2_000.mm, 0.mm), reach)
        viewModel.aim(PointMm(3_000.mm, 0.mm), reach)
        viewModel.commit()
        busy.complete(Unit)

        // The first was already on its way; the second waited while the finger moved on.
        assertThat(
            engine.sent
                .flatten()
                .filterIsInstance<EngineCommand.Aim>()
                .map { it.at.x },
        ).containsExactly(0.mm, 1_000.mm, 3_000.mm)
            .inOrder()
        assertThat((engine.sent.last().single() as EngineCommand.AddWall).wall.end).isEqualTo(PointMm(3_000.mm, 0.mm))
    }

    @Test
    fun aWallBackToTheFirstPointClosesTheRun() {
        val viewModel = EditorViewModel(engine, { content }, { catalog }, store, prices)
        viewModel.tap(PointMm(0.mm, 0.mm), reach)
        viewModel.tap(PointMm(6_000.mm, 0.mm), reach)
        viewModel.tap(PointMm(6_000.mm, 4_000.mm), reach)
        viewModel.tap(PointMm(0.mm, 4_000.mm), reach)

        viewModel.tap(PointMm(0.mm, 0.mm), reach)

        val state = viewModel.uiState.value
        assertThat(state.outlines).hasSize(4)
        assertThat(state.draft).isNull()
        assertThat((engine.sent.last().single() as EngineCommand.AddWall).wall.end).isEqualTo(PointMm(0.mm, 0.mm))
    }

    @Test
    fun aWallThatMeetsThePlanEndsTheRun() {
        val viewModel = EditorViewModel(engine, { content }, { catalog }, store, prices)
        viewModel.tap(PointMm(0.mm, 0.mm), reach)
        viewModel.tap(PointMm(6_000.mm, 0.mm), reach)
        viewModel.finishWall()

        // A partition down onto the first wall: the engine puts its end on that wall's line.
        viewModel.tap(PointMm(5_000.mm, 2_000.mm), reach)
        engine.aimNext = Aimed(PointMm(5_000.mm, 0.mm), length = 2_000.mm, meets = true)
        viewModel.tap(PointMm(5_060.mm, 450.mm), reach)

        assertThat((engine.sent.last().single() as EngineCommand.AddWall).wall.end).isEqualTo(PointMm(5_000.mm, 0.mm))
        assertThat(viewModel.uiState.value.outlines).hasSize(2)
        assertThat(viewModel.uiState.value.draft).isNull()
    }

    @Test
    fun aRoomIsEnteredByItsMeasuredLengthsAndClosed() {
        val viewModel = EditorViewModel(engine, { content }, { catalog }, store, prices)
        viewModel.tap(PointMm(0.mm, 0.mm), reach)
        viewModel.startMeasure()

        // 6 400 to the right, then left turns: 4 000 up and 6 400 back.
        listOf(6, 4, 0, 0).forEach(viewModel::typeDigit)
        assertThat(viewModel.uiState.value.draft)
            .isEqualTo(Draft(PointMm(0.mm, 0.mm), Aimed(PointMm(6_400.mm, 0.mm), length = 6_400.mm)))
        viewModel.nextMeasured()
        viewModel.setTurn(Turn.LEFT)
        listOf(4, 0, 0, 1).forEach(viewModel::typeDigit)
        viewModel.eraseDigit()
        viewModel.typeDigit(0)
        viewModel.nextMeasured()
        assertThat(viewModel.uiState.value.measure).isEqualTo(Measure(turn = Turn.LEFT))
        assertThat(viewModel.uiState.value.canCloseRun).isTrue()
        listOf(6, 4, 0, 0).forEach(viewModel::typeDigit)
        // The engine is told where the wall before started, so it knows which way is left.
        assertThat(engine.sent.last())
            .containsExactly(
                EngineCommand.AimMeasured(level, PointMm(6_400.mm, 4_000.mm), before = PointMm(6_400.mm, 0.mm), Turn.LEFT, 6_400.mm),
            )
        assertThat(
            viewModel.uiState.value.draft
                ?.to
                ?.point,
        ).isEqualTo(PointMm(0.mm, 4_000.mm))
        viewModel.nextMeasured()

        viewModel.closeMeasured()

        val state = viewModel.uiState.value
        assertThat(state.outlines).hasSize(4)
        assertThat(state.measure).isNull()
        assertThat(state.draft).isNull()
        val closing = (engine.sent.last().single() as EngineCommand.AddWall).wall
        assertThat(closing.start).isEqualTo(PointMm(0.mm, 4_000.mm))
        assertThat(closing.end).isEqualTo(PointMm(0.mm, 0.mm))
    }

    @Test
    fun touchesDoNotPlacePointsWhileLengthsAreEntered() {
        val viewModel = EditorViewModel(engine, { content }, { catalog }, store, prices)
        viewModel.tap(PointMm(0.mm, 0.mm), reach)
        viewModel.startMeasure()

        viewModel.tap(PointMm(3_000.mm, 0.mm), reach)
        assertThat(viewModel.uiState.value.outlines).isEmpty()
        viewModel.nextMeasured()
        assertThat(viewModel.uiState.value.outlines).isEmpty()
        // Nor does a touch build the wall whose length is typed.
        viewModel.typeDigit(3)
        viewModel.tap(PointMm(3_000.mm, 0.mm), reach)
        assertThat(viewModel.uiState.value.outlines).isEmpty()

        viewModel.stopMeasure()
        assertThat(viewModel.uiState.value.measure).isNull()
        assertThat(viewModel.uiState.value.draft).isEqualTo(Draft(PointMm(0.mm, 0.mm)))
    }

    @Test
    fun aTouchBesideAnOpenListClosesItAndDrawsNothing() {
        val viewModel = EditorViewModel(engine, { content }, { catalog }, store, prices)
        viewModel.showTakeoff(true)

        viewModel.tap(PointMm(0.mm, 0.mm), reach)

        assertThat(viewModel.uiState.value.takeoffOpen).isFalse()
        assertThat(viewModel.uiState.value.draft).isNull()
    }

    @Test
    fun aSecondFingerDropsThePointBeingAimed() {
        val viewModel = EditorViewModel(engine, { content }, { catalog }, store, prices)

        viewModel.aim(PointMm(0.mm, 0.mm), reach)
        viewModel.cancelAim()
        assertThat(viewModel.uiState.value.draft).isNull()

        viewModel.tap(PointMm(0.mm, 0.mm), reach)
        viewModel.aim(PointMm(3_000.mm, 0.mm), reach)
        viewModel.cancelAim()
        assertThat(viewModel.uiState.value.draft).isEqualTo(Draft(PointMm(0.mm, 0.mm)))
        assertThat(viewModel.uiState.value.outlines).isEmpty()
    }

    @Test
    fun undoStepsBackAlongTheRun() {
        val viewModel = EditorViewModel(engine, { content }, { catalog }, store, prices)
        viewModel.tap(PointMm(0.mm, 0.mm), reach)
        viewModel.tap(PointMm(6_000.mm, 0.mm), reach)
        viewModel.tap(PointMm(6_000.mm, 4_000.mm), reach)

        viewModel.undo()

        assertThat(viewModel.uiState.value.outlines).hasSize(1)
        assertThat(viewModel.uiState.value.draft).isEqualTo(Draft(PointMm(6_000.mm, 0.mm)))
    }

    @Test
    fun aTypedLengthIsLeftToTheEngine() {
        val viewModel = EditorViewModel(engine, { content }, { catalog }, store, prices)
        viewModel.tap(PointMm(0.mm, 0.mm), reach)
        viewModel.tap(PointMm(6_000.mm, 0.mm), reach)
        viewModel.tap(PointMm(6_000.mm, 4_000.mm), reach)
        val first =
            viewModel.uiState.value.outlines
                .first()
                .wall

        viewModel.editLength(first)
        assertThat(viewModel.uiState.value.entry).isEqualTo(LengthEntry(first, was = 6_000.mm))
        listOf(0, 6, 4, 3, 7, 1).forEach(viewModel::typeDigit)
        viewModel.eraseDigit()
        viewModel.confirmLength()

        assertThat(engine.sent.last()).containsExactly(EngineCommand.SetWallLength(first, 6_437.mm))
        assertThat(viewModel.uiState.value.entry).isNull()
    }

    @Test
    fun undoingATypedLengthLeavesTheRunOnItsWall() {
        val viewModel = EditorViewModel(engine, { content }, { catalog }, store, prices)
        viewModel.tap(PointMm(0.mm, 0.mm), reach)
        viewModel.tap(PointMm(6_400.mm, 0.mm), reach)
        viewModel.editLength(viewModel.uiState.value.drawn!!)
        listOf(6, 4, 3, 7).forEach(viewModel::typeDigit)
        viewModel.confirmLength()

        viewModel.undo()

        assertThat(viewModel.uiState.value.outlines).hasSize(1)
        assertThat(viewModel.uiState.value.draft).isEqualTo(Draft(PointMm(6_400.mm, 0.mm)))
    }

    @Test
    fun aTouchWhileASizeIsTypedDrawsNothing() {
        val viewModel = EditorViewModel(engine, { content }, { catalog }, store, prices)
        viewModel.startSize()

        viewModel.tap(PointMm(0.mm, 0.mm), reach)

        assertThat(viewModel.uiState.value.draft).isNull()
        assertThat(engine.sent.flatten().filterIsInstance<EngineCommand.Aim>()).isEmpty()
    }

    @Test
    fun theLengthOfTheWallJustDrawnMovesWhereTheNextOneStarts() {
        val viewModel = EditorViewModel(engine, { content }, { catalog }, store, prices)
        viewModel.tap(PointMm(0.mm, 0.mm), reach)
        viewModel.tap(PointMm(6_400.mm, 0.mm), reach)
        val drawn = viewModel.uiState.value.drawn!!

        viewModel.editLength(drawn)
        listOf(6, 4, 3, 7).forEach(viewModel::typeDigit)
        viewModel.confirmLength()

        assertThat(viewModel.uiState.value.draft).isEqualTo(Draft(PointMm(6_437.mm, 0.mm)))
    }

    @Test
    fun aWindowGoesIntoTheTappedWallWithTheChosenSize() {
        val viewModel = EditorViewModel(engine, { content }, { catalog }, store, prices)
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
        val viewModel = EditorViewModel(engine, { content }, { catalog }, store, prices)
        viewModel.selectTool(Tool.DOOR)

        viewModel.tap(PointMm(1_000.mm, 0.mm), reach)

        val place = engine.sent.last().single() as EngineCommand.PlaceOpening
        assertThat(place.sill).isEqualTo(0.mm)
        assertThat(place.width).isEqualTo(900.mm)
        assertThat(place.kind).isEqualTo(OpeningKind.DOOR)
    }

    @Test
    fun changingTheToolDropsAWallStart() {
        val viewModel = EditorViewModel(engine, { content }, { catalog }, store, prices)
        viewModel.tap(PointMm(0.mm, 0.mm), reach)

        viewModel.selectTool(Tool.WINDOW)

        assertThat(viewModel.uiState.value.draft).isNull()
    }

    @Test
    fun aNewStoreyStandsOnTheTopOneAndIsShownOverIt() {
        val viewModel = EditorViewModel(engine, { content }, { catalog }, store, prices)
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
        val viewModel = EditorViewModel(engine, { content }, { catalog }, store, prices)
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
        val viewModel = EditorViewModel(engine, { content }, { catalog }, store, prices)
        viewModel.addLevel()

        viewModel.undo()

        assertThat(viewModel.uiState.value.levels).containsExactly(level)
        assertThat(viewModel.uiState.value.level).isEqualTo(level)
        assertThat(viewModel.uiState.value.canUndo).isFalse()
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
        val viewModel = EditorViewModel(engine, { content }, { catalog }, store, prices)
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
        val viewModel = EditorViewModel(engine, { content }, { catalog }, store, prices)
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
        val viewModel = EditorViewModel(engine, { content }, { catalog }, store, prices)
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
        val viewModel = EditorViewModel(engine, { content }, { catalog }, store, prices)
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
    fun wallsAreDrawnWithoutAMaterialUntilOneIsChosen() {
        val viewModel = EditorViewModel(engine, { content }, { catalog }, store, prices)
        assertThat(viewModel.uiState.value.material).isNull()
        assertThat(viewModel.uiState.value.thickness).isEqualTo(380.mm)
        viewModel.tap(PointMm(0.mm, 0.mm), reach)
        viewModel.tap(PointMm(6_000.mm, 0.mm), reach)
        assertThat((engine.sent.last().single() as EngineCommand.AddWall).wall.unit).isNull()

        viewModel.showMaterials(true)
        viewModel.chooseMaterial(catalog.units[1])

        val state = viewModel.uiState.value
        assertThat(state.materialOpen).isFalse()
        assertThat(state.material).isEqualTo(block)
        // 380 mm is not a block wall; 390 mm is the nearest one.
        assertThat(state.thickness).isEqualTo(390.mm)
        viewModel.tap(PointMm(6_000.mm, 4_000.mm), reach)
        val wall = (engine.sent.last().single() as EngineCommand.AddWall).wall
        assertThat(wall.unit).isEqualTo(block)
        assertThat(wall.thickness).isEqualTo(390.mm)
    }

    @Test
    fun aUnitSizeOfTheBuildersOwnIsTypedAndChosen() {
        val viewModel = EditorViewModel(engine, { content }, { catalog }, store, prices)
        viewModel.showMaterials(true)
        viewModel.startSize()

        // The local 300 × 150 × 200 block.
        listOf(3, 0, 0).forEach(viewModel::typeDigit)
        viewModel.selectSizeField(1)
        listOf(1, 5, 0, 9).forEach(viewModel::typeDigit)
        viewModel.eraseDigit()
        assertThat(
            viewModel.uiState.value.sizeEntry
                ?.size,
        ).isNull()
        viewModel.selectSizeField(2)
        listOf(2, 0, 0).forEach(viewModel::typeDigit)
        viewModel.confirmSize()

        val state = viewModel.uiState.value
        val own = UnitSize(300.mm, 150.mm, 200.mm)
        assertThat(state.sizeEntry).isNull()
        assertThat(state.materialOpen).isFalse()
        assertThat(state.material).isEqualTo(own)
        // Laid along the wall or across it; 380 mm is nearer the 300.
        assertThat(state.thicknessesOf(own)).containsExactly(150.mm, 300.mm).inOrder()
        assertThat(state.thickness).isEqualTo(300.mm)
    }

    @Test
    fun theSelectedWallTakesTheChosenMaterialAndAThicknessItBuilds() {
        val viewModel = EditorViewModel(engine, { content }, { catalog }, store, prices)
        val wall = viewModel.buildWall()
        viewModel.selectTool(Tool.SELECT)
        engine.pickNext = wall
        viewModel.tap(PointMm(3_000.mm, 0.mm), reach)

        viewModel.chooseMaterial(catalog.units[1])

        assertThat(engine.sent.last())
            .containsExactly(
                EngineCommand.BeginGesture,
                EngineCommand.SetWallUnit(wall, block),
                EngineCommand.SetWallThickness(wall, 390.mm),
                EngineCommand.EndGesture,
            ).inOrder()
        assertThat((viewModel.uiState.value.selected as Selected.WallPart).wall.unit).isEqualTo(block)
        // The builder has chosen nothing for new walls yet, so they go on as this one.
        assertThat(viewModel.uiState.value.material).isEqualTo(block)
    }

    @Test
    fun aChoiceForOneWallLeavesWhatNewWallsAreLaidFrom() {
        val viewModel = EditorViewModel(engine, { content }, { catalog }, store, prices)
        viewModel.chooseMaterial(catalog.units[0])
        val wall = viewModel.buildWall()
        viewModel.selectTool(Tool.SELECT)
        engine.pickNext = wall
        viewModel.tap(PointMm(3_000.mm, 0.mm), reach)

        viewModel.chooseMaterial(catalog.units[1])

        assertThat((viewModel.uiState.value.selected as Selected.WallPart).wall.unit).isEqualTo(block)
        assertThat(viewModel.uiState.value.material).isEqualTo(brick)
        assertThat(viewModel.uiState.value.thickness).isEqualTo(380.mm)
    }

    @Test
    fun theStoreyGetsTheRingBeamAndMeshesTheBuilderAsksFor() {
        val viewModel = EditorViewModel(engine, { content }, { catalog }, store, prices)
        assertThat(viewModel.uiState.value.ringBeam).isEqualTo(0.mm)

        viewModel.setRingBeam(150.mm)
        viewModel.setJunctionMeshes(true)

        assertThat(engine.sent.takeLast(2).map { it.single() })
            .containsExactly(EngineCommand.SetLevelRingBeam(level, 150.mm), EngineCommand.SetLevelJunctionMeshes(level, true))
            .inOrder()
        assertThat(viewModel.uiState.value.ringBeam).isEqualTo(150.mm)
        assertThat(viewModel.uiState.value.junctionMeshes).isTrue()
    }

    @Test
    fun theStoreysFootingIsTypedAsWidthAndDepthAndRemoved() {
        val viewModel = EditorViewModel(engine, { content }, { catalog }, store, prices)
        viewModel.showTakeoff(true)

        viewModel.startFooting()
        listOf(5, 0, 0).forEach(viewModel::typeDigit)
        viewModel.confirmSize()
        assertThat(viewModel.uiState.value.sizeEntry).isNotNull()
        viewModel.selectSizeField(1)
        listOf(8, 0, 0).forEach(viewModel::typeDigit)
        viewModel.confirmSize()

        assertThat(engine.sent.last().single()).isEqualTo(EngineCommand.SetLevelFooting(level, Footing(500.mm, 800.mm)))
        assertThat(viewModel.uiState.value.sizeEntry).isNull()
        assertThat(viewModel.uiState.value.footing).isEqualTo(Footing(500.mm, 800.mm))

        viewModel.setFooting(null)
        assertThat(viewModel.uiState.value.footing).isNull()
    }

    @Test
    fun aSavedJobGoesOnInTheMaterialOfItsWalls() {
        val wall = ElementId(Uuid.parse("019a5f00-0000-7000-8000-000000000011"))
        val saved =
            mapOf(
                level to Level(0.mm, 3_000.mm),
                wall to Wall(level, PointMm(0.mm, 0.mm), PointMm(6_000.mm, 0.mm), 390.mm, 2_800.mm, unit = block),
            )
        store = FakeStore(SavedJob(saved, clock = 42L))

        val viewModel = EditorViewModel(engine, { content }, { catalog }, store, prices)

        assertThat(viewModel.uiState.value.material).isEqualTo(block)
        assertThat(viewModel.uiState.value.thickness).isEqualTo(390.mm)
    }

    @Test
    fun theJobsConditionsFollowTheEngineAndGoWhenUndone() {
        val viewModel = EditorViewModel(engine, { content }, { catalog }, store, prices)
        assertThat(viewModel.uiState.value.conditions).isEqualTo(Conditions())
        viewModel.showTakeoff(true)
        viewModel.showConditions(true)

        viewModel.setCondition(Condition.CEMENT_GRADE, 500)

        assertThat(engine.sent.last().single()).isEqualTo(EngineCommand.SetCondition(Condition.CEMENT_GRADE, 500))
        assertThat(viewModel.uiState.value.conditions.cementGrade).isEqualTo(500)
        assertThat(viewModel.uiState.value.conditionsOpen).isTrue()

        viewModel.undo()
        assertThat(viewModel.uiState.value.conditions).isEqualTo(Conditions())
    }

    @Test
    fun aTypedPriceGoesToThePriceListAndPricesTheLine() {
        val viewModel = EditorViewModel(engine, { content }, { catalog }, store, prices)
        viewModel.buildWall()
        val line =
            viewModel.uiState.value.takeoff
                .single()

        viewModel.editPrice(line)
        listOf(0, 1, 4, 5, 0, 9).forEach(viewModel::typeDigit)
        viewModel.eraseDigit()
        viewModel.confirmPrice()

        val state = viewModel.uiState.value
        assertThat(state.priceEntry).isNull()
        // 1 450 sum a piece, in tiyin.
        assertThat(state.prices).containsExactly("masonry_unit", Money(145_000, Currency.UZS))
        // 100 pieces.
        assertThat(cost(line, state.prices)).isEqualTo(Money(14_500_000, Currency.UZS))
        assertThat(total(state.takeoff, state.prices)).isEqualTo(Money(14_500_000, Currency.UZS))

        viewModel.editPrice(line)
        viewModel.confirmPrice()
        assertThat(viewModel.uiState.value.prices).isEmpty()
        assertThat(total(state.takeoff, viewModel.uiState.value.prices)).isNull()
    }

    @Test
    fun onlyThePartsOfAMixAreBoughtAndTotalled() {
        val lines =
            listOf(
                QuantityLine("masonry", "masonry_unit.250x120x65", QuantityUnit.PIECE, 1_000.0),
                QuantityLine("masonry", "mortar", QuantityUnit.CUBIC_METRE, 1.0),
                QuantityLine("masonry", "mortar.cement", QuantityUnit.KILOGRAM, 300.0),
                QuantityLine("masonry", "mortar.cement.bag50", QuantityUnit.PIECE, 6.0),
                QuantityLine("masonry", "mortar.sand", QuantityUnit.CUBIC_METRE, 1.18),
            )
        val sum = { whole: Long -> Money(whole * 100, Currency.UZS) }
        // A price left on the mortar or on the cement by weight is not counted twice.
        val prices =
            mapOf(
                "mortar" to sum(650_000),
                "mortar.cement" to sum(1_400),
                "mortar.cement.bag50" to sum(70_000),
                "mortar.sand" to sum(150_000),
            )

        assertThat(bought(lines).map { it.item }).containsExactly("masonry_unit.250x120x65", "mortar.cement.bag50", "mortar.sand").inOrder()
        // Six bags and 1,18 m³ of sand.
        assertThat(total(lines, prices)).isEqualTo(sum(6 * 70_000 + 177_000))
    }

    @Test
    fun aCostFollowsTheAmountAsItIsShown() {
        // 5,256 m³ shows as 5,26; at 650 000 sum a cubic metre that is 3 419 000, not 3 416 400.
        val mortar = QuantityLine("masonry", "mortar", QuantityUnit.CUBIC_METRE, 5.256)

        assertThat(cost(mortar, mapOf("mortar" to Money(65_000_000, Currency.UZS)))).isEqualTo(Money(341_900_000, Currency.UZS))
    }

    @Test
    fun wallsWithoutAMaterialAreListed() {
        val viewModel = EditorViewModel(engine, { content }, { catalog }, store, prices)
        val wall = ElementId(Uuid.fromLongs(2, 0))
        engine.issues = listOf(Issue(IssueKind.MATERIAL_NOT_CHOSEN, wall, level, null))

        viewModel.buildWall()

        assertThat(viewModel.uiState.value.unchosen).containsExactly(wall)
    }

    @Test
    fun eachWallOfTheStoreyGetsItsLength() {
        val viewModel = EditorViewModel(engine, { content }, { catalog }, store, prices)

        viewModel.buildWall()

        val dimension =
            viewModel.uiState.value.dimensions
                .single()
        assertThat(dimension.wall.end).isEqualTo(PointMm(6_000.mm, 0.mm))
        assertThat(dimension.length).isEqualTo(6_000.mm)
    }

    @Test
    fun onlyTheIssuesOfTheShownStoreyAreDrawn() {
        val viewModel = EditorViewModel(engine, { content }, { catalog }, store, prices)
        val here = Issue(IssueKind.WALL_OVERLAP, element = null, level, PointMm(0.mm, 0.mm))
        val upstairs = Issue(IssueKind.SHARP_ANGLE, element = null, ElementId(Uuid.fromLongs(9, 9)), PointMm(0.mm, 0.mm))
        engine.issues = listOf(here, upstairs)

        viewModel.buildWall()

        assertThat(viewModel.uiState.value.issues).containsExactly(here)
    }

    @Test
    fun refusalAndFailedTakeoffAreShown() {
        val viewModel = EditorViewModel(engine, { content }, { catalog }, store, prices)
        viewModel.tap(PointMm(0.mm, 0.mm), reach)

        viewModel.aim(PointMm(4_000.mm, 0.mm), reach)
        engine.refuseNext = EngineError.OFF_PLAN
        viewModel.commit()
        assertThat(viewModel.uiState.value.refused).isEqualTo(EngineError.OFF_PLAN)
        // The wall was not built, so the next one still starts where this one did.
        assertThat(viewModel.uiState.value.draft).isEqualTo(Draft(PointMm(0.mm, 0.mm)))

        engine.refuseNext = EngineError.RULE_FAILED
        viewModel.undo()
        assertThat(viewModel.uiState.value.takeoffFailed).isTrue()
        assertThat(viewModel.uiState.value.takeoff).isEmpty()
    }
}
