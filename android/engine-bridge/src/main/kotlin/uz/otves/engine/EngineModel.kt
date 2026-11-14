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

/** Which way from a point of the plan, looking at it. */
public enum class Hand { TOP, BOTTOM, LEFT, RIGHT }

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

    /** Masonry laid in front of the wall. */
    FACING,
    INSULATION,

    /** A space the walls close. */
    FLOOR,

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

    /** Where two layers of a wall meet. */
    LAYER,

    /** A dimension line, its ticks and the lines that lead to it. */
    DIMENSION,

    /** The same for the sizes of the space in focus. */
    CHAIN,

    /** A stroke from a newer engine than this app knows; it is not painted. */
    UNKNOWN,
}

public enum class TextKind {
    /** A wall's length. */
    LENGTH,

    /** A length nobody has typed or measured. */
    ABOUT_LENGTH,

    /** A room's name: the value is its [RoomKind]'s code in the schema. */
    ROOM_NAME,

    /** A space's floor: the value is its area in cm². */
    ROOM_AREA,

    /** The plan's size from outer face to outer face, in millimetres (docs/adr/0016). */
    OVERALL,

    /** The clear size of the space in focus, between two of its walls. */
    CLEAR,

    /** A piece of wall between a corner or an opening and the next opening. */
    PIER,

    /** An opening's width. */
    OPENING_WIDTH,

    /** A window's mark (docs/adr/0017): the value is its number among the marks of windows. */
    WINDOW_MARK,
    DOOR_MARK,

    /** The mark of an opening with nothing in it. */
    EMPTY_MARK,

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
 * to the left or straight down. The [value] is millimetres unless the [kind] says otherwise.
 * [fit] is the room there is for the text along its direction, in metres: a text wider than that
 * is left out. Zero for a text the engine fitted itself.
 *
 * A size is a handle (docs/adr/0016): [EngineCommand.SetDimension] gives it a typed value.
 * [element] is then what the value changes, and for a piece of wall [from] and [forward] say
 * where its fixed end lies; the app only hands them back. A clear size lies between two walls:
 * [element] at its far end, to the right or the top of the plan, moves, and [other] stays; the
 * app may swap them to move the other wall.
 */
public data class DrawnText(
    public val element: ElementId?,
    public val kind: TextKind,
    public val value: Long,
    public val at: PointM,
    public val along: PointM,
    public val fit: Double = 0.0,
    public val from: Mm = Mm(0),
    public val forward: Boolean = false,
    public val other: ElementId? = null,
) {
    /** Whether a typed value changes the model through this text. */
    public val isHandle: Boolean
        get() =
            element != null &&
                when (kind) {
                    TextKind.LENGTH, TextKind.ABOUT_LENGTH, TextKind.PIER, TextKind.OPENING_WIDTH -> true

                    TextKind.CLEAR -> other != null

                    TextKind.OVERALL,
                    TextKind.ROOM_NAME,
                    TextKind.ROOM_AREA,
                    TextKind.WINDOW_MARK,
                    TextKind.DOOR_MARK,
                    TextKind.EMPTY_MARK,
                    TextKind.UNKNOWN,
                    -> false
                }

    /** Whether the text is an opening's mark. */
    public val isMark: Boolean
        get() = kind == TextKind.WINDOW_MARK || kind == TextKind.DOOR_MARK || kind == TextKind.EMPTY_MARK
}

/**
 * The space under a picked point (docs/adr/0015): the floor the walls close there, with the
 * [room] that names it, if one does. Areas are in m² and lengths in metres.
 */
public data class Space(
    public val room: ElementId?,
    /** What that room is used as. */
    public val kind: RoomKind,
    /** The floor, less what stands free inside. */
    public val area: Double,
    /** Along the outline and round what stands free inside. */
    public val perimeter: Double,
    /** Counter-clockwise on the plan. */
    public val outline: List<PointM>,
    /** What stands free inside, each clockwise. */
    public val islands: List<List<PointM>> = emptyList(),
)

/** A line of a level's room schedule: a space, the room that names it, if one does, and its floor in m². */
public data class ListedRoom(
    public val room: ElementId?,
    public val kind: RoomKind,
    public val area: Double,
)

/**
 * A line of the job's opening schedule: a mark, as the [kind] of its text and its [number], the
 * size it stands for and how many openings of the job have it.
 */
public data class ListedOpening(
    public val kind: TextKind,
    public val number: Long,
    public val width: Mm,
    public val height: Mm,
    public val count: Int,
)

/**
 * One level as a display list (docs/adr/0013), in plan metres: what to draw and where. It is
 * painted in the order of its lists, each in the order given; nothing in it is to be moved. With
 * it come the schedules a sheet prints beside the plan: the level's [rooms] in reading order and
 * the job's [openings] by mark.
 */
public data class Drawing(
    public val polygons: List<DrawnPolygon> = emptyList(),
    public val lines: List<DrawnLine> = emptyList(),
    public val arcs: List<DrawnArc> = emptyList(),
    public val texts: List<DrawnText> = emptyList(),
    public val rooms: List<ListedRoom> = emptyList(),
    public val openings: List<ListedOpening> = emptyList(),
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

    /** A room's anchor lies where the walls close no space. */
    ROOM_WITHOUT_SPACE,

    /** A room's anchor lies in a space an older room names. */
    ROOM_SHARES_SPACE,

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
    /**
     * What kind of wall this is (docs/adr/0014); null for a wall with no type. A wall with a type
     * has [thickness] zero and no [unit]: its type decides them.
     */
    public val type: ElementId? = null,
    /**
     * The type's layers start from the wall's right face, looking from start to end, or from its
     * left one when this is set.
     */
    public val mirrored: Boolean = false,
) : EngineElement

/** What a wall is for. Exterior and bearing walls carry load; partitions do not. */
public enum class WallRole { EXTERIOR, BEARING, PARTITION }

/** A kind of wall the job builds with (docs/adr/0014); its layers are [WallLayer] elements. */
public data class WallType(
    public val role: WallRole,
) : EngineElement

/** What a layer of a wall does; only structure and facing are laid from a unit. */
public enum class LayerFunction { STRUCTURE, INSULATION, FACING, GAP }

/**
 * One layer of a wall [type]. Layers run from the wall's outer face in, in the order of their
 * [position]s.
 */
public data class WallLayer(
    public val type: ElementId,
    public val position: Int,
    public val function: LayerFunction,
    public val thickness: Mm,
    /** What a masonry layer is laid from; null until chosen, and always for insulation and gaps. */
    public val unit: UnitSize? = null,
) : EngineElement

/** A layer of a wall type that is about to be made: what it does, how thick it is, its unit. */
public data class LayerSpec(
    public val function: LayerFunction,
    public val thickness: Mm,
    public val unit: UnitSize? = null,
)

/** What a room is used as; the app writes the name in the builder's language. */
public enum class RoomKind {
    ROOM,
    LIVING_ROOM,
    KITCHEN_LIVING,
    KITCHEN,
    DINING,
    BEDROOM,
    CHILDREN,
    STUDY,
    HALL,
    CORRIDOR,

    /** A bath and a toilet in one room. */
    BATHROOM,
    BATH,
    TOILET,
    BOILER,
    STORAGE,
    WARDROBE,
    VERANDA,
    GARAGE,
    TAMBOUR,
}

/**
 * A name for a space the walls of [level] close (docs/adr/0015): the space that holds [anchor] is
 * this room.
 */
public data class Room(
    public val level: ElementId,
    public val anchor: PointMm,
    public val kind: RoomKind = RoomKind.ROOM,
) : EngineElement

/** The edge of an opening a door's leaf hangs on (docs/adr/0017): [NEAR] is towards the wall's start. */
public enum class Hinge { NEAR, FAR }

/** The side of its wall a door's leaf opens to, looking from the wall's start. */
public enum class Swing { LEFT, RIGHT }

/**
 * A window, door or empty opening, placed along its wall from the start and up from the base.
 * [hinge] and [swing] say how a door hangs; other openings keep them unused.
 */
public data class Opening(
    public val wall: ElementId,
    public val offset: Mm,
    public val width: Mm,
    public val height: Mm,
    public val sill: Mm,
    public val kind: OpeningKind = OpeningKind.WINDOW,
    public val hinge: Hinge = Hinge.NEAR,
    public val swing: Swing = Swing.LEFT,
) : EngineElement
