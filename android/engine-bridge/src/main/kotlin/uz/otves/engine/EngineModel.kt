package uz.otves.engine

import uz.otves.core.model.ElementId
import uz.otves.core.model.Mm

/** A point on the plan: x to the right, y up. */
public data class PointMm(
    public val x: Mm,
    public val y: Mm,
)

/** Which line of a wall its points lie on, looking from its start to its end. */
public enum class LocationLine { CENTER, LEFT_FACE, RIGHT_FACE }

public enum class OpeningKind { WINDOW, DOOR, EMPTY }

public enum class WallEnd { START, END }

/** A point of geometry the engine derived, in metres: for drawing, never for measuring. */
public data class PointM(
    public val x: Double,
    public val y: Double,
)

/** A wall's plan outline after joining, counter-clockwise. */
public data class WallOutline(
    public val wall: ElementId,
    public val level: ElementId,
    public val points: List<PointM>,
)

/** The rectangle an opening cuts through its wall on the plan, counter-clockwise. */
public data class OpeningOutline(
    public val opening: ElementId,
    public val level: ElementId,
    public val kind: OpeningKind,
    public val points: List<PointM>,
)

/** A Luau file from content/: a norm table or a rule (docs/adr/0005). */
public data class ContentScript(
    public val name: String,
    public val source: String,
)

public enum class QuantityUnit { PIECE, METRE, SQUARE_METRE, CUBIC_METRE, KILOGRAM }

/** A material line from a content rule, such as `masonry_unit` or `lintel.bar.l22`. */
public data class QuantityLine(
    public val rule: String,
    public val item: String,
    public val unit: QuantityUnit,
    public val amount: Double,
)

/** A model element as the engine reports it (engine/core). */
public sealed interface EngineElement

/** A storey; walls stand on it and measure their height from its floor. */
public data class Level(
    public val elevation: Mm,
    public val height: Mm,
) : EngineElement

public data class Wall(
    public val level: ElementId,
    public val start: PointMm,
    public val end: PointMm,
    public val thickness: Mm,
    public val height: Mm,
    public val locationLine: LocationLine = LocationLine.CENTER,
) : EngineElement

/** A window, door or empty opening, placed along its wall from the start and up from the base. */
public data class Opening(
    public val wall: ElementId,
    public val offset: Mm,
    public val width: Mm,
    public val height: Mm,
    public val sill: Mm,
    public val kind: OpeningKind = OpeningKind.WINDOW,
) : EngineElement
