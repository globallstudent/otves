package uz.otves.engine

import com.google.common.truth.Truth.assertThat
import org.junit.Test
import uz.otves.core.model.ElementId
import uz.otves.core.model.mm
import uz.otves.schema.Command
import uz.otves.schema.Request
import kotlin.uuid.Uuid
import uz.otves.schema.AddWall as AddWallTable
import uz.otves.schema.LoadContent as LoadContentTable
import uz.otves.schema.LocationLine as LocationLineCode
import uz.otves.schema.MoveWallEnd as MoveWallEndTable
import uz.otves.schema.WallEnd as WallEndCode

class RequestEncoderTest {
    private val level = ElementId(Uuid.parse("019a5f00-0000-7000-8000-000000000001"))
    private val wall = ElementId(Uuid.parse("019a5f00-0000-7000-8000-000000000002"))

    @Test
    fun commandsKeepTheirOrderTypeAndValues() {
        val commands =
            listOf(
                EngineCommand.BeginGesture,
                EngineCommand.AddWall(
                    Wall(
                        level = level,
                        start = PointMm(0.mm, 0.mm),
                        end = PointMm(6400.mm, 0.mm),
                        thickness = 380.mm,
                        height = 2800.mm,
                        locationLine = LocationLine.LEFT_FACE,
                    ),
                ),
                EngineCommand.MoveWallEnd(wall, WallEnd.START, PointMm((-100).mm, 50.mm)),
                EngineCommand.EndGesture,
                EngineCommand.Undo,
            )

        val request = Request.getRootAsRequest(RequestEncoder.encode(commands))

        assertThat(request.version()).isEqualTo(1L)
        assertThat(List(request.commandsLength()) { request.commands(it).commandType() })
            .containsExactly(Command.BeginGesture, Command.AddWall, Command.MoveWallEnd, Command.EndGesture, Command.Undo)
            .inOrder()

        val added = request.commands(1).command(AddWallTable()) as AddWallTable
        val encoded = added.wall()
        assertThat(Uuid.fromLongs(encoded.level().high(), encoded.level().low())).isEqualTo(level.uuid)
        assertThat(encoded.end().x()).isEqualTo(6400L)
        assertThat(encoded.thickness()).isEqualTo(380L)
        assertThat(encoded.height()).isEqualTo(2800L)
        assertThat(encoded.locationLine()).isEqualTo(LocationLineCode.LeftFace)

        val moved = request.commands(2).command(MoveWallEndTable()) as MoveWallEndTable
        assertThat(moved.end()).isEqualTo(WallEndCode.Start)
        assertThat(moved.to().x()).isEqualTo(-100L)
        assertThat(moved.to().y()).isEqualTo(50L)
    }

    @Test
    fun contentCarriesItsScripts() {
        val load =
            EngineCommand.LoadContent(
                norms = listOf(ContentScript("practice", "return {}"), ContentScript("seismic", "return { a = 1 }")),
                rules = listOf(ContentScript("masonry", "return { run = function() return {} end }")),
            )

        val request = Request.getRootAsRequest(RequestEncoder.encode(listOf(load)))

        assertThat(request.commands(0).commandType()).isEqualTo(Command.LoadContent)
        val content = request.commands(0).command(LoadContentTable()) as LoadContentTable
        assertThat(List(content.normsLength()) { content.norms(it).name() }).containsExactly("practice", "seismic").inOrder()
        assertThat(content.norms(1).source()).isEqualTo("return { a = 1 }")
        assertThat(content.rules(0).name()).isEqualTo("masonry")
    }

    @Test
    fun requestCarriesTheIdentifier() {
        val buffer = RequestEncoder.encode(listOf(EngineCommand.Redo))

        assertThat(Request.RequestBufferHasIdentifier(buffer)).isTrue()
    }
}
