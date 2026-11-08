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
import uz.otves.schema.LogEntry as LogEntryTable
import uz.otves.schema.Opening as OpeningTable
import uz.otves.schema.OpeningKind as OpeningKindCode
import uz.otves.schema.OpeningOutline as OpeningOutlineTable
import uz.otves.schema.PointM as PointMStruct
import uz.otves.schema.PointMm as PointStruct
import uz.otves.schema.QuantityLine as LineTable
import uz.otves.schema.Unit as UnitCode
import uz.otves.schema.Uuid as UuidStruct
import uz.otves.schema.Wall as WallTable
import uz.otves.schema.WallOutline as OutlineTable

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
            outlines = response.outlinesVector()?.let { List(response.outlinesLength()) { outline(response.outlines(it)) } },
            takeoff = takeoff(response),
            openings = response.openingsVector()?.let { List(response.openingsLength()) { opening(response.openings(it)) } },
            log = List(response.logLength()) { logEntry(response.log(it)) },
        )
    }

    // An absent vector means unchanged; an empty one means there is nothing.
    private fun takeoff(response: Response): Takeoff? {
        if (response.takeoffVector() == null) {
            return null
        }
        return when (val error = error(response.takeoffError())) {
            null -> Takeoff.Lines(List(response.takeoffLength()) { line(response.takeoff(it)) })
            else -> Takeoff.Failed(error = error, message = response.takeoffMessage().orEmpty())
        }
    }

    private fun outline(outline: OutlineTable): WallOutline =
        WallOutline(
            wall = id(checkNotNull(outline.wall()) { "An outline has no wall" }),
            level = id(checkNotNull(outline.level()) { "An outline has no level" }),
            points =
                List(outline.pointsLength()) {
                    val point = outline.points(PointMStruct(), it)
                    PointM(point.x(), point.y())
                },
        )

    private fun opening(outline: OpeningOutlineTable): OpeningOutline =
        OpeningOutline(
            opening = id(checkNotNull(outline.opening()) { "An opening outline has no opening" }),
            level = id(checkNotNull(outline.level()) { "An opening outline has no level" }),
            kind = openingKind(outline.kind()),
            points =
                List(outline.pointsLength()) {
                    val point = outline.points(PointMStruct(), it)
                    PointM(point.x(), point.y())
                },
        )

    private fun logEntry(entry: LogEntryTable): LogEntry {
        val ops = checkNotNull(entry.opsAsByteBuffer()) { "A log entry has no ops" }
        return LogEntry(
            id = uuid(checkNotNull(entry.id()) { "A log entry has no id" }),
            kind = entry.kind(),
            hlc = entry.hlc(),
            device = uuid(checkNotNull(entry.device()) { "A log entry has no device" }),
            origin = entry.origin()?.let(::uuid)?.takeIf { it != Uuid.NIL },
            ops = ByteArray(ops.remaining()).also { ops.get(it) },
        )
    }

    private fun line(line: LineTable): QuantityLine =
        QuantityLine(
            rule = line.rule().orEmpty(),
            item = line.item().orEmpty(),
            unit = unit(line.unit()),
            amount = line.amount(),
        )

    private fun unit(code: Int): QuantityUnit =
        when (code) {
            UnitCode.Piece -> QuantityUnit.PIECE
            UnitCode.Metre -> QuantityUnit.METRE
            UnitCode.SquareMetre -> QuantityUnit.SQUARE_METRE
            UnitCode.CubicMetre -> QuantityUnit.CUBIC_METRE
            UnitCode.Kilogram -> QuantityUnit.KILOGRAM
            else -> error("The engine sent an unknown unit $code")
        }

    private fun hasIdentifier(buffer: ByteBuffer): Boolean {
        val start = buffer.position() + IDENTIFIER_OFFSET
        return buffer.limit() >= start + IDENTIFIER.length &&
            IDENTIFIER.indices.all { buffer.get(start + it) == IDENTIFIER[it].code.toByte() }
    }

    private fun result(result: ResultTable): CommandResult =
        when (val error = error(result.error())) {
            null -> CommandResult.Done(created = result.created()?.let(::id), picked = result.picked()?.let(::id))
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

    private fun uuid(uuid: UuidStruct): Uuid = Uuid.fromLongs(uuid.high(), uuid.low())

    private fun id(uuid: UuidStruct): ElementId = ElementId(uuid(uuid))

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
            ErrorCode.NoWall -> EngineError.NO_WALL
            ErrorCode.DoesNotFit -> EngineError.DOES_NOT_FIT
            else -> EngineError.UNKNOWN
        }
}
