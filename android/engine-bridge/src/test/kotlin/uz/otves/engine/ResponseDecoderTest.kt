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
import uz.otves.schema.CommandResult as ResultTable
import uz.otves.schema.ElementChange as ChangeTable
import uz.otves.schema.LocationLine as LocationLineCode
import uz.otves.schema.PointM as PointMStruct
import uz.otves.schema.PointMm as PointStruct
import uz.otves.schema.QuantityLine as LineTable
import uz.otves.schema.Unit as UnitCode
import uz.otves.schema.Uuid as UuidStruct
import uz.otves.schema.Wall as WallTable
import uz.otves.schema.WallOutline as OutlineTable

class ResponseDecoderTest {
    private val level = ElementId(Uuid.parse("019a5f00-0000-7000-8000-000000000001"))
    private val wall = ElementId(Uuid.parse("019a5f00-0000-7000-8000-000000000002"))
    private val gone = ElementId(Uuid.parse("019a5f00-0000-7000-8000-000000000003"))

    @Test
    fun resultsAndChangesComeBackTyped() {
        val decoded = ResponseDecoder.decode(engineResponse(errorCode = ErrorCode.NotFound))

        assertThat(decoded.results)
            .containsExactly(
                CommandResult.Done(created = wall),
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
            .containsExactly(WallOutline(wall, level, listOf(PointM(0.0, -0.19), PointM(6.4, -0.19), PointM(6.4, 0.19))))
        assertThat(decoded.takeoff)
            .isEqualTo(Takeoff.Lines(listOf(QuantityLine("masonry", "masonry_unit", QuantityUnit.PIECE, 2767.0))))
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

    // A response like the engine's: one created wall, one failure, the wall's value and a deletion.
    private fun engineResponse(errorCode: Int): ByteBuffer {
        val builder = FlatBufferBuilder(1024)
        val message = builder.createString("wall not found")

        ResultTable.startCommandResult(builder)
        ResultTable.addCreated(builder, builder.uuid(wall))
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
        val outline = OutlineTable.endWallOutline(builder)
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
        val outlines = Response.createOutlinesVector(builder, intArrayOf(outline))
        val takeoff = Response.createTakeoffVector(builder, lines)
        val results = Response.createResultsVector(builder, intArrayOf())
        Response.startResponse(builder)
        Response.addResults(builder, results)
        Response.addOutlines(builder, outlines)
        Response.addTakeoff(builder, takeoff)
        Response.addTakeoffError(builder, takeoffError)
        Response.addTakeoffMessage(builder, message)
        builder.finish(Response.endResponse(builder), "OTVS")
        return builder.dataBuffer()
    }

    private fun FlatBufferBuilder.uuid(id: ElementId): Int = id.uuid.toLongs { high, low -> UuidStruct.createUuid(this, high, low) }
}
