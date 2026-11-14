package uz.otves.engine

import com.google.common.truth.Truth.assertThat
import com.google.flatbuffers.FlatBufferBuilder
import org.junit.Assert.assertThrows
import org.junit.Test
import uz.otves.core.model.ElementId
import uz.otves.core.model.mm
import uz.otves.schema.Element
import uz.otves.schema.ErrorCode
import uz.otves.schema.Response
import java.nio.ByteBuffer
import kotlin.uuid.Uuid
import uz.otves.schema.Aimed as AimedTable
import uz.otves.schema.CommandKind as CommandKindCode
import uz.otves.schema.CommandResult as ResultTable
import uz.otves.schema.Drawing as DrawingTable
import uz.otves.schema.DrawnArc as ArcTable
import uz.otves.schema.DrawnLine as DrawnLineTable
import uz.otves.schema.DrawnPolygon as PolygonTable
import uz.otves.schema.DrawnText as TextTable
import uz.otves.schema.ElementChange as ChangeTable
import uz.otves.schema.Fill as FillCode
import uz.otves.schema.Guide as GuideStruct
import uz.otves.schema.Issue as IssueTable
import uz.otves.schema.IssueKind as IssueKindCode
import uz.otves.schema.LayerFunction as LayerFunctionCode
import uz.otves.schema.LocationLine as LocationLineCode
import uz.otves.schema.LogEntry as LogEntryTable
import uz.otves.schema.OpeningKind as OpeningKindCode
import uz.otves.schema.OpeningOutline as OpeningOutlineTable
import uz.otves.schema.PickedSpace as SpaceTable
import uz.otves.schema.PointM as PointMStruct
import uz.otves.schema.PointMm as PointStruct
import uz.otves.schema.QuantityLine as LineTable
import uz.otves.schema.Ring as RingTable
import uz.otves.schema.Room as RoomTable
import uz.otves.schema.RoomKind as RoomKindCode
import uz.otves.schema.Stroke as StrokeCode
import uz.otves.schema.TextKind as TextKindCode
import uz.otves.schema.Unit as UnitCode
import uz.otves.schema.Uuid as UuidStruct
import uz.otves.schema.Wall as WallTable
import uz.otves.schema.WallLayer as WallLayerTable
import uz.otves.schema.WallOutline as OutlineTable
import uz.otves.schema.WallRole as WallRoleCode
import uz.otves.schema.WallType as WallTypeTable

class ResponseDecoderTest {
    private val level = ElementId(Uuid.parse("019a5f00-0000-7000-8000-000000000001"))
    private val wall = ElementId(Uuid.parse("019a5f00-0000-7000-8000-000000000002"))
    private val gone = ElementId(Uuid.parse("019a5f00-0000-7000-8000-000000000003"))
    private val window = ElementId(Uuid.parse("019a5f00-0000-7000-8000-000000000004"))

    @Test
    fun resultsAndChangesComeBackTyped() {
        val decoded = ResponseDecoder.decode(engineResponse(errorCode = ErrorCode.NotFound))

        assertThat(decoded.results)
            .containsExactly(
                CommandResult.Done(created = wall, picked = window),
                CommandResult.Failed(error = EngineError.NOT_FOUND, message = "wall not found"),
            ).inOrder()
        assertThat(decoded.changes)
            .containsExactly(
                ElementChange(
                    id = wall,
                    element =
                        Wall(
                            level = level,
                            start = PointMm(0.mm, 0.mm),
                            end = PointMm(6400.mm, 0.mm),
                            thickness = 380.mm,
                            height = 2800.mm,
                            locationLine = LocationLine.RIGHT_FACE,
                            exact = false,
                        ),
                ),
                ElementChange(id = gone, element = null),
            ).inOrder()
        assertThat(decoded.canUndo).isTrue()
        assertThat(decoded.canRedo).isFalse()
        assertThat(decoded.outlines).isNull()
        assertThat(decoded.takeoff).isNull()
    }

    @Test
    fun outlinesAndTakeoffComeBackWhenTheModelChanged() {
        val decoded = ResponseDecoder.decode(derivedResponse(takeoffError = ErrorCode.None))

        assertThat(decoded.outlines)
            .containsExactly(WallOutline(wall, level, listOf(PointM(0.0, -0.19), PointM(6.4, -0.19), PointM(6.4, 0.19)), 6400.mm))
        assertThat(decoded.takeoff)
            .isEqualTo(Takeoff.Lines(listOf(QuantityLine("masonry", "masonry_unit", QuantityUnit.PIECE, 2767.0))))
        assertThat(decoded.openings)
            .containsExactly(OpeningOutline(window, level, OpeningKind.DOOR, listOf(PointM(1.0, -0.19), PointM(1.9, -0.19))))
        assertThat(decoded.issues)
            .containsExactly(
                Issue(IssueKind.SHORT_BEARING, window, level, at = null),
                Issue(IssueKind.WALL_OVERLAP, element = null, level, PointMm(0.mm, 0.mm)),
                Issue(IssueKind.UNKNOWN, element = null, level, at = null),
            ).inOrder()
    }

    @Test
    fun logEntriesComeBackWithTheirBytes() {
        val builder = FlatBufferBuilder(1024)
        val ops = LogEntryTable.createOpsVector(builder, byteArrayOf(1, 2, 3))
        LogEntryTable.startLogEntry(builder)
        LogEntryTable.addId(builder, builder.uuid(wall))
        LogEntryTable.addKind(builder, CommandKindCode.Undo)
        LogEntryTable.addHlc(builder, 42L)
        LogEntryTable.addDevice(builder, builder.uuid(level))
        LogEntryTable.addOrigin(builder, builder.uuid(gone))
        LogEntryTable.addOps(builder, ops)
        val undone = LogEntryTable.endLogEntry(builder)
        val noOps = LogEntryTable.createOpsVector(builder, byteArrayOf())
        LogEntryTable.startLogEntry(builder)
        LogEntryTable.addId(builder, builder.uuid(window))
        LogEntryTable.addDevice(builder, builder.uuid(level))
        LogEntryTable.addOrigin(builder, UuidStruct.createUuid(builder, 0, 0))
        LogEntryTable.addOps(builder, noOps)
        val added = LogEntryTable.endLogEntry(builder)
        val log = Response.createLogVector(builder, intArrayOf(undone, added))
        Response.startResponse(builder)
        Response.addLog(builder, log)
        builder.finish(Response.endResponse(builder), "OTVS")

        val decoded = ResponseDecoder.decode(builder.dataBuffer())

        assertThat(decoded.log)
            .containsExactly(
                LogEntry(wall.uuid, CommandKindCode.Undo, 42L, level.uuid, gone.uuid, byteArrayOf(1, 2, 3)),
                LogEntry(window.uuid, CommandKindCode.AddLevel, 0L, level.uuid, null, byteArrayOf()),
            ).inOrder()
        assertThat(decoded.log.map { it.isUndo }).containsExactly(true, false).inOrder()
        assertThat(decoded.log.map { it.isRedo }).containsExactly(false, false).inOrder()
    }

    @Test
    fun takeoffThatCouldNotRunCarriesItsError() {
        val decoded = ResponseDecoder.decode(derivedResponse(takeoffError = ErrorCode.RuleFailed))

        assertThat(decoded.takeoff).isEqualTo(Takeoff.Failed(EngineError.RULE_FAILED, "masonry:3: boom"))
    }

    @Test
    fun codeFromANewerEngineIsUnknown() {
        val decoded = ResponseDecoder.decode(engineResponse(errorCode = 200))

        assertThat((decoded.results[1] as CommandResult.Failed).error).isEqualTo(EngineError.UNKNOWN)
    }

    @Test
    fun bufferWithoutTheIdentifierIsRefused() {
        val buffer = engineResponse(errorCode = ErrorCode.NotFound)
        buffer.put(buffer.position() + 4, 'X'.code.toByte())

        assertThrows(IllegalStateException::class.java) { ResponseDecoder.decode(buffer) }
    }

    @Test
    fun anAimComesBackWithItsPointLengthAndGuides() {
        val builder = FlatBufferBuilder(256)
        AimedTable.startGuidesVector(builder, 1)
        GuideStruct.createGuide(builder, 0, 0, 0, 4000)
        val guides = builder.endVector()
        AimedTable.startAimed(builder)
        AimedTable.addPoint(builder, PointStruct.createPointMm(builder, 0, 4000))
        AimedTable.addLength(builder, 6000)
        AimedTable.addGuides(builder, guides)
        AimedTable.addMeets(builder, true)
        val aimed = AimedTable.endAimed(builder)
        ResultTable.startCommandResult(builder)
        ResultTable.addAimed(builder, aimed)
        val result = ResultTable.endCommandResult(builder)
        val results = Response.createResultsVector(builder, intArrayOf(result))
        Response.startResponse(builder)
        Response.addVersion(builder, 1L)
        Response.addResults(builder, results)
        builder.finish(Response.endResponse(builder), "OTVS")

        val decoded = ResponseDecoder.decode(builder.dataBuffer())

        val corner = PointMm(0.mm, 4000.mm)
        assertThat(decoded.results)
            .containsExactly(
                CommandResult.Done(
                    created = null,
                    aimed = Aimed(corner, length = 6000.mm, guides = listOf(Guide(PointMm(0.mm, 0.mm), corner)), meets = true),
                ),
            )
    }

    @Test
    fun aDrawingComesBackAsListsOfWhatToPaint() {
        val builder = FlatBufferBuilder(1024)
        PolygonTable.startPointsVector(builder, 3)
        PointMStruct.createPointM(builder, 6.4, 0.19)
        PointMStruct.createPointM(builder, 6.4, -0.19)
        PointMStruct.createPointM(builder, 0.0, -0.19)
        val outline = builder.endVector()
        PolygonTable.startDrawnPolygon(builder)
        PolygonTable.addElement(builder, builder.uuid(wall))
        PolygonTable.addFill(builder, FillCode.Wall)
        PolygonTable.addStroke(builder, StrokeCode.WallEdge)
        PolygonTable.addPoints(builder, outline)
        val masonry = PolygonTable.endDrawnPolygon(builder)
        // Hatched by a newer engine, under no element.
        PolygonTable.startDrawnPolygon(builder)
        PolygonTable.addFill(builder, 99)
        val newer = PolygonTable.endDrawnPolygon(builder)
        DrawnLineTable.startDrawnLine(builder)
        DrawnLineTable.addElement(builder, builder.uuid(window))
        DrawnLineTable.addStroke(builder, StrokeCode.Frame)
        DrawnLineTable.addFrom(builder, PointMStruct.createPointM(builder, 2.0, 0.0))
        DrawnLineTable.addTo(builder, PointMStruct.createPointM(builder, 3.5, 0.0))
        val glazing = DrawnLineTable.endDrawnLine(builder)
        ArcTable.startDrawnArc(builder)
        ArcTable.addStroke(builder, StrokeCode.Leaf)
        ArcTable.addCentre(builder, PointMStruct.createPointM(builder, 1.0, 0.19))
        ArcTable.addFrom(builder, PointMStruct.createPointM(builder, 1.9, 0.19))
        ArcTable.addTo(builder, PointMStruct.createPointM(builder, 1.0, 1.09))
        val swing = ArcTable.endDrawnArc(builder)
        TextTable.startDrawnText(builder)
        TextTable.addElement(builder, builder.uuid(wall))
        TextTable.addKind(builder, TextKindCode.AboutLength)
        TextTable.addValue(builder, 6400)
        TextTable.addAt(builder, PointMStruct.createPointM(builder, 3.2, 0.415))
        TextTable.addAlong(builder, PointMStruct.createPointM(builder, 1.0, 0.0))
        val length = TextTable.endDrawnText(builder)
        val drawing =
            DrawingTable.createDrawing(
                builder,
                DrawingTable.createPolygonsVector(builder, intArrayOf(masonry, newer)),
                DrawingTable.createLinesVector(builder, intArrayOf(glazing)),
                DrawingTable.createArcsVector(builder, intArrayOf(swing)),
                DrawingTable.createTextsVector(builder, intArrayOf(length)),
            )
        ResultTable.startCommandResult(builder)
        ResultTable.addDrawing(builder, drawing)
        val result = ResultTable.endCommandResult(builder)
        val results = Response.createResultsVector(builder, intArrayOf(result))
        Response.startResponse(builder)
        Response.addVersion(builder, 1L)
        Response.addResults(builder, results)
        builder.finish(Response.endResponse(builder), "OTVS")

        val decoded = (ResponseDecoder.decode(builder.dataBuffer()).results.single() as CommandResult.Done).drawing

        assertThat(decoded)
            .isEqualTo(
                Drawing(
                    polygons =
                        listOf(
                            DrawnPolygon(
                                wall,
                                FillKind.WALL,
                                StrokeKind.WALL_EDGE,
                                listOf(PointM(0.0, -0.19), PointM(6.4, -0.19), PointM(6.4, 0.19)),
                            ),
                            DrawnPolygon(element = null, FillKind.UNKNOWN, StrokeKind.NONE, emptyList()),
                        ),
                    lines = listOf(DrawnLine(window, StrokeKind.FRAME, PointM(2.0, 0.0), PointM(3.5, 0.0))),
                    arcs = listOf(DrawnArc(element = null, StrokeKind.LEAF, PointM(1.0, 0.19), PointM(1.9, 0.19), PointM(1.0, 1.09))),
                    texts = listOf(DrawnText(wall, TextKind.ABOUT_LENGTH, 6400, PointM(3.2, 0.415), PointM(1.0, 0.0))),
                ),
            )
    }

    @Test
    fun wallTypesLayersAndTypedWallsComeBackTyped() {
        val type = ElementId(Uuid.parse("019a5f00-0000-7000-8000-000000000007"))
        val layer = ElementId(Uuid.parse("019a5f00-0000-7000-8000-000000000008"))
        val builder = FlatBufferBuilder(1024)
        val typeValue = WallTypeTable.createWallType(builder, WallRoleCode.Partition)
        WallLayerTable.startWallLayer(builder)
        WallLayerTable.addType(builder, builder.uuid(type))
        WallLayerTable.addPosition(builder, 1)
        WallLayerTable.addFunction(builder, LayerFunctionCode.Insulation)
        WallLayerTable.addThickness(builder, 100)
        val layerValue = WallLayerTable.endWallLayer(builder)
        WallTable.startWall(builder)
        WallTable.addLevel(builder, builder.uuid(level))
        WallTable.addStart(builder, PointStruct.createPointMm(builder, 0, 0))
        WallTable.addEnd(builder, PointStruct.createPointMm(builder, 6000, 0))
        WallTable.addHeight(builder, 2800)
        WallTable.addType(builder, builder.uuid(type))
        WallTable.addMirrored(builder, true)
        val wallValue = WallTable.endWall(builder)
        val changes =
            listOf(
                Triple(type, Element.WallType, typeValue),
                Triple(layer, Element.WallLayer, layerValue),
                Triple(wall, Element.Wall, wallValue),
            ).map { (id, kind, value) ->
                ChangeTable.startElementChange(builder)
                ChangeTable.addId(builder, builder.uuid(id))
                ChangeTable.addElementType(builder, kind)
                ChangeTable.addElement(builder, value)
                ChangeTable.endElementChange(builder)
            }.toIntArray()
        ResultTable.startCommandResult(builder)
        ResultTable.addError(builder, ErrorCode.TypeDecides)
        val refused = ResultTable.endCommandResult(builder)
        val results = Response.createResultsVector(builder, intArrayOf(refused))
        val changed = Response.createChangesVector(builder, changes)
        Response.startResponse(builder)
        Response.addVersion(builder, 1L)
        Response.addResults(builder, results)
        Response.addChanges(builder, changed)
        builder.finish(Response.endResponse(builder), "OTVS")

        val decoded = ResponseDecoder.decode(builder.dataBuffer())

        assertThat(decoded.changes)
            .containsExactly(
                ElementChange(type, WallType(WallRole.PARTITION)),
                ElementChange(layer, WallLayer(type, position = 1, LayerFunction.INSULATION, 100.mm)),
                ElementChange(wall, Wall(level, PointMm(0.mm, 0.mm), PointMm(6000.mm, 0.mm), 0.mm, 2800.mm, type = type, mirrored = true)),
            ).inOrder()
        assertThat((decoded.results.single() as CommandResult.Failed).error).isEqualTo(EngineError.TYPE_DECIDES)
    }

    @Test
    fun aPickedSpaceARoomAndItsTextsComeBackTyped() {
        val room = ElementId(Uuid.parse("019a5f00-0000-7000-8000-000000000009"))
        val builder = FlatBufferBuilder(1024)
        RingTable.startPointsVector(builder, 3)
        PointMStruct.createPointM(builder, 2.0, 3.0)
        PointMStruct.createPointM(builder, 3.0, 2.0)
        PointMStruct.createPointM(builder, 2.0, 2.0)
        val island = RingTable.createRing(builder, builder.endVector())
        val islands = SpaceTable.createIslandsVector(builder, intArrayOf(island))
        SpaceTable.startOutlineVector(builder, 4)
        PointMStruct.createPointM(builder, 0.19, 3.81)
        PointMStruct.createPointM(builder, 5.81, 3.81)
        PointMStruct.createPointM(builder, 5.81, 0.19)
        PointMStruct.createPointM(builder, 0.19, 0.19)
        val outline = builder.endVector()
        SpaceTable.startPickedSpace(builder)
        SpaceTable.addRoom(builder, builder.uuid(room))
        SpaceTable.addKind(builder, RoomKindCode.Study)
        SpaceTable.addArea(builder, 19.8444)
        SpaceTable.addPerimeter(builder, 21.9)
        SpaceTable.addOutline(builder, outline)
        SpaceTable.addIslands(builder, islands)
        val space = SpaceTable.endPickedSpace(builder)
        ResultTable.startCommandResult(builder)
        ResultTable.addSpace(builder, space)
        val picked = ResultTable.endCommandResult(builder)

        TextTable.startDrawnText(builder)
        TextTable.addElement(builder, builder.uuid(room))
        TextTable.addKind(builder, TextKindCode.RoomName)
        TextTable.addValue(builder, RoomKindCode.Study.toLong())
        TextTable.addAt(builder, PointMStruct.createPointM(builder, 3.0, 2.15))
        TextTable.addAlong(builder, PointMStruct.createPointM(builder, 1.0, 0.0))
        TextTable.addFit(builder, 4.5)
        val name = TextTable.endDrawnText(builder)
        // A piece of wall that leads back from a corner 5.8 m along the wall to an opening.
        TextTable.startDrawnText(builder)
        TextTable.addElement(builder, builder.uuid(window))
        TextTable.addKind(builder, TextKindCode.Pier)
        TextTable.addValue(builder, 2_300)
        TextTable.addAt(builder, PointMStruct.createPointM(builder, 4.6, 0.6))
        TextTable.addAlong(builder, PointMStruct.createPointM(builder, 1.0, 0.0))
        TextTable.addFrom(builder, 5_800)
        TextTable.addForward(builder, true)
        val pier = TextTable.endDrawnText(builder)
        DrawnLineTable.startDrawnLine(builder)
        DrawnLineTable.addStroke(builder, StrokeCode.Chain)
        DrawnLineTable.addFrom(builder, PointMStruct.createPointM(builder, 0.2, 0.44))
        DrawnLineTable.addTo(builder, PointMStruct.createPointM(builder, 5.8, 0.44))
        val chain = DrawnLineTable.endDrawnLine(builder)
        val lines = DrawingTable.createLinesVector(builder, intArrayOf(chain))
        PolygonTable.startDrawnPolygon(builder)
        PolygonTable.addElement(builder, builder.uuid(room))
        PolygonTable.addFill(builder, FillCode.Floor)
        val floor = PolygonTable.endDrawnPolygon(builder)
        val polygons = DrawingTable.createPolygonsVector(builder, intArrayOf(floor))
        val texts = DrawingTable.createTextsVector(builder, intArrayOf(name, pier))
        DrawingTable.startDrawing(builder)
        DrawingTable.addPolygons(builder, polygons)
        DrawingTable.addLines(builder, lines)
        DrawingTable.addTexts(builder, texts)
        val drawing = DrawingTable.endDrawing(builder)
        ResultTable.startCommandResult(builder)
        ResultTable.addDrawing(builder, drawing)
        val drawn = ResultTable.endCommandResult(builder)

        RoomTable.startRoom(builder)
        RoomTable.addLevel(builder, builder.uuid(level))
        RoomTable.addAnchor(builder, PointStruct.createPointMm(builder, 1_000, 2_000))
        RoomTable.addKind(builder, RoomKindCode.Study)
        val value = RoomTable.endRoom(builder)
        ChangeTable.startElementChange(builder)
        ChangeTable.addId(builder, builder.uuid(room))
        ChangeTable.addElementType(builder, Element.Room)
        ChangeTable.addElement(builder, value)
        val change = ChangeTable.endElementChange(builder)
        IssueTable.startIssue(builder)
        IssueTable.addKind(builder, IssueKindCode.RoomSharesSpace)
        IssueTable.addElement(builder, builder.uuid(room))
        IssueTable.addLevel(builder, builder.uuid(level))
        IssueTable.addAt(builder, PointStruct.createPointMm(builder, 1_000, 2_000))
        val issue = IssueTable.endIssue(builder)
        val results = Response.createResultsVector(builder, intArrayOf(picked, drawn))
        val changes = Response.createChangesVector(builder, intArrayOf(change))
        val issues = Response.createIssuesVector(builder, intArrayOf(issue))
        Response.startResponse(builder)
        Response.addVersion(builder, 1L)
        Response.addResults(builder, results)
        Response.addChanges(builder, changes)
        Response.addIssues(builder, issues)
        builder.finish(Response.endResponse(builder), "OTVS")

        val decoded = ResponseDecoder.decode(builder.dataBuffer())

        assertThat((decoded.results[0] as CommandResult.Done).space)
            .isEqualTo(
                Space(
                    room = room,
                    kind = RoomKind.STUDY,
                    area = 19.8444,
                    perimeter = 21.9,
                    outline = listOf(PointM(0.19, 0.19), PointM(5.81, 0.19), PointM(5.81, 3.81), PointM(0.19, 3.81)),
                    islands = listOf(listOf(PointM(2.0, 2.0), PointM(3.0, 2.0), PointM(2.0, 3.0))),
                ),
            )
        val painted = (decoded.results[1] as CommandResult.Done).drawing
        assertThat(painted?.polygons?.single()?.fill).isEqualTo(FillKind.FLOOR)
        assertThat(painted?.texts)
            .containsExactly(
                DrawnText(room, TextKind.ROOM_NAME, RoomKind.STUDY.code.toLong(), PointM(3.0, 2.15), PointM(1.0, 0.0), fit = 4.5),
                DrawnText(window, TextKind.PIER, 2_300, PointM(4.6, 0.6), PointM(1.0, 0.0), from = 5_800.mm, forward = true),
            ).inOrder()
        assertThat(painted?.lines?.single()?.stroke).isEqualTo(StrokeKind.CHAIN)
        // The piece of wall is a handle for a typed value; the room's name is not.
        assertThat(painted?.texts?.map { it.isHandle }).containsExactly(false, true).inOrder()
        assertThat(decoded.changes).containsExactly(ElementChange(room, Room(level, PointMm(1_000.mm, 2_000.mm), RoomKind.STUDY)))
        assertThat(decoded.issues).containsExactly(Issue(IssueKind.ROOM_SHARES_SPACE, room, level, PointMm(1_000.mm, 2_000.mm)))
    }

    // A response like the engine's: one created wall, one failure, the wall's value and a deletion.
    private fun engineResponse(errorCode: Int): ByteBuffer {
        val builder = FlatBufferBuilder(1024)
        val message = builder.createString("wall not found")

        ResultTable.startCommandResult(builder)
        ResultTable.addCreated(builder, builder.uuid(wall))
        ResultTable.addPicked(builder, builder.uuid(window))
        val done = ResultTable.endCommandResult(builder)
        ResultTable.startCommandResult(builder)
        ResultTable.addError(builder, errorCode)
        ResultTable.addMessage(builder, message)
        val failed = ResultTable.endCommandResult(builder)

        WallTable.startWall(builder)
        WallTable.addLevel(builder, builder.uuid(level))
        WallTable.addStart(builder, PointStruct.createPointMm(builder, 0, 0))
        WallTable.addEnd(builder, PointStruct.createPointMm(builder, 6400, 0))
        WallTable.addThickness(builder, 380)
        WallTable.addHeight(builder, 2800)
        WallTable.addLocationLine(builder, LocationLineCode.RightFace)
        WallTable.addExact(builder, false)
        val wallValue = WallTable.endWall(builder)

        ChangeTable.startElementChange(builder)
        ChangeTable.addId(builder, builder.uuid(wall))
        ChangeTable.addElementType(builder, Element.Wall)
        ChangeTable.addElement(builder, wallValue)
        val changed = ChangeTable.endElementChange(builder)
        ChangeTable.startElementChange(builder)
        ChangeTable.addId(builder, builder.uuid(gone))
        val deleted = ChangeTable.endElementChange(builder)

        val response =
            Response.createResponse(
                builder,
                1L,
                Response.createResultsVector(builder, intArrayOf(done, failed)),
                Response.createChangesVector(builder, intArrayOf(changed, deleted)),
                true,
                false,
                0,
                0,
                ErrorCode.None,
                0,
                0,
                0,
                0,
            )
        builder.finish(response, "OTVS")
        return builder.dataBuffer()
    }

    // A response after a change: one wall's outline and a take-off, or the take-off's error.
    private fun derivedResponse(takeoffError: Int): ByteBuffer {
        val builder = FlatBufferBuilder(1024)
        OutlineTable.startPointsVector(builder, 3)
        for ((x, y) in listOf(6.4 to 0.19, 6.4 to -0.19, 0.0 to -0.19)) {
            PointMStruct.createPointM(builder, x, y)
        }
        val points = builder.endVector()
        OutlineTable.startWallOutline(builder)
        OutlineTable.addWall(builder, builder.uuid(wall))
        OutlineTable.addLevel(builder, builder.uuid(level))
        OutlineTable.addPoints(builder, points)
        OutlineTable.addLength(builder, 6400)
        val outline = OutlineTable.endWallOutline(builder)
        OpeningOutlineTable.startPointsVector(builder, 2)
        for ((x, y) in listOf(1.9 to -0.19, 1.0 to -0.19)) {
            PointMStruct.createPointM(builder, x, y)
        }
        val openingPoints = builder.endVector()
        OpeningOutlineTable.startOpeningOutline(builder)
        OpeningOutlineTable.addOpening(builder, builder.uuid(window))
        OpeningOutlineTable.addLevel(builder, builder.uuid(level))
        OpeningOutlineTable.addKind(builder, OpeningKindCode.Door)
        OpeningOutlineTable.addPoints(builder, openingPoints)
        val opening = OpeningOutlineTable.endOpeningOutline(builder)
        val line =
            LineTable.createQuantityLine(
                builder,
                builder.createString("masonry"),
                builder.createString("masonry_unit"),
                UnitCode.Piece,
                2767.0,
            )
        val lines = if (takeoffError == ErrorCode.None) intArrayOf(line) else intArrayOf()
        val message = builder.createString("masonry:3: boom")
        IssueTable.startIssue(builder)
        IssueTable.addKind(builder, IssueKindCode.ShortBearing)
        IssueTable.addElement(builder, builder.uuid(window))
        IssueTable.addLevel(builder, builder.uuid(level))
        val shortBearing = IssueTable.endIssue(builder)
        IssueTable.startIssue(builder)
        IssueTable.addKind(builder, IssueKindCode.WallOverlap)
        IssueTable.addLevel(builder, builder.uuid(level))
        IssueTable.addAt(builder, PointStruct.createPointMm(builder, 0, 0))
        val overlap = IssueTable.endIssue(builder)
        IssueTable.startIssue(builder)
        IssueTable.addKind(builder, 200)
        IssueTable.addLevel(builder, builder.uuid(level))
        val newer = IssueTable.endIssue(builder)
        val issues = Response.createIssuesVector(builder, intArrayOf(shortBearing, overlap, newer))
        val outlines = Response.createOutlinesVector(builder, intArrayOf(outline))
        val openings = Response.createOpeningsVector(builder, intArrayOf(opening))
        val takeoff = Response.createTakeoffVector(builder, lines)
        val results = Response.createResultsVector(builder, intArrayOf())
        Response.startResponse(builder)
        Response.addResults(builder, results)
        Response.addOutlines(builder, outlines)
        Response.addTakeoff(builder, takeoff)
        Response.addTakeoffError(builder, takeoffError)
        Response.addTakeoffMessage(builder, message)
        Response.addOpenings(builder, openings)
        Response.addIssues(builder, issues)
        builder.finish(Response.endResponse(builder), "OTVS")
        return builder.dataBuffer()
    }

    private fun FlatBufferBuilder.uuid(id: ElementId): Int = id.uuid.toLongs { high, low -> UuidStruct.createUuid(this, high, low) }
}
