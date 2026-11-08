package uz.otves.engine

import com.google.common.truth.Truth.assertThat
import org.junit.Test
import uz.otves.core.model.ElementId
import uz.otves.core.model.mm
import uz.otves.schema.Command
import uz.otves.schema.Element
import uz.otves.schema.Request
import kotlin.uuid.Uuid
import uz.otves.schema.AddWall as AddWallTable
import uz.otves.schema.LoadContent as LoadContentTable
import uz.otves.schema.LocationLine as LocationLineCode
import uz.otves.schema.MoveWallEnd as MoveWallEndTable
import uz.otves.schema.OpenModel as OpenModelTable
import uz.otves.schema.Opening as OpeningTable
import uz.otves.schema.OpeningKind as OpeningKindCode
import uz.otves.schema.Pick as PickTable
import uz.otves.schema.PlaceOpening as PlaceOpeningTable
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
    fun placementCarriesTheTapReachAndOpening() {
        val place =
            EngineCommand.PlaceOpening(
                level = level,
                at = PointMm(3000.mm, 250.mm),
                reach = 400.mm,
                width = 900.mm,
                height = 2100.mm,
                sill = 0.mm,
                kind = OpeningKind.DOOR,
            )

        val request = Request.getRootAsRequest(RequestEncoder.encode(listOf(place)))

        assertThat(request.commands(0).commandType()).isEqualTo(Command.PlaceOpening)
        val placed = request.commands(0).command(PlaceOpeningTable()) as PlaceOpeningTable
        assertThat(Uuid.fromLongs(placed.level().high(), placed.level().low())).isEqualTo(level.uuid)
        assertThat(placed.at().y()).isEqualTo(250L)
        assertThat(placed.reach()).isEqualTo(400L)
        assertThat(placed.width()).isEqualTo(900L)
        assertThat(placed.height()).isEqualTo(2100L)
        assertThat(placed.kind()).isEqualTo(OpeningKindCode.Door)
    }

    @Test
    fun openingAJobCarriesItsElementsAndClock() {
        val opening = ElementId(Uuid.parse("019a5f00-0000-7000-8000-000000000003"))
        val open =
            EngineCommand.OpenModel(
                elements =
                    mapOf(
                        level to Level(elevation = 0.mm, height = 3000.mm),
                        wall to Wall(level, PointMm(0.mm, 0.mm), PointMm(6400.mm, 0.mm), 380.mm, 2800.mm),
                        opening to Opening(wall, 1200.mm, 1500.mm, 1500.mm, 900.mm, OpeningKind.WINDOW),
                    ),
                clock = 0x0100_0000_0000_0000L,
            )

        val request = Request.getRootAsRequest(RequestEncoder.encode(listOf(open)))

        assertThat(request.commands(0).commandType()).isEqualTo(Command.OpenModel)
        val opened = request.commands(0).command(OpenModelTable()) as OpenModelTable
        assertThat(opened.clock()).isEqualTo(0x0100_0000_0000_0000L)
        assertThat(List(opened.elementsLength()) { opened.elements(it).elementType() })
            .containsExactly(Element.Level, Element.Wall, Element.Opening)
            .inOrder()
        val saved = opened.elements(2)
        assertThat(Uuid.fromLongs(saved.id().high(), saved.id().low())).isEqualTo(opening.uuid)
        assertThat((saved.element(OpeningTable()) as OpeningTable).sill()).isEqualTo(900L)
    }

    @Test
    fun pickCarriesTheTap() {
        val request = Request.getRootAsRequest(RequestEncoder.encode(listOf(EngineCommand.Pick(level, PointMm(10.mm, (-20).mm), 300.mm))))

        assertThat(request.commands(0).commandType()).isEqualTo(Command.Pick)
        val pick = request.commands(0).command(PickTable()) as PickTable
        assertThat(pick.at().y()).isEqualTo(-20L)
        assertThat(pick.reach()).isEqualTo(300L)
    }

    @Test
    fun requestCarriesTheIdentifier() {
        val buffer = RequestEncoder.encode(listOf(EngineCommand.Redo))

        assertThat(Request.RequestBufferHasIdentifier(buffer)).isTrue()
    }
}
