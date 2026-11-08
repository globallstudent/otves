package uz.otves.data.project

import uz.otves.core.model.ElementId
import uz.otves.core.model.Mm
import uz.otves.engine.EngineElement
import uz.otves.engine.Level
import uz.otves.engine.LocationLine
import uz.otves.engine.LogEntry
import uz.otves.engine.Opening
import uz.otves.engine.OpeningKind
import uz.otves.engine.PointMm
import uz.otves.engine.Wall
import kotlin.uuid.Uuid

// Engine values to rows and back. Enum codes are those of schema/engine.fbs; RowMappingTest
// checks them against the generated schema.

internal const val CENTER = 0
internal const val LEFT_FACE = 1
internal const val RIGHT_FACE = 2
internal const val WINDOW = 0
internal const val DOOR = 1
internal const val EMPTY = 2

internal fun ElementId.key(): String = uuid.toString()

internal fun elementId(key: String): ElementId = ElementId(Uuid.parse(key))

internal fun Level.row(id: ElementId): LevelRow = LevelRow(id.key(), elevation.value, height.value)

internal fun Wall.row(id: ElementId): WallRow =
    WallRow(
        id = id.key(),
        levelId = level.key(),
        startX = start.x.value,
        startY = start.y.value,
        endX = end.x.value,
        endY = end.y.value,
        thickness = thickness.value,
        height = height.value,
        locationLine =
            when (locationLine) {
                LocationLine.CENTER -> CENTER
                LocationLine.LEFT_FACE -> LEFT_FACE
                LocationLine.RIGHT_FACE -> RIGHT_FACE
            },
    )

internal fun Opening.row(id: ElementId): OpeningRow =
    OpeningRow(
        id = id.key(),
        wallId = wall.key(),
        offset = offset.value,
        width = width.value,
        height = height.value,
        sill = sill.value,
        kind =
            when (kind) {
                OpeningKind.WINDOW -> WINDOW
                OpeningKind.DOOR -> DOOR
                OpeningKind.EMPTY -> EMPTY
            },
    )

internal fun LevelRow.element(): Pair<ElementId, EngineElement> = elementId(id) to Level(Mm(elevation), Mm(height))

internal fun WallRow.element(): Pair<ElementId, EngineElement> =
    elementId(id) to
        Wall(
            level = elementId(levelId),
            start = PointMm(Mm(startX), Mm(startY)),
            end = PointMm(Mm(endX), Mm(endY)),
            thickness = Mm(thickness),
            height = Mm(height),
            locationLine =
                when (locationLine) {
                    CENTER -> LocationLine.CENTER
                    LEFT_FACE -> LocationLine.LEFT_FACE
                    RIGHT_FACE -> LocationLine.RIGHT_FACE
                    else -> throw CorruptJobException("wall $id has location line $locationLine")
                },
        )

internal fun OpeningRow.element(): Pair<ElementId, EngineElement> =
    elementId(id) to
        Opening(
            wall = elementId(wallId),
            offset = Mm(offset),
            width = Mm(width),
            height = Mm(height),
            sill = Mm(sill),
            kind =
                when (kind) {
                    WINDOW -> OpeningKind.WINDOW
                    DOOR -> OpeningKind.DOOR
                    EMPTY -> OpeningKind.EMPTY
                    else -> throw CorruptJobException("opening $id has kind $kind")
                },
        )

internal fun LogEntry.row(): OpGroupRow =
    OpGroupRow(
        id = id.toString(),
        kind = kind,
        hlc = hlc,
        device = device.toString(),
        origin = origin?.toString(),
        ops = ops,
    )

/** A job file holds a value this app cannot read. */
public class CorruptJobException(
    message: String,
) : Exception(message)
