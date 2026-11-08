package uz.otves.engine

import uz.otves.core.model.ElementId
import uz.otves.core.model.Mm
import uz.otves.schema.Element
import uz.otves.schema.ErrorCode
import uz.otves.schema.Response
import java.nio.ByteBuffer
import kotlin.uuid.Uuid
import uz.otves.schema.CommandResult as ResultTable
import uz.otves.schema.ElementChange as ChangeTable
import uz.otves.schema.Level as LevelTable
import uz.otves.schema.LocationLine as LocationLineCode
import uz.otves.schema.Opening as OpeningTable
import uz.otves.schema.OpeningKind as OpeningKindCode
import uz.otves.schema.PointMm as PointStruct
import uz.otves.schema.Uuid as UuidStruct
import uz.otves.schema.Wall as WallTable

/**
 * Reads schema `Response` buffers (schema/engine.fbs). The engine ships with the app, so a value
 * this app cannot read is a bug and throws; only error codes may come from a newer engine.
 */
internal object ResponseDecoder {
    private const val VERSION = 1L
    private const val IDENTIFIER = "OTVS"
    private const val IDENTIFIER_OFFSET = 4

    fun decode(buffer: ByteBuffer): EngineResponse {
        check(hasIdentifier(buffer)) { "The engine answered without the $IDENTIFIER identifier" }
        val response = Response.getRootAsResponse(buffer)
        check(response.version() == VERSION) { "The engine answered with version ${response.version()}" }
        return EngineResponse(
            results = List(response.resultsLength()) { result(response.results(it)) },
            changes = List(response.changesLength()) { change(response.changes(it)) },
            canUndo = response.canUndo(),
            canRedo = response.canRedo(),
        )
    }

    private fun hasIdentifier(buffer: ByteBuffer): Boolean {
        val start = buffer.position() + IDENTIFIER_OFFSET
        return buffer.limit() >= start + IDENTIFIER.length &&
            IDENTIFIER.indices.all { buffer.get(start + it) == IDENTIFIER[it].code.toByte() }
    }

    private fun result(result: ResultTable): CommandResult =
        when (val error = error(result.error())) {
            null -> CommandResult.Done(created = result.created()?.let(::id))
            else -> CommandResult.Failed(error = error, message = result.message().orEmpty())
        }

    private fun change(change: ChangeTable): ElementChange {
        val id = id(checkNotNull(change.id()) { "An element change has no id" })
        val element =
            when (change.elementType()) {
                Element.NONE -> null
                Element.Level -> level(change.element(LevelTable()) as LevelTable)
                Element.Wall -> wall(change.element(WallTable()) as WallTable)
                Element.Opening -> opening(change.element(OpeningTable()) as OpeningTable)
                else -> error("The engine sent an unknown element type ${change.elementType()}")
            }
        return ElementChange(id = id, element = element)
    }

    private fun id(uuid: UuidStruct): ElementId = ElementId(Uuid.fromLongs(uuid.high(), uuid.low()))

    private fun point(point: PointStruct?): PointMm {
        checkNotNull(point) { "A wall has no point" }
        return PointMm(Mm(point.x()), Mm(point.y()))
    }

    private fun level(level: LevelTable): Level = Level(elevation = Mm(level.elevation()), height = Mm(level.height()))

    private fun wall(wall: WallTable): Wall =
        Wall(
            level = id(checkNotNull(wall.level()) { "A wall has no level" }),
            start = point(wall.start()),
            end = point(wall.end()),
            thickness = Mm(wall.thickness()),
            height = Mm(wall.height()),
            locationLine = locationLine(wall.locationLine()),
        )

    private fun opening(opening: OpeningTable): Opening =
        Opening(
            wall = id(checkNotNull(opening.wall()) { "An opening has no wall" }),
            offset = Mm(opening.offset()),
            width = Mm(opening.width()),
            height = Mm(opening.height()),
            sill = Mm(opening.sill()),
            kind = openingKind(opening.kind()),
        )

    private fun locationLine(code: Int): LocationLine =
        when (code) {
            LocationLineCode.Center -> LocationLine.CENTER
            LocationLineCode.LeftFace -> LocationLine.LEFT_FACE
            LocationLineCode.RightFace -> LocationLine.RIGHT_FACE
            else -> error("The engine sent an unknown location line $code")
        }

    private fun openingKind(code: Int): OpeningKind =
        when (code) {
            OpeningKindCode.Window -> OpeningKind.WINDOW
            OpeningKindCode.Door -> OpeningKind.DOOR
            OpeningKindCode.Empty -> OpeningKind.EMPTY
            else -> error("The engine sent an unknown opening kind $code")
        }

    /** Null for success. */
    private fun error(code: Int): EngineError? =
        when (code) {
            ErrorCode.None -> null
            ErrorCode.NotFound -> EngineError.NOT_FOUND
            ErrorCode.OffPlan -> EngineError.OFF_PLAN
            ErrorCode.ZeroLength -> EngineError.ZERO_LENGTH
            ErrorCode.ThicknessNotPositive -> EngineError.THICKNESS_NOT_POSITIVE
            ErrorCode.HeightNotPositive -> EngineError.HEIGHT_NOT_POSITIVE
            ErrorCode.WidthNotPositive -> EngineError.WIDTH_NOT_POSITIVE
            ErrorCode.OffsetNegative -> EngineError.OFFSET_NEGATIVE
            ErrorCode.SillNegative -> EngineError.SILL_NEGATIVE
            ErrorCode.BearingNegative -> EngineError.BEARING_NEGATIVE
            ErrorCode.UnitSizeNotPositive -> EngineError.UNIT_SIZE_NOT_POSITIVE
            ErrorCode.JointNegative -> EngineError.JOINT_NEGATIVE
            ErrorCode.RuleSyntax -> EngineError.RULE_SYNTAX
            ErrorCode.RuleFailed -> EngineError.RULE_FAILED
            ErrorCode.RuleOutput -> EngineError.RULE_OUTPUT
            ErrorCode.GestureOpen -> EngineError.GESTURE_OPEN
            ErrorCode.NothingToUndo -> EngineError.NOTHING_TO_UNDO
            ErrorCode.NothingToRedo -> EngineError.NOTHING_TO_REDO
            ErrorCode.CorruptData -> EngineError.CORRUPT_DATA
            ErrorCode.UnsupportedVersion -> EngineError.UNSUPPORTED_VERSION
            ErrorCode.Internal -> EngineError.INTERNAL
            else -> EngineError.UNKNOWN
        }
}
