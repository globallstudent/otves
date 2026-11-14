package uz.otves.engine

import com.google.common.truth.Truth.assertThat
import org.junit.Test
import uz.otves.core.model.ElementId
import uz.otves.core.model.mm
import uz.otves.schema.Command
import uz.otves.schema.Element
import uz.otves.schema.Request
import kotlin.uuid.Uuid
import uz.otves.schema.AddOpening as AddOpeningTable
import uz.otves.schema.AddRoom as AddRoomTable
import uz.otves.schema.AddRoomWalls as AddRoomWallsTable
import uz.otves.schema.AddSketch as AddSketchTable
import uz.otves.schema.AddWall as AddWallTable
import uz.otves.schema.AddWallAcross as AddWallAcrossTable
import uz.otves.schema.AddWallType as AddWallTypeTable
import uz.otves.schema.Aim as AimTable
import uz.otves.schema.AimMeasured as AimMeasuredTable
import uz.otves.schema.CenterOpening as CenterOpeningTable
import uz.otves.schema.Draw as DrawTable
import uz.otves.schema.FixedEnd as FixedEndCode
import uz.otves.schema.Hand as HandCode
import uz.otves.schema.Hinge as HingeCode
import uz.otves.schema.LayerFunction as LayerFunctionCode
import uz.otves.schema.LoadContent as LoadContentTable
import uz.otves.schema.LocationLine as LocationLineCode
import uz.otves.schema.MoveRoom as MoveRoomTable
import uz.otves.schema.MoveWallEnd as MoveWallEndTable
import uz.otves.schema.OpenModel as OpenModelTable
import uz.otves.schema.Opening as OpeningTable
import uz.otves.schema.OpeningKind as OpeningKindCode
import uz.otves.schema.Pick as PickTable
import uz.otves.schema.PlaceOpening as PlaceOpeningTable
import uz.otves.schema.RemoveRoom as RemoveRoomTable
import uz.otves.schema.Room as RoomTable
import uz.otves.schema.RoomKind as RoomKindCode
import uz.otves.schema.SetDimension as SetDimensionTable
import uz.otves.schema.SetOpeningHinge as SetOpeningHingeTable
import uz.otves.schema.SetOpeningSwing as SetOpeningSwingTable
import uz.otves.schema.SetRoomKind as SetRoomKindTable
import uz.otves.schema.SetWallLayer as SetWallLayerTable
import uz.otves.schema.SetWallLength as SetWallLengthTable
import uz.otves.schema.SetWallMirrored as SetWallMirroredTable
import uz.otves.schema.SetWallType as SetWallTypeTable
import uz.otves.schema.SetWallTypeRole as SetWallTypeRoleTable
import uz.otves.schema.Swing as SwingCode
import uz.otves.schema.TextKind as TextKindCode
import uz.otves.schema.Turn as TurnCode
import uz.otves.schema.WallEnd as WallEndCode
import uz.otves.schema.WallLayer as WallLayerTable
import uz.otves.schema.WallRole as WallRoleCode

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
    fun aimsAndATypedLengthCarryTheirValues() {
        val commands =
            listOf(
                EngineCommand.Aim(level, from = null, at = PointMm(30.mm, (-40).mm), reach = 300.mm, grid = 100.mm, square = true),
                EngineCommand.Aim(level, PointMm(0.mm, 0.mm), PointMm(6_420.mm, 260.mm), 300.mm, 100.mm, square = false),
                EngineCommand.AimMeasured(level, PointMm(6_400.mm, 0.mm), before = PointMm(0.mm, 0.mm), Turn.LEFT, 4_000.mm),
                EngineCommand.AimMeasured(level, PointMm(0.mm, 0.mm), before = null, Turn.STRAIGHT, 6_400.mm),
                EngineCommand.SetWallLength(wall, 6_437.mm, fixed = WallEnd.END),
                EngineCommand.SetWallLength(wall, 6_437.mm),
            )

        val request = Request.getRootAsRequest(RequestEncoder.encode(commands))

        assertThat(List(request.commandsLength()) { request.commands(it).commandType() })
            .containsExactly(
                Command.Aim,
                Command.Aim,
                Command.AimMeasured,
                Command.AimMeasured,
                Command.SetWallLength,
                Command.SetWallLength,
            ).inOrder()
        val first = request.commands(0).command(AimTable()) as AimTable
        assertThat(Uuid.fromLongs(first.level().high(), first.level().low())).isEqualTo(level.uuid)
        assertThat(first.from()).isNull()
        assertThat(first.at().y()).isEqualTo(-40L)
        assertThat(first.reach()).isEqualTo(300L)
        assertThat(first.grid()).isEqualTo(100L)
        assertThat(first.square()).isTrue()
        val second = request.commands(1).command(AimTable()) as AimTable
        assertThat(second.from().x()).isEqualTo(0L)
        assertThat(second.at().x()).isEqualTo(6_420L)
        assertThat(second.square()).isFalse()
        val turned = request.commands(2).command(AimMeasuredTable()) as AimMeasuredTable
        assertThat(turned.from().x()).isEqualTo(6_400L)
        assertThat(turned.before().x()).isEqualTo(0L)
        assertThat(turned.turn()).isEqualTo(TurnCode.Left)
        assertThat(turned.length()).isEqualTo(4_000L)
        val straight = request.commands(3).command(AimMeasuredTable()) as AimMeasuredTable
        assertThat(straight.before()).isNull()
        assertThat(straight.turn()).isEqualTo(TurnCode.Straight)
        val sized = request.commands(4).command(SetWallLengthTable()) as SetWallLengthTable
        assertThat(Uuid.fromLongs(sized.wall().high(), sized.wall().low())).isEqualTo(wall.uuid)
        assertThat(sized.length()).isEqualTo(6_437L)
        assertThat(sized.fixed()).isEqualTo(FixedEndCode.End)
        // With no end named the engine reads which one stays off the plan.
        assertThat((request.commands(5).command(SetWallLengthTable()) as SetWallLengthTable).fixed()).isEqualTo(FixedEndCode.Joined)
    }

    @Test
    fun aSketchCarriesItsStrokeAndTheWallToBuild() {
        val sketch =
            EngineCommand.AddSketch(
                level = level,
                points = listOf(PointMm(40.mm, (-30).mm), PointMm(3_000.mm, 20.mm), PointMm(5_960.mm, 50.mm)),
                reach = 300.mm,
                grid = 100.mm,
                square = true,
                thickness = 380.mm,
                height = 2_800.mm,
                unit = UnitSize(250.mm, 120.mm, 65.mm),
            )

        val request = Request.getRootAsRequest(RequestEncoder.encode(listOf(sketch)))

        assertThat(request.commands(0).commandType()).isEqualTo(Command.AddSketch)
        val drawn = request.commands(0).command(AddSketchTable()) as AddSketchTable
        assertThat(List(drawn.pointsLength()) { drawn.points(it).x() }).containsExactly(40L, 3_000L, 5_960L).inOrder()
        assertThat(drawn.points(0).y()).isEqualTo(-30L)
        assertThat(drawn.reach()).isEqualTo(300L)
        assertThat(drawn.grid()).isEqualTo(100L)
        assertThat(drawn.square()).isTrue()
        val like = drawn.wall()
        assertThat(Uuid.fromLongs(like.level().high(), like.level().low())).isEqualTo(level.uuid)
        assertThat(like.start()).isNull()
        assertThat(like.thickness()).isEqualTo(380L)
        assertThat(like.height()).isEqualTo(2_800L)
        assertThat(like.unitWidth()).isEqualTo(120L)
    }

    @Test
    fun aWallSaysWhetherItsLengthIsExact() {
        val sketched = Wall(level, PointMm(0.mm, 0.mm), PointMm(6_000.mm, 0.mm), 380.mm, 2_800.mm, exact = false)
        val commands = listOf(EngineCommand.AddWall(sketched), EngineCommand.AddWall(sketched.copy(exact = true)))

        val request = Request.getRootAsRequest(RequestEncoder.encode(commands))

        assertThat((request.commands(0).command(AddWallTable()) as AddWallTable).wall().exact()).isFalse()
        assertThat((request.commands(1).command(AddWallTable()) as AddWallTable).wall().exact()).isTrue()
    }

    @Test
    fun aDrawingIsAskedWithTheSizeOfTheLabelText() {
        val request =
            Request.getRootAsRequest(
                RequestEncoder.encode(listOf(EngineCommand.Draw(level, textHeight = 400.mm, digitWidth = 210.mm))),
            )

        assertThat(request.commands(0).commandType()).isEqualTo(Command.Draw)
        val draw = request.commands(0).command(DrawTable()) as DrawTable
        assertThat(Uuid.fromLongs(draw.level().high(), draw.level().low())).isEqualTo(level.uuid)
        assertThat(draw.textHeight()).isEqualTo(400L)
        assertThat(draw.digitWidth()).isEqualTo(210L)
        // No space is in focus unless the app names a point, and the screen's drawing is no sheet.
        assertThat(draw.focus()).isNull()
        assertThat(draw.sheet()).isFalse()
    }

    @Test
    fun aWallAcrossARoomCarriesThePointTheHandAndTheDistance() {
        val type = ElementId(Uuid.parse("019a5f00-0000-7000-8000-000000000007"))
        val commands =
            listOf(
                EngineCommand.AddWallAcross(level, PointMm(3_000.mm, 2_000.mm), Hand.LEFT, 1_500.mm, 120.mm, 2_800.mm),
                EngineCommand.AddWallAcross(level, PointMm(3_000.mm, 2_000.mm), Hand.TOP, 900.mm, 0.mm, 2_800.mm, type = type),
            )

        val request = Request.getRootAsRequest(RequestEncoder.encode(commands))

        assertThat(request.commands(0).commandType()).isEqualTo(Command.AddWallAcross)
        val across = request.commands(0).command(AddWallAcrossTable()) as AddWallAcrossTable
        assertThat(across.at().x()).isEqualTo(3_000L)
        assertThat(across.at().y()).isEqualTo(2_000L)
        assertThat(across.from()).isEqualTo(HandCode.Left)
        assertThat(across.distance()).isEqualTo(1_500L)
        assertThat(across.wall().thickness()).isEqualTo(120L)
        assertThat(across.wall().height()).isEqualTo(2_800L)
        assertThat(Uuid.fromLongs(across.wall().level().high(), across.wall().level().low())).isEqualTo(level.uuid)
        // The engine places the wall: it carries no points.
        assertThat(across.wall().start()).isNull()
        val typed = (request.commands(1).command(AddWallAcrossTable()) as AddWallAcrossTable).wall()
        assertThat(Uuid.fromLongs(typed.type().high(), typed.type().low())).isEqualTo(type.uuid)
    }

    @Test
    fun aRoomByItsSizeCarriesTheTouchTheSizesAndTheWayItLies() {
        val room =
            EngineCommand.AddRoomWalls(
                level = level,
                at = PointMm(300.mm, 350.mm),
                reach = 300.mm,
                grid = 100.mm,
                width = 1_600.mm,
                depth = 2_000.mm,
                right = false,
                up = true,
                thickness = 120.mm,
                height = 2_800.mm,
            )

        val request = Request.getRootAsRequest(RequestEncoder.encode(listOf(room)))

        assertThat(request.commands(0).commandType()).isEqualTo(Command.AddRoomWalls)
        val sent = request.commands(0).command(AddRoomWallsTable()) as AddRoomWallsTable
        assertThat(sent.at().x()).isEqualTo(300L)
        assertThat(sent.at().y()).isEqualTo(350L)
        assertThat(sent.reach()).isEqualTo(300L)
        assertThat(sent.grid()).isEqualTo(100L)
        assertThat(sent.width()).isEqualTo(1_600L)
        assertThat(sent.depth()).isEqualTo(2_000L)
        assertThat(sent.right()).isFalse()
        assertThat(sent.up()).isTrue()
        assertThat(sent.wall().thickness()).isEqualTo(120L)
        // The engine places the walls: the request carries no points for them.
        assertThat(sent.wall().start()).isNull()
    }

    @Test
    fun aSheetIsAskedWithEveryRoomsSizes() {
        val request = Request.getRootAsRequest(RequestEncoder.encode(listOf(EngineCommand.Draw(level, 60.mm, 30.mm, sheet = true))))

        assertThat((request.commands(0).command(DrawTable()) as DrawTable).sheet()).isTrue()
    }

    @Test
    fun aDrawingNamesTheSpaceInFocusAndATypedSizeGoesBackAsItsTextCame() {
        val window = ElementId(Uuid.parse("019a5f00-0000-7000-8000-000000000003"))
        val pier = DrawnText(window, TextKind.PIER, 1_800, PointM(1.1, 0.6), PointM(1.0, 0.0), from = 5_800.mm, forward = false)
        val commands =
            listOf(
                EngineCommand.Draw(level, 400.mm, 210.mm, focus = PointMm(3_000.mm, (-2_000).mm)),
                EngineCommand.SetDimension(pier, 1_250.mm),
                EngineCommand.SetDimension(pier.copy(kind = TextKind.OPENING_WIDTH), 900.mm),
                EngineCommand.SetDimension(pier.copy(kind = TextKind.ROOM_AREA, element = null), 900.mm),
            )

        val request = Request.getRootAsRequest(RequestEncoder.encode(commands))

        val draw = request.commands(0).command(DrawTable()) as DrawTable
        assertThat(draw.focus().x()).isEqualTo(3_000L)
        assertThat(draw.focus().y()).isEqualTo(-2_000L)
        assertThat(request.commands(1).commandType()).isEqualTo(Command.SetDimension)
        val sized = request.commands(1).command(SetDimensionTable()) as SetDimensionTable
        assertThat(sized.kind()).isEqualTo(TextKindCode.Pier)
        assertThat(Uuid.fromLongs(sized.element().high(), sized.element().low())).isEqualTo(window.uuid)
        assertThat(sized.from()).isEqualTo(5_800L)
        assertThat(sized.forward()).isFalse()
        assertThat(sized.value()).isEqualTo(1_250L)
        assertThat((request.commands(2).command(SetDimensionTable()) as SetDimensionTable).kind()).isEqualTo(TextKindCode.OpeningWidth)
        // A text that is no size goes out as one the engine refuses.
        val refused = request.commands(3).command(SetDimensionTable()) as SetDimensionTable
        assertThat(refused.kind()).isEqualTo(TextKindCode.Overall)
        assertThat(refused.element()).isNull()
    }

    @Test
    fun aWallTypeCarriesItsRoleAndItsLayersFromTheOuterFaceIn() {
        val brick = UnitSize(250.mm, 120.mm, 65.mm)
        val add =
            EngineCommand.AddWallType(
                role = WallRole.BEARING,
                layers =
                    listOf(
                        LayerSpec(LayerFunction.FACING, 120.mm, brick),
                        LayerSpec(LayerFunction.INSULATION, 100.mm),
                        LayerSpec(LayerFunction.STRUCTURE, 250.mm, brick),
                    ),
            )

        val request = Request.getRootAsRequest(RequestEncoder.encode(listOf(add)))

        assertThat(request.commands(0).commandType()).isEqualTo(Command.AddWallType)
        val type = request.commands(0).command(AddWallTypeTable()) as AddWallTypeTable
        assertThat(type.role()).isEqualTo(WallRoleCode.Bearing)
        assertThat(List(type.layersLength()) { type.layers(it).function() })
            .containsExactly(LayerFunctionCode.Facing, LayerFunctionCode.Insulation, LayerFunctionCode.Structure)
            .inOrder()
        assertThat(List(type.layersLength()) { type.layers(it).thickness() }).containsExactly(120L, 100L, 250L).inOrder()
        assertThat(type.layers(0).unitHeight()).isEqualTo(65L)
        assertThat(type.layers(1).unitLength()).isEqualTo(0L)
    }

    @Test
    fun aWallNamesItsTypeAndWhichFaceItsLayersStartFrom() {
        val type = ElementId(Uuid.parse("019a5f00-0000-7000-8000-000000000007"))
        val layer = ElementId(Uuid.parse("019a5f00-0000-7000-8000-000000000008"))
        val typed = Wall(level, PointMm(0.mm, 0.mm), PointMm(6_000.mm, 0.mm), 0.mm, 2_800.mm, type = type, mirrored = true)
        val commands =
            listOf(
                EngineCommand.AddWall(typed),
                EngineCommand.SetWallType(wall, type),
                EngineCommand.SetWallMirrored(wall, mirrored = true),
                EngineCommand.SetWallLayer(layer, 150.mm, unit = null),
                EngineCommand.SetWallTypeRole(type, WallRole.PARTITION),
                EngineCommand.AddSketch(
                    level,
                    listOf(PointMm(0.mm, 0.mm), PointMm(6_000.mm, 0.mm)),
                    300.mm,
                    100.mm,
                    true,
                    0.mm,
                    2_800.mm,
                    type = type,
                ),
            )

        val request = Request.getRootAsRequest(RequestEncoder.encode(commands))

        val added = (request.commands(0).command(AddWallTable()) as AddWallTable).wall()
        assertThat(Uuid.fromLongs(added.type().high(), added.type().low())).isEqualTo(type.uuid)
        assertThat(added.mirrored()).isTrue()
        assertThat(added.thickness()).isEqualTo(0L)
        val set = request.commands(1).command(SetWallTypeTable()) as SetWallTypeTable
        assertThat(Uuid.fromLongs(set.wall().high(), set.wall().low())).isEqualTo(wall.uuid)
        assertThat(Uuid.fromLongs(set.type().high(), set.type().low())).isEqualTo(type.uuid)
        assertThat((request.commands(2).command(SetWallMirroredTable()) as SetWallMirroredTable).mirrored()).isTrue()
        val thicker = request.commands(3).command(SetWallLayerTable()) as SetWallLayerTable
        assertThat(Uuid.fromLongs(thicker.layer().high(), thicker.layer().low())).isEqualTo(layer.uuid)
        assertThat(thicker.thickness()).isEqualTo(150L)
        assertThat(thicker.unitLength()).isEqualTo(0L)
        assertThat((request.commands(4).command(SetWallTypeRoleTable()) as SetWallTypeRoleTable).role()).isEqualTo(WallRoleCode.Partition)
        val sketched = (request.commands(5).command(AddSketchTable()) as AddSketchTable).wall()
        assertThat(Uuid.fromLongs(sketched.type().high(), sketched.type().low())).isEqualTo(type.uuid)
        // A wall with no type says so by leaving it out.
        val plain = RequestEncoder.encode(listOf(EngineCommand.AddWall(typed.copy(type = null, thickness = 380.mm))))
        assertThat((Request.getRootAsRequest(plain).commands(0).command(AddWallTable()) as AddWallTable).wall().type()).isNull()
    }

    @Test
    fun aSavedJobCarriesItsWallTypesAndLayers() {
        val type = ElementId(Uuid.parse("019a5f00-0000-7000-8000-000000000007"))
        val layer = ElementId(Uuid.parse("019a5f00-0000-7000-8000-000000000008"))
        val open =
            EngineCommand.OpenModel(
                elements =
                    mapOf(
                        type to WallType(WallRole.EXTERIOR),
                        layer to WallLayer(type, position = 2, LayerFunction.STRUCTURE, 190.mm, UnitSize(390.mm, 190.mm, 188.mm)),
                    ),
                clock = 1L,
            )

        val request = Request.getRootAsRequest(RequestEncoder.encode(listOf(open)))

        val opened = request.commands(0).command(OpenModelTable()) as OpenModelTable
        assertThat(
            List(opened.elementsLength()) {
                opened.elements(it).elementType()
            },
        ).containsExactly(Element.WallType, Element.WallLayer).inOrder()
        val saved = opened.elements(1).element(WallLayerTable()) as WallLayerTable
        assertThat(Uuid.fromLongs(saved.type().high(), saved.type().low())).isEqualTo(type.uuid)
        assertThat(saved.position()).isEqualTo(2L)
        assertThat(saved.function()).isEqualTo(LayerFunctionCode.Structure)
        assertThat(saved.unitWidth()).isEqualTo(190L)
    }

    @Test
    fun aRoomCarriesItsLevelAnchorAndKind() {
        val room = ElementId(Uuid.parse("019a5f00-0000-7000-8000-000000000009"))
        val commands =
            listOf(
                EngineCommand.AddRoom(Room(level, PointMm(2_000.mm, (-1_500).mm), RoomKind.BEDROOM)),
                EngineCommand.MoveRoom(room, PointMm(300.mm, 400.mm)),
                EngineCommand.SetRoomKind(room, RoomKind.TAMBOUR),
                EngineCommand.RemoveRoom(room),
                EngineCommand.OpenModel(mapOf(room to Room(level, PointMm(1.mm, 2.mm), RoomKind.KITCHEN)), clock = 1L),
            )

        val request = Request.getRootAsRequest(RequestEncoder.encode(commands))

        assertThat(List(request.commandsLength()) { request.commands(it).commandType() })
            .containsExactly(Command.AddRoom, Command.MoveRoom, Command.SetRoomKind, Command.RemoveRoom, Command.OpenModel)
            .inOrder()
        val added = (request.commands(0).command(AddRoomTable()) as AddRoomTable).room()
        assertThat(Uuid.fromLongs(added.level().high(), added.level().low())).isEqualTo(level.uuid)
        assertThat(added.anchor().y()).isEqualTo(-1_500L)
        assertThat(added.kind()).isEqualTo(RoomKindCode.Bedroom)
        val moved = request.commands(1).command(MoveRoomTable()) as MoveRoomTable
        assertThat(Uuid.fromLongs(moved.room().high(), moved.room().low())).isEqualTo(room.uuid)
        assertThat(moved.anchor().x()).isEqualTo(300L)
        assertThat((request.commands(2).command(SetRoomKindTable()) as SetRoomKindTable).kind()).isEqualTo(RoomKindCode.Tambour)
        val removed = request.commands(3).command(RemoveRoomTable()) as RemoveRoomTable
        assertThat(Uuid.fromLongs(removed.room().high(), removed.room().low())).isEqualTo(room.uuid)
        val opened = request.commands(4).command(OpenModelTable()) as OpenModelTable
        assertThat(opened.elements(0).elementType()).isEqualTo(Element.Room)
        assertThat((opened.elements(0).element(RoomTable()) as RoomTable).kind()).isEqualTo(RoomKindCode.Kitchen)
    }

    @Test
    fun aDoorCarriesHowItHangsAndTheSelectedElementGoesWithADrawing() {
        val door = ElementId(Uuid.parse("019a5f00-0000-7000-8000-000000000003"))
        val commands =
            listOf(
                EngineCommand.AddOpening(Opening(wall, 1_000.mm, 900.mm, 2_100.mm, 0.mm, OpeningKind.DOOR, Hinge.FAR, Swing.RIGHT)),
                EngineCommand.SetOpeningHinge(door, Hinge.FAR),
                EngineCommand.SetOpeningSwing(door, Swing.RIGHT),
                EngineCommand.CenterOpening(door),
                EngineCommand.Draw(level, 400.mm, 210.mm, selected = door),
            )

        val request = Request.getRootAsRequest(RequestEncoder.encode(commands))

        assertThat(List(request.commandsLength()) { request.commands(it).commandType() })
            .containsExactly(Command.AddOpening, Command.SetOpeningHinge, Command.SetOpeningSwing, Command.CenterOpening, Command.Draw)
            .inOrder()
        val added = (request.commands(0).command(AddOpeningTable()) as AddOpeningTable).opening()
        assertThat(added.hinge()).isEqualTo(HingeCode.Far)
        assertThat(added.swing()).isEqualTo(SwingCode.Right)
        assertThat((request.commands(1).command(SetOpeningHingeTable()) as SetOpeningHingeTable).hinge()).isEqualTo(HingeCode.Far)
        assertThat((request.commands(2).command(SetOpeningSwingTable()) as SetOpeningSwingTable).swing()).isEqualTo(SwingCode.Right)
        val centred = request.commands(3).command(CenterOpeningTable()) as CenterOpeningTable
        assertThat(Uuid.fromLongs(centred.opening().high(), centred.opening().low())).isEqualTo(door.uuid)
        val draw = request.commands(4).command(DrawTable()) as DrawTable
        assertThat(Uuid.fromLongs(draw.selected().high(), draw.selected().low())).isEqualTo(door.uuid)
    }

    @Test
    fun everyRoomKindHasACodeOfItsOwn() {
        assertThat(RoomKind.entries.map { it.code }.toSet()).hasSize(RoomKind.entries.size)
        assertThat(RoomKind.entries.map { roomKindOf(it.code.toLong()) }).containsExactlyElementsIn(RoomKind.entries).inOrder()
        assertThat(roomKindOf(200)).isNull()
    }

    @Test
    fun requestCarriesTheIdentifier() {
        val buffer = RequestEncoder.encode(listOf(EngineCommand.Redo))

        assertThat(Request.RequestBufferHasIdentifier(buffer)).isTrue()
    }
}
