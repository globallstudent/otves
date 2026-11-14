package uz.otves.data.project

import androidx.room.ColumnInfo
import androidx.room.Entity
import androidx.room.Index
import androidx.room.PrimaryKey

// Tables of a job file, schema version 6 (docs/adr/0007 to 0010, 0012 and 0014). Ids are canonical
// UUID text, lengths whole millimetres, enums the codes of schema/engine.fbs.

@Entity(tableName = "job")
internal data class JobRow(
    @PrimaryKey val id: String,
    val name: String,
    @ColumnInfo(name = "created_at") val createdAt: Long,
    @ColumnInfo(name = "updated_at") val updatedAt: Long,
)

@Entity(tableName = "level")
internal data class LevelRow(
    @PrimaryKey val id: String,
    val elevation: Long,
    val height: Long,
    @ColumnInfo(name = "ring_beam", defaultValue = "0") val ringBeam: Long,
    @ColumnInfo(name = "junction_meshes", defaultValue = "0") val junctionMeshes: Boolean,
    // The strip footing's section; both zero for none.
    @ColumnInfo(name = "footing_width", defaultValue = "0") val footingWidth: Long,
    @ColumnInfo(name = "footing_depth", defaultValue = "0") val footingDepth: Long,
)

@Entity(tableName = "wall", indices = [Index("level_id")])
internal data class WallRow(
    @PrimaryKey val id: String,
    @ColumnInfo(name = "level_id") val levelId: String,
    @ColumnInfo(name = "start_x") val startX: Long,
    @ColumnInfo(name = "start_y") val startY: Long,
    @ColumnInfo(name = "end_x") val endX: Long,
    @ColumnInfo(name = "end_y") val endY: Long,
    val thickness: Long,
    val height: Long,
    @ColumnInfo(name = "location_line") val locationLine: Int,
    // The masonry unit as laid; all zero until chosen.
    @ColumnInfo(name = "unit_length", defaultValue = "0") val unitLength: Long,
    @ColumnInfo(name = "unit_width", defaultValue = "0") val unitWidth: Long,
    @ColumnInfo(name = "unit_height", defaultValue = "0") val unitHeight: Long,
    // Whether the length was typed or measured; a sketched wall is not until it gets one.
    @ColumnInfo(defaultValue = "1") val exact: Boolean,
    // The wall's type; null for a wall with no type. A wall with one has thickness 0 and no unit.
    @ColumnInfo(name = "type_id") val typeId: String?,
    // The type's layers start from the wall's left face instead of its right one.
    @ColumnInfo(defaultValue = "0") val mirrored: Boolean,
)

/** A kind of wall the job builds with (docs/adr/0014); its layers are in `wall_layer`. */
@Entity(tableName = "wall_type")
internal data class WallTypeRow(
    @PrimaryKey val id: String,
    val role: Int,
)

/** One layer of a wall type, from the wall's outer face in by [position]. */
@Entity(tableName = "wall_layer", indices = [Index("type_id")])
internal data class WallLayerRow(
    @PrimaryKey val id: String,
    @ColumnInfo(name = "type_id") val typeId: String,
    val position: Int,
    val function: Int,
    val thickness: Long,
    // The masonry unit as laid; all zero for insulation, gaps and layers with none chosen.
    @ColumnInfo(name = "unit_length") val unitLength: Long,
    @ColumnInfo(name = "unit_width") val unitWidth: Long,
    @ColumnInfo(name = "unit_height") val unitHeight: Long,
)

/** A name for the space of a level that holds the point ([x], [y]) (docs/adr/0015). */
@Entity(tableName = "room", indices = [Index("level_id")])
internal data class RoomRow(
    @PrimaryKey val id: String,
    @ColumnInfo(name = "level_id") val levelId: String,
    val x: Long,
    val y: Long,
    // What the room is used as: the kind's code in schema/engine.fbs.
    val kind: Int,
)

@Entity(tableName = "opening", indices = [Index("wall_id")])
internal data class OpeningRow(
    @PrimaryKey val id: String,
    @ColumnInfo(name = "wall_id") val wallId: String,
    val offset: Long,
    val width: Long,
    val height: Long,
    val sill: Long,
    val kind: Int,
    // How a door hangs (docs/adr/0017): the edge its leaf is on, near (0) or far (1), and the
    // side of the wall it opens to, left (0) or right (1). Unused by other openings.
    @ColumnInfo(defaultValue = "0") val hinge: Int,
    @ColumnInfo(defaultValue = "0") val swing: Int,
)

/** The job's estimating conditions (docs/adr/0009): one row at most; null is left to the norm. */
@Entity(tableName = "conditions")
internal data class ConditionsRow(
    @PrimaryKey val id: String,
    @ColumnInfo(name = "seismic_intensity") val seismicIntensity: Int?,
    @ColumnInfo(name = "cement_grade") val cementGrade: Int?,
    @ColumnInfo(name = "mortar_grade") val mortarGrade: Int?,
    @ColumnInfo(name = "unit_waste") val unitWaste: Int?,
    @ColumnInfo(name = "mortar_waste") val mortarWaste: Int?,
)

/** One group of the operation log; [seq] keeps the order the engine appended them in. */
@Entity(tableName = "op_group", indices = [Index("id", unique = true)])
internal class OpGroupRow(
    @PrimaryKey(autoGenerate = true) val seq: Long = 0,
    val id: String,
    val kind: Int,
    val hlc: Long,
    val device: String,
    val origin: String?,
    val ops: ByteArray,
)
