package uz.otves.data.project

import androidx.room.ColumnInfo
import androidx.room.Entity
import androidx.room.Index
import androidx.room.PrimaryKey

// Tables of a job file, schema version 1 (docs/adr/0007). Ids are canonical UUID text, lengths
// whole millimetres, enums the codes of schema/engine.fbs.

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
