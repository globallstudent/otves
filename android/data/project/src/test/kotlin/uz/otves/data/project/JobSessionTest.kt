package uz.otves.data.project

import com.google.common.truth.Truth.assertThat
import kotlinx.coroutines.test.runTest
import org.junit.Assert.assertThrows
import org.junit.Test
import uz.otves.core.model.ElementId
import uz.otves.core.model.mm
import uz.otves.engine.CommandResult
import uz.otves.engine.Drawing
import uz.otves.engine.ElementChange
import uz.otves.engine.Engine
import uz.otves.engine.EngineCommand
import uz.otves.engine.EngineError
import uz.otves.engine.EngineResponse
import uz.otves.engine.Level
import uz.otves.engine.LogEntry
import uz.otves.engine.PointMm
import uz.otves.engine.QuantityLine
import uz.otves.engine.QuantityUnit
import uz.otves.engine.Takeoff
import uz.otves.engine.Wall
import uz.otves.engine.WallOutline
import uz.otves.schema.CommandKind
import kotlin.uuid.Uuid

class JobSessionTest {
    private val ground = ElementId(Uuid.parse("019a5f00-0000-7000-8000-000000000001"))
    private val upper = ElementId(Uuid.parse("019a5f00-0000-7000-8000-000000000002"))
    private val wall = ElementId(Uuid.parse("019a5f00-0000-7000-8000-000000000003"))
    private val content = EngineCommand.LoadContent(emptyList(), emptyList())
    private val floor = Level(elevation = 0.mm, height = 3_000.mm)
    private val built = Wall(ground, PointMm(0.mm, 0.mm), PointMm(6_000.mm, 0.mm), 380.mm, 2_800.mm)
    private val engine = ScriptedEngine()

    @Test
    fun aNewJobGetsAGroundFloorThatUndoLeaves() =
        runTest {
            val store = FakeStore()
            val session = JobSession(engine, store)
            // The engine can undo the storey it just made; the builder did not make it.
            engine.answers += response(results = 3, changes = listOf(ElementChange(ground, floor)), canUndo = true, log = 1)

            session.inTurn { session.open(content, floor) }

            assertThat(engine.sent.single())
                .containsExactly(content, EngineCommand.OpenModel(emptyMap(), 0L), EngineCommand.AddLevel(floor))
                .inOrder()
            assertThat(store.recorded).hasSize(1)
            assertThat(session.model.levels).containsExactly(ground, floor)
            assertThat(session.model.canUndo).isFalse()
        }

    @Test
    fun aSavedJobOpensAsItWasSaved() =
        runTest {
            val saved = SavedJob(mapOf(ground to floor), clock = 42L, name = "Дом")
            val session = JobSession(engine, FakeStore(saved))
            engine.answers += response(results = 2, changes = listOf(ElementChange(ground, floor)))

            session.inTurn { session.open(content, floor) }

            assertThat(engine.sent.single()).containsExactly(content, EngineCommand.OpenModel(saved.elements, 42L)).inOrder()
            assertThat(session.name).isEqualTo("Дом")
        }

    @Test
    fun aStepIsWrittenToTheFileAndMirrored() =
        runTest {
            val store = FakeStore()
            val session = opened(store)
            val outline = WallOutline(wall, ground, emptyList(), length = 6_000.mm)
            val lines = Takeoff.Lines(listOf(QuantityLine("masonry", "masonry_unit.250x120x65", QuantityUnit.PIECE, 1_000.0)))
            val drawing = CommandResult.Done(created = null, drawing = Drawing())
            engine.answers +=
                EngineResponse(
                    results = listOf(CommandResult.Done(created = wall), drawing),
                    changes = listOf(ElementChange(wall, built)),
                    canUndo = true,
                    canRedo = false,
                    outlines = listOf(outline),
                    takeoff = lines,
                    log = listOf(entry(CommandKind.AddWall)),
                )
            val ask = EngineCommand.Draw(ground, 400.mm, 200.mm)

            val outcome = session.inTurn { session.execute(listOf(EngineCommand.AddWall(built)), questions = listOf(ask)) }

            // The question rides in the same request and its answer comes back apart.
            assertThat(engine.sent.last()).containsExactly(EngineCommand.AddWall(built), ask).inOrder()
            assertThat(outcome.created).isEqualTo(wall)
            assertThat(outcome.results).hasSize(1)
            assertThat(outcome.answers).containsExactly(drawing)
            assertThat(outcome.takeoff).isEqualTo(lines)
            assertThat(store.recorded).hasSize(2)
            assertThat(session.model.walls).containsExactly(wall, built)
            assertThat(session.model.outlines).containsExactly(outline)
            assertThat(session.model.canUndo).isTrue()

            // A response with no outlines keeps the ones before it; a deleted element goes.
            engine.answers += response(results = 1, changes = listOf(ElementChange(wall, null)), canUndo = true, log = 1)
            session.inTurn { session.execute(listOf(EngineCommand.RemoveWall(wall))) }
            assertThat(session.model.walls).isEmpty()
            assertThat(session.model.outlines).containsExactly(outline)
        }

    @Test
    fun undoGoesBackToTheStateTheJobWasOpenedInAndNoFurther() =
        runTest {
            val session = opened(FakeStore())
            engine.answers += response(results = 1, changes = listOf(ElementChange(wall, built)), canUndo = true, log = 1)
            session.inTurn { session.execute(listOf(EngineCommand.AddWall(built))) }
            assertThat(session.model.canUndo).isTrue()

            // The engine could still undo the ground floor.
            engine.answers +=
                EngineResponse(
                    results = listOf(CommandResult.Done(created = null)),
                    changes = listOf(ElementChange(wall, null)),
                    canUndo = true,
                    canRedo = true,
                    log = listOf(entry(CommandKind.Undo)),
                )
            session.inTurn { session.execute(listOf(EngineCommand.Undo)) }

            assertThat(session.model.canUndo).isFalse()
            assertThat(session.model.canRedo).isTrue()
        }

    @Test
    fun aRefusalIsReportedAndAQuestionIsNotWritten() =
        runTest {
            val store = FakeStore()
            val session = opened(store)
            engine.answers +=
                EngineResponse(
                    results = listOf(CommandResult.Failed(EngineError.ZERO_LENGTH, "wall has no length")),
                    changes = emptyList(),
                    canUndo = false,
                    canRedo = false,
                )
            val outcome = session.inTurn { session.execute(listOf(EngineCommand.AddWall(built))) }
            assertThat(outcome.refused).isEqualTo(EngineError.ZERO_LENGTH)
            assertThat(outcome.created).isNull()

            val written = store.recorded.size
            val found = CommandResult.Done(created = null, picked = wall)
            engine.answers += EngineResponse(results = listOf(found), changes = emptyList(), canUndo = false, canRedo = false)
            val answers = session.inTurn { session.ask(listOf(EngineCommand.Pick(ground, PointMm(0.mm, 0.mm), 100.mm))) }
            assertThat(answers).containsExactly(found)
            assertThat(store.recorded).hasSize(written)
        }

    @Test
    fun storeysAreListedFromTheBottomUp() =
        runTest {
            val saved = SavedJob(mapOf(upper to Level(3_000.mm, 3_000.mm), ground to floor), clock = 1L)
            val session = JobSession(engine, FakeStore(saved))
            engine.answers += response(results = 2, changes = saved.elements.map { ElementChange(it.key, it.value) })

            session.inTurn { session.open(content, floor) }

            assertThat(session.model.levelsFromBottom()).containsExactly(ground, upper).inOrder()
        }

    @Test
    fun theEngineIsSpokenToOnlyInsideATurnAndClosingClosesTheFile() {
        val store = FakeStore()
        val session = JobSession(engine, store)

        assertThrows(IllegalStateException::class.java) {
            runTest { session.execute(listOf(EngineCommand.Undo)) }
        }
        assertThat(engine.sent).isEmpty()

        session.close()
        assertThat(store.closed).isTrue()
    }

    // A session with its job open on the ground floor.
    private suspend fun opened(store: FakeStore): JobSession {
        val session = JobSession(engine, store)
        engine.answers += response(results = 3, changes = listOf(ElementChange(ground, floor)), canUndo = true, log = 1)
        session.inTurn { session.open(content, floor) }
        return session
    }

    private fun response(
        results: Int,
        changes: List<ElementChange>,
        canUndo: Boolean = false,
        log: Int = 0,
    ) = EngineResponse(
        results = List(results) { CommandResult.Done(created = null) },
        changes = changes,
        canUndo = canUndo,
        canRedo = false,
        log = List(log) { entry(CommandKind.AddWall) },
    )

    private fun entry(kind: Int) = LogEntry(Uuid.random(), kind, hlc = 1L, device = Uuid.NIL, origin = null, ops = ByteArray(0))

    /** An engine that answers with what the test lined up, in order. */
    private class ScriptedEngine : Engine {
        val sent = mutableListOf<List<EngineCommand>>()
        val answers = ArrayDeque<EngineResponse>()

        override suspend fun send(commands: List<EngineCommand>): EngineResponse {
            sent += commands
            return answers.removeFirst()
        }
    }

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
}
