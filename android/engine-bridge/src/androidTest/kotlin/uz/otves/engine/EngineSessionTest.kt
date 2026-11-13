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
    fun theEngineAimsAWallAndATypedLengthKeepsTheRoomSquare() {
        val level = session.send(listOf(EngineCommand.AddLevel(Level(0.mm, 3000.mm)))).createdId()
        val corners = listOf(PointMm(0.mm, 0.mm), PointMm(6000.mm, 0.mm), PointMm(6000.mm, 4000.mm), PointMm(0.mm, 4000.mm))
        val room =
            corners.indices.map { index ->
                val wall = Wall(level, corners[index], corners[(index + 1) % corners.size], 380.mm, 2800.mm)
                session.send(listOf(EngineCommand.AddWall(wall))).createdId()
            }

        // A partition from inside the room: the finger is beside the bottom wall and off the grid.
        val from = PointMm(3000.mm, 2000.mm)
        val asked =
            session.send(
                listOf(
                    EngineCommand.Aim(level, from, at = PointMm(3060.mm, 450.mm), reach = 300.mm, grid = 100.mm, square = true),
                    EngineCommand.AimMeasured(level, corners[1], before = corners[0], Turn.LEFT, 4000.mm),
                ),
            )
        assertThat(asked.results)
            .containsExactly(
                CommandResult.Done(created = null, aimed = Aimed(PointMm(3000.mm, 0.mm), length = 2000.mm, meets = true)),
                CommandResult.Done(created = null, aimed = Aimed(corners[2], length = 4000.mm, meets = true)),
            ).inOrder()
        assertThat(asked.changes).isEmpty()
        assertThat(asked.log).isEmpty()

        val longer = session.send(listOf(EngineCommand.SetWallLength(room[0], 6400.mm)))
        assertThat(longer.results.single()).isEqualTo(CommandResult.Done(created = null))
        val moved = longer.changes.associate { it.id to it.element as Wall }
        assertThat(moved.keys).containsExactly(room[0], room[1], room[2])
        assertThat(moved.getValue(room[1]).start).isEqualTo(PointMm(6400.mm, 0.mm))
        assertThat(moved.getValue(room[1]).end).isEqualTo(PointMm(6400.mm, 4000.mm))
        assertThat(longer.log).hasSize(1)

        // A partition drawn from inside the room onto the bottom wall grows from that wall.
        val partition =
            session
                .send(listOf(EngineCommand.AddWall(Wall(level, PointMm(3000.mm, 3000.mm), PointMm(3000.mm, 0.mm), 120.mm, 2800.mm))))
                .createdId()
        val grown = session.send(listOf(EngineCommand.SetWallLength(partition, 3500.mm)))
        assertThat((grown.changes.single().element as Wall).start).isEqualTo(PointMm(3000.mm, 3500.mm))

        val refused = session.send(listOf(EngineCommand.SetWallLength(room[0], 0.mm)))
        assertThat((refused.results.single() as CommandResult.Failed).error).isEqualTo(EngineError.ZERO_LENGTH)

        val undone = session.send(listOf(EngineCommand.Undo, EngineCommand.Undo, EngineCommand.Undo))
        assertThat((undone.changes.single { it.id == room[1] }.element as Wall).end).isEqualTo(corners[2])
    }

    @Test
    fun aSketchedRoomComesBackAsWallsThatAreNotExactUntilSized() {
        val level = session.send(listOf(EngineCommand.AddLevel(Level(0.mm, 3000.mm)))).createdId()
        // A room drawn roughly: no corner on the grid, the loop not quite closed.
        val stroke =
            listOf(40 to -30, 3000 to 20, 5960 to 50, 6030 to 2000, 5980 to 4040, 3000 to 3970, -20 to 3960, 30 to 2000, 10 to 60)
                .map { (x, y) -> PointMm(x.mm, y.mm) }
        val sketch =
            EngineCommand.AddSketch(
                level,
                stroke,
                reach = 300.mm,
                grid = 100.mm,
                square = true,
                thickness = 380.mm,
                height = 2800.mm,
            )

        val drawn = session.send(listOf(sketch))

        assertThat(drawn.results.single()).isEqualTo(CommandResult.Done(created = null))
        val walls = drawn.changes.map { it.element as Wall }
        val corners = listOf(PointMm(0.mm, 0.mm), PointMm(6000.mm, 0.mm), PointMm(6000.mm, 4000.mm), PointMm(0.mm, 4000.mm))
        assertThat(walls.map { it.start to it.end }).containsExactly(*Array(4) { corners[it] to corners[(it + 1) % 4] })
        assertThat(walls.none { it.exact }).isTrue()
        assertThat(walls.all { it.thickness == 380.mm && it.level == level }).isTrue()
        assertThat(drawn.outlines).hasSize(4)
        assertThat(drawn.log).hasSize(1)

        val bottom = drawn.changes.single { (it.element as Wall).end == corners[1] }.id
        val sized = session.send(listOf(EngineCommand.SetWallLength(bottom, 6400.mm)))
        assertThat((sized.changes.single { it.id == bottom }.element as Wall).exact).isTrue()

        val tap = session.send(listOf(sketch.copy(points = stroke.take(1))))
        assertThat((tap.results.single() as CommandResult.Failed).error).isEqualTo(EngineError.ZERO_LENGTH)
        assertThat(tap.changes).isEmpty()
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

        // The builder chose 1НФ brick and a 150 mm ring beam; nothing else is counted.
        val level = session.send(listOf(EngineCommand.AddLevel(Level(0.mm, 3000.mm, ringBeam = 150.mm)))).createdId()
        val brick = UnitSize(250.mm, 120.mm, 65.mm)
        val wall = Wall(level, PointMm(0.mm, 0.mm), PointMm(6400.mm, 0.mm), 380.mm, 2800.mm, unit = brick)
        val built = session.send(listOf(EngineCommand.AddWall(wall)))

        assertThat(built.outlines).hasSize(1)
        val lines = (built.takeoff as Takeoff.Lines).lines
        // 6.4 × 2.8 m of 380 mm 1НФ brick: about 2 686 units net, before waste.
        assertThat(lines.single { it.item == "masonry_unit.250x120x65" }.amount).isGreaterThan(2686.0)
        assertThat(lines.map { it.item }).contains("concrete.b15")
        assertThat(lines.map { it.item }).doesNotContain("mesh.junction")
        assertThat(ContentAssets.catalog(assets).units.map { it.size }).contains(brick)
    }

    @Test
    fun closedSessionRefusesCommands() {
        session.close()

        assertThrows(IllegalStateException::class.java) { session.send(listOf(EngineCommand.Undo)) }
    }
}
