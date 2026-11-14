package uz.otves.data.project

import androidx.test.ext.junit.runners.AndroidJUnit4
import androidx.test.platform.app.InstrumentationRegistry
import com.google.common.truth.Truth.assertThat
import kotlinx.coroutines.test.runTest
import org.junit.After
import org.junit.Test
import org.junit.runner.RunWith
import uz.otves.core.model.ElementId
import uz.otves.core.model.Mm
import uz.otves.core.model.mm
import uz.otves.engine.CommandResult
import uz.otves.engine.ContentAssets
import uz.otves.engine.Drawing
import uz.otves.engine.EngineCommand
import uz.otves.engine.EngineGateway
import uz.otves.engine.IssueKind
import uz.otves.engine.Level
import uz.otves.engine.Opening
import uz.otves.engine.OpeningKind
import uz.otves.engine.PointMm
import uz.otves.engine.Room
import uz.otves.engine.RoomKind
import uz.otves.engine.Wall
import java.io.File
import kotlin.math.abs
import kotlin.uuid.Uuid

/**
 * «План3», one of the architects' sheets the editor is to match (docs/roadmap-drawing.md), built
 * in the real engine and kept in a job file: a one-storey house 8 300 by 12 800 mm, outer walls
 * 400, a bearing wall 300 down its length and partitions 150. The sizes are read off the sheet;
 * the areas are the ones written in its rooms.
 */
@RunWith(AndroidJUnit4::class)
class GoldenJobTest {
    private val context = InstrumentationRegistry.getInstrumentation().targetContext
    private val directory = File(context.cacheDir, "golden-test-${System.nanoTime()}")
    private val files = JobFiles(context, directory)
    private val device = Uuid.parse("019a5f00-0000-7000-8000-0000000000dd")
    private val ground = Level(elevation = 0.mm, height = 3_000.mm)

    @After
    fun tearDown() {
        directory.deleteRecursively()
    }

    // A wall on its centre line, from the outer bottom left corner of the house.
    private class Line(
        val fromX: Int,
        val fromY: Int,
        val toX: Int,
        val toY: Int,
        val thickness: Int,
    )

    private val walls =
        listOf(
            // Outside, counter-clockwise from the bottom.
            Line(200, 200, 8_100, 200, 400),
            Line(8_100, 200, 8_100, 12_600, 400),
            Line(8_100, 12_600, 200, 12_600, 400),
            Line(200, 12_600, 200, 200, 400),
            // The bearing wall: 3 100 clear to its left.
            Line(3_650, 12_600, 3_650, 200, 300),
            // Under the kitchen-living room, 8 100 deep, and under the boiler room, 2 000 deep.
            Line(200, 4_225, 3_650, 4_225, 150),
            Line(200, 2_075, 3_650, 2_075, 150),
            // Under the upper bedroom, 3 300 deep.
            Line(3_650, 9_025, 8_100, 9_025, 150),
            // Between the corridor and the bedrooms, and round the corner cut out of the lower one.
            Line(5_025, 9_025, 5_025, 1_725, 150),
            Line(5_025, 4_875, 8_100, 4_875, 150),
            Line(5_025, 1_725, 5_625, 1_725, 150),
            Line(5_625, 1_725, 5_625, 200, 150),
        )

    // An opening by the place of its wall in [walls], how far it starts from that wall's start
    // and how wide it is. The sheet gives no heights.
    private class Cut(
        val wall: Int,
        val offset: Int,
        val width: Int,
        val kind: OpeningKind = OpeningKind.WINDOW,
    )

    private val cuts =
        listOf(
            Cut(0, 675, 1_250),
            Cut(0, 3_750, 1_040, OpeningKind.DOOR),
            Cut(0, 5_975, 1_250),
            Cut(1, 5_950, 1_600),
            Cut(2, 950, 1_600),
            Cut(2, 5_350, 1_600),
            Cut(3, 1_700, 1_800),
            Cut(3, 5_200, 1_600),
            Cut(3, 9_150, 600),
            // The open way from the kitchen-living room to the corridor, then the doors.
            Cut(4, 3_600, 3_200, OpeningKind.EMPTY),
            Cut(4, 9_400, 900, OpeningKind.DOOR),
            Cut(4, 10_750, 800, OpeningKind.DOOR),
            Cut(7, 300, 900, OpeningKind.DOOR),
            Cut(8, 3_025, 900, OpeningKind.DOOR),
            Cut(8, 4_375, 900, OpeningKind.DOOR),
        )

    private val rooms =
        listOf(
            Triple(2_000, 8_000, RoomKind.KITCHEN_LIVING),
            Triple(6_000, 11_000, RoomKind.BEDROOM),
            Triple(6_500, 7_000, RoomKind.BEDROOM),
            Triple(6_500, 3_000, RoomKind.BEDROOM),
            Triple(4_400, 5_000, RoomKind.CORRIDOR),
            Triple(2_000, 3_000, RoomKind.BOILER),
            Triple(2_000, 1_000, RoomKind.BATHROOM),
        )

    private fun created(results: List<CommandResult>): List<ElementId> = results.map { checkNotNull((it as CommandResult.Done).created) }

    // What would say the plan is drawn wrong. A wall with no material yet and a lintel with too
    // short a bearing are the builder's to settle, not faults of the drawing.
    private val faults =
        setOf(
            IssueKind.OPENING_OUTSIDE_WALL,
            IssueKind.OPENING_OVERLAP,
            IssueKind.OPENING_WITHOUT_WALL,
            IssueKind.WALL_OVERLAP,
            IssueKind.SHARP_ANGLE,
            IssueKind.ROOM_WITHOUT_SPACE,
            IssueKind.ROOM_SHARES_SPACE,
        )

    private fun faultsOf(session: JobSession): List<IssueKind> =
        session.model.issues
            .map { it.kind }
            .filter { it in faults }

    // Draws the plan in an open job: the walls, the openings in them and the rooms' names.
    private suspend fun build(
        session: JobSession,
        level: ElementId,
    ) {
        val built =
            session.execute(
                walls.map { line ->
                    val wall =
                        Wall(level, PointMm(line.fromX.mm, line.fromY.mm), PointMm(line.toX.mm, line.toY.mm), line.thickness.mm, 2_800.mm)
                    EngineCommand.AddWall(wall)
                },
            )
        val ids = created(built.results)
        val cut =
            session.execute(
                cuts.map { opening ->
                    val window = opening.kind == OpeningKind.WINDOW
                    EngineCommand.AddOpening(
                        Opening(
                            ids[opening.wall],
                            opening.offset.mm,
                            opening.width.mm,
                            if (window) 1_500.mm else 2_100.mm,
                            if (window) 900.mm else 0.mm,
                            opening.kind,
                        ),
                    )
                },
            )
        assertThat(cut.refused).isNull()
        val named = session.execute(rooms.map { (x, y, kind) -> EngineCommand.AddRoom(Room(level, PointMm(x.mm, y.mm), kind)) })
        assertThat(named.refused).isNull()
    }

    // The storey as a sheet, with label text as an A4 sheet has it on this plan.
    private suspend fun sheet(
        session: JobSession,
        level: ElementId,
    ): Drawing {
        val asked = session.ask(listOf(EngineCommand.Draw(level, textHeight = Mm(223), digitWidth = Mm(108), sheet = true)))
        return checkNotNull((asked.single() as CommandResult.Done).drawing)
    }

    @Test
    fun theReferencePlanComesBackFromItsFileAsItWasDrawn() =
        runTest {
            val content = ContentAssets.load(context.assets)
            val job = files.create("План3")

            // Drawn once, in one engine.
            val (drawn, model) =
                EngineGateway(device).use { engine ->
                    JobSession(engine, files.open(job.id)).use { session ->
                        session.inTurn {
                            session.open(content, ground)
                            val level =
                                session.model.levels.keys
                                    .single()
                            build(session, level)
                            assertThat(faultsOf(session)).isEmpty()
                            sheet(session, level) to Triple(session.model.walls, session.model.openings, session.model.rooms)
                        }
                    }
                }

            // The rooms have the areas written on the sheet: 25,1, 13,5, 11,2 and 11,5 m², the
            // boiler room 6,2 and the bathroom 5.
            val areas = drawn.rooms.map { it.area }.sorted()
            val written = listOf(3.1 * 1.6, 3.1 * 2.0, 2.8 * 4.0, 2.8 * 4.4 - 0.6 * 1.4, 4.1 * 3.3, 3.1 * 8.1)
            assertThat(areas).hasSize(7)
            written.forEach { area -> assertThat(areas.any { abs(it - area) < 1e-9 }).isTrue() }
            // Windows of four sizes, doors of three and the open way.
            assertThat(drawn.openings.map { it.count }).containsExactly(2, 4, 1, 1, 1, 4, 1, 1).inOrder()

            // Opened again from the file, in another engine: the same model and the same sheet,
            // to the last number.
            EngineGateway(device).use { engine ->
                JobSession(engine, files.open(job.id)).use { session ->
                    session.inTurn {
                        session.open(content, ground)
                        val level =
                            session.model.levels.keys
                                .single()
                        assertThat(session.name).isEqualTo("План3")
                        assertThat(Triple(session.model.walls, session.model.openings, session.model.rooms)).isEqualTo(model)
                        assertThat(faultsOf(session)).isEmpty()
                        assertThat(sheet(session, level)).isEqualTo(drawn)
                    }
                }
            }
        }
}
