package uz.otves.engine

import androidx.test.ext.junit.runners.AndroidJUnit4
import androidx.test.platform.app.InstrumentationRegistry
import com.google.common.truth.Truth.assertThat
import org.junit.After
import org.junit.Assert.assertThrows
import org.junit.Test
import org.junit.runner.RunWith
import uz.otves.core.model.ElementId
import uz.otves.core.model.mm
import kotlin.uuid.Uuid

/** Kotlin → libdesign.so → Kotlin on a real device: the whole bridge, no fakes. */
@RunWith(AndroidJUnit4::class)
class EngineSessionTest {
    private val session = EngineSession(Uuid.random())

    @After
    fun close() = session.close()

    private fun EngineResponse.createdId(): ElementId = checkNotNull((results.single() as CommandResult.Done).created)

    @Test
    fun levelWallDragAndUndoTravelThroughTheEngine() {
        val ground = Level(elevation = 0.mm, height = 3000.mm)
        val added = session.send(listOf(EngineCommand.AddLevel(ground)))
        val level = added.createdId()
        assertThat(added.changes).containsExactly(ElementChange(level, ground))
        assertThat(added.canUndo).isTrue()

        val wall4 = Wall(level, PointMm(0.mm, 0.mm), PointMm(6400.mm, 0.mm), thickness = 380.mm, height = 2800.mm)
        val built = session.send(listOf(EngineCommand.AddWall(wall4)))
        val wall = built.createdId()
        assertThat(built.changes).containsExactly(ElementChange(wall, wall4))

        val drag =
            listOf(EngineCommand.BeginGesture) +
                (1..10).map { EngineCommand.MoveWallEnd(wall, WallEnd.END, PointMm((6400 + it * 10).mm, 0.mm)) } +
                EngineCommand.EndGesture
        val dragged = session.send(drag)
        assertThat(dragged.results).hasSize(drag.size)
        assertThat(dragged.results.all { it is CommandResult.Done }).isTrue()
        assertThat((dragged.changes.single().element as Wall).end).isEqualTo(PointMm(6500.mm, 0.mm))

        val undone = session.send(listOf(EngineCommand.Undo))
        assertThat((undone.changes.single().element as Wall).end).isEqualTo(PointMm(6400.mm, 0.mm))
        assertThat(undone.canRedo).isTrue()
    }

    @Test
    fun removingAWallReportsItAndItsOpeningsGone() {
        val level = session.send(listOf(EngineCommand.AddLevel(Level(0.mm, 3000.mm)))).createdId()
        val wall =
            session
                .send(listOf(EngineCommand.AddWall(Wall(level, PointMm(0.mm, 0.mm), PointMm(6400.mm, 0.mm), 380.mm, 2800.mm))))
                .createdId()
        val window =
            session
                .send(listOf(EngineCommand.AddOpening(Opening(wall, offset = 1200.mm, width = 1500.mm, height = 1500.mm, sill = 900.mm))))
                .createdId()

        val removed = session.send(listOf(EngineCommand.RemoveWall(wall)))

        assertThat(removed.changes).containsExactly(ElementChange(wall, null), ElementChange(window, null))
    }

    @Test
    fun refusedCommandsComeBackAsTypedErrors() {
        val response =
            session.send(
                listOf(
                    EngineCommand.AddLevel(Level(0.mm, 0.mm)),
                    EngineCommand.RemoveWall(ElementId(Uuid.random())),
                    EngineCommand.Undo,
                ),
            )

        assertThat(response.results.map { (it as CommandResult.Failed).error })
            .containsExactly(EngineError.HEIGHT_NOT_POSITIVE, EngineError.NOT_FOUND, EngineError.NOTHING_TO_UNDO)
            .inOrder()
        assertThat(response.changes).isEmpty()
    }

    @Test
    fun takeoffRunsTheBundledContent() {
        val assets = InstrumentationRegistry.getInstrumentation().targetContext.assets
        val loaded = session.send(listOf(ContentAssets.load(assets)))
        assertThat(loaded.results.single()).isInstanceOf(CommandResult.Done::class.java)

        val level = session.send(listOf(EngineCommand.AddLevel(Level(0.mm, 3000.mm)))).createdId()
        val built =
            session.send(listOf(EngineCommand.AddWall(Wall(level, PointMm(0.mm, 0.mm), PointMm(6400.mm, 0.mm), 380.mm, 2800.mm))))

        assertThat(built.outlines).hasSize(1)
        val lines = (built.takeoff as Takeoff.Lines).lines
        // 6.4 × 2.8 m of 380 mm 1НФ brick: about 2 686 units net, before waste.
        assertThat(lines.single { it.item == "masonry_unit" }.amount).isGreaterThan(2686.0)
        assertThat(lines.map { it.item }).contains("concrete.b15")
    }

    @Test
    fun closedSessionRefusesCommands() {
        session.close()

        assertThrows(IllegalStateException::class.java) { session.send(listOf(EngineCommand.Undo)) }
    }
}
