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

/** Which way a wall goes from the wall before it in a run. */
public enum class Turn { LEFT, STRAIGHT, RIGHT }

/** The aimed point lined up with a wall end: a line from that end to the point. */
public data class Guide(
    public val from: PointMm,
    public val to: PointMm,
)

/** Where a wall being drawn would end, as the engine sees the plan (engine/geom/editing.h). */
public data class Aimed(
    public val point: PointMm,
    /** From the wall's start along its line; zero for a first point. */
    public val length: Mm,
    public val guides: List<Guide> = emptyList(),
    /** The point is a wall end or lies in a wall, so a wall ending here joins the plan. */
    public val meets: Boolean = false,
)

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
    /** Along the location line, for dimensions. */
    public val length: Mm = Mm(0),
)

/** The rectangle an opening cuts through its wall on the plan, counter-clockwise. */
public data class OpeningOutline(
    public val opening: ElementId,
    public val level: ElementId,
    public val kind: OpeningKind,
    public val points: List<PointM>,
)

/** What a polygon of a drawing is filled as (docs/adr/0013); the painter maps each to a paint. */
public enum class FillKind {
    NONE,

    /** Masonry. */
    WALL,

    /** A wall with no material yet. */
    UNCHOSEN,

    /** What an opening leaves of its wall. */
    GAP,

    /** A fill from a newer engine than this app knows; it is not painted. */
    UNKNOWN,
}

/** What a line of a drawing is drawn as. */
public enum class StrokeKind {
    NONE,
    WALL_EDGE,

    /** A wall of the level below. */
    BELOW,

    /** A window's faces and glazing. */
    FRAME,

    /** A door's leaf and swing. */
    LEAF,

    /** A stroke from a newer engine than this app knows; it is not painted. */
    UNKNOWN,
}

public enum class TextKind {
    /** A wall's length. */
    LENGTH,

    /** A length nobody has typed or measured. */
    ABOUT_LENGTH,

    /** A text from a newer engine than this app knows; it is not written. */
    UNKNOWN,
}

/** A filled outline of a drawing. [element] is what it draws, null for what cannot be picked. */
public data class DrawnPolygon(
    public val element: ElementId?,
    public val fill: FillKind,
    /** Of its outline. */
    public val stroke: StrokeKind,
    public val points: List<PointM>,
)

public data class DrawnLine(
    public val element: ElementId?,
    public val stroke: StrokeKind,
    public val from: PointM,
    public val to: PointM,
)

/** Around [centre] from [from] to [to], counter-clockwise on the plan; both lie at one radius. */
public data class DrawnArc(
    public val element: ElementId?,
    public val stroke: StrokeKind,
    public val centre: PointM,
    public val from: PointM,
    public val to: PointM,
)

/**
 * A number written with its middle at [at], along the unit direction [along], which never points
 * to the left or straight down.
 */
public data class DrawnText(
    public val element: ElementId?,
    public val kind: TextKind,
    public val value: Mm,
    public val at: PointM,
    public val along: PointM,
)

/**
 * One level as a display list (docs/adr/0013), in plan metres: what to draw and where. It is
 * painted in the order of its lists, each in the order given; nothing in it is to be moved.
 */
public data class Drawing(
    public val polygons: List<DrawnPolygon> = emptyList(),
    public val lines: List<DrawnLine> = emptyList(),
    public val arcs: List<DrawnArc> = emptyList(),
    public val texts: List<DrawnText> = emptyList(),
)

/** What is wrong with an opening or where walls meet; the app shows translated text for each. */
public enum class IssueKind {
    /** Past the wall's ends, below its base or above its top. */
    OPENING_OUTSIDE_WALL,

    /** Over another opening in the same wall. */
    OPENING_OVERLAP,
    OPENING_WITHOUT_WALL,

    /** The lintel runs past the wall's end or into another opening. */
    SHORT_BEARING,

    /** The lintel rises above the wall's top. */
    NO_ROOM_FOR_LINTEL,

    /** Walls along one line from a point, or one inside another. */
    WALL_OVERLAP,

    /** Walls meet at so small an angle that their ends are cut square. */
    SHARP_ANGLE,

    /** The wall has no masonry unit, so it is not in the take-off. */
    MATERIAL_NOT_CHOSEN,

    /** The norm asks for a ring beam on the storey's load-bearing walls; names the level only. */
    RING_BEAM_MISSING,

    /** The norm asks for meshes where the storey's load-bearing walls meet; names the level only. */
    JUNCTION_MESHES_MISSING,

    /** A kind from a newer engine than this app knows. */
    UNKNOWN,
}

/**
 * Something to fix: an opening or material issue names the [element]; a wall join issue has its
 * point [at].
 */
public data class Issue(
    public val kind: IssueKind,
    public val element: ElementId?,
    public val level: ElementId,
    public val at: PointMm?,
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

/**
 * A storey; walls stand on it and measure their height from its floor. [ringBeam] is the height of
 * the ring beam on its load-bearing walls, zero for none; [junctionMeshes] asks for meshes where
 * they meet. The take-off counts only what is chosen here.
 */
public data class Level(
    public val elevation: Mm,
    public val height: Mm,
    public val ringBeam: Mm = Mm(0),
    public val junctionMeshes: Boolean = false,
    /** The strip footing under the storey's load-bearing walls (docs/adr/0010); null for none. */
    public val footing: Footing? = null,
) : EngineElement

/** The section of a strip footing: how wide and how deep it is cast. */
public data class Footing(
    public val width: Mm,
    public val depth: Mm,
)

/**
 * What the job is estimated under (docs/adr/0009); a null value is left to the norm default.
 * Grades are their numbers (400 for ПЦ400, 100 for М100), waste allowances per mille.
 */
public data class Conditions(
    public val seismicIntensity: Int? = null,
    public val cementGrade: Int? = null,
    public val mortarGrade: Int? = null,
    public val unitWaste: Int? = null,
    public val mortarWaste: Int? = null,
) : EngineElement {
    public operator fun get(condition: Condition): Int? =
        when (condition) {
            Condition.SEISMIC_INTENSITY -> seismicIntensity
            Condition.CEMENT_GRADE -> cementGrade
            Condition.MORTAR_GRADE -> mortarGrade
            Condition.UNIT_WASTE -> unitWaste
            Condition.MORTAR_WASTE -> mortarWaste
        }
}

public enum class Condition { SEISMIC_INTENSITY, CEMENT_GRADE, MORTAR_GRADE, UNIT_WASTE, MORTAR_WASTE }

/** A brick or block as laid: [length] along the wall, [width] across it, [height] of one course. */
public data class UnitSize(
    public val length: Mm,
    public val width: Mm,
    public val height: Mm,
)

public data class Wall(
    public val level: ElementId,
    public val start: PointMm,
    public val end: PointMm,
    public val thickness: Mm,
    public val height: Mm,
    public val locationLine: LocationLine = LocationLine.CENTER,
    /** What the wall is laid from; null until the builder chooses. */
    public val unit: UnitSize? = null,
    /**
     * Whether the length was typed or measured. A wall read from a sketch is not, until it gets a
     * length (docs/adr/0012).
     */
    public val exact: Boolean = true,
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
