package uz.otves.data.project

import androidx.room.AutoMigration
import androidx.room.Dao
import androidx.room.Database
import androidx.room.Insert
import androidx.room.Query
import androidx.room.RoomDatabase
import androidx.room.Transaction
import androidx.room.Upsert

/** One job file (docs/adr/0007). */
@Database(
    entities = [
        JobRow::class,
        LevelRow::class,
        WallRow::class,
        OpeningRow::class,
        ConditionsRow::class,
        OpGroupRow::class,
        WallTypeRow::class,
        WallLayerRow::class,
        RoomRow::class,
    ],
    version = 7,
    exportSchema = true,
    // Version 2 adds what the builder chose (docs/adr/0008): new columns, zero in old jobs.
    // Version 3 adds the conditions table (docs/adr/0009), empty in old jobs.
    // Version 4 adds the footing's section to level (docs/adr/0010), zero in old jobs.
    // Version 5 adds whether a wall's length is exact (docs/adr/0012), true in old jobs.
    // Version 6 adds wall types and their layers (docs/adr/0014): two tables, empty in old jobs,
    // whose walls have no type.
    // Version 7 adds the names of rooms (docs/adr/0015): a table, empty in old jobs.
    autoMigrations = [
        AutoMigration(from = 1, to = 2),
        AutoMigration(from = 2, to = 3),
        AutoMigration(from = 3, to = 4),
        AutoMigration(from = 4, to = 5),
        AutoMigration(from = 5, to = 6),
        AutoMigration(from = 6, to = 7),
    ],
)
internal abstract class ProjectDatabase : RoomDatabase() {
    abstract fun jobs(): JobDao
}

/** What one engine response changed: rows to write, ids to delete and log groups to append. */
internal data class Recorded(
    val levels: List<LevelRow>,
    val walls: List<WallRow>,
    val openings: List<OpeningRow>,
    val conditions: List<ConditionsRow>,
    val wallTypes: List<WallTypeRow>,
    val wallLayers: List<WallLayerRow>,
    val rooms: List<RoomRow>,
    val deleted: List<String>,
    val groups: List<OpGroupRow>,
)

@Dao
internal abstract class JobDao {
    @Query("SELECT * FROM job LIMIT 1")
    abstract suspend fun job(): JobRow?

    @Insert
    abstract suspend fun insertJob(job: JobRow)

    @Query("SELECT * FROM level")
    abstract suspend fun levels(): List<LevelRow>

    @Query("SELECT * FROM wall")
    abstract suspend fun walls(): List<WallRow>

    @Query("SELECT * FROM opening")
    abstract suspend fun openings(): List<OpeningRow>

    @Query("SELECT * FROM conditions")
    abstract suspend fun conditions(): List<ConditionsRow>

    @Upsert
    abstract suspend fun upsertConditions(rows: List<ConditionsRow>)

    @Query("DELETE FROM conditions WHERE id IN (:ids)")
    abstract suspend fun deleteConditions(ids: List<String>)

    @Query("SELECT * FROM wall_type")
    abstract suspend fun wallTypes(): List<WallTypeRow>

    @Query("SELECT * FROM wall_layer")
    abstract suspend fun wallLayers(): List<WallLayerRow>

    @Upsert
    abstract suspend fun upsertWallTypes(rows: List<WallTypeRow>)

    @Upsert
    abstract suspend fun upsertWallLayers(rows: List<WallLayerRow>)

    @Query("DELETE FROM wall_type WHERE id IN (:ids)")
    abstract suspend fun deleteWallTypes(ids: List<String>)

    @Query("DELETE FROM wall_layer WHERE id IN (:ids)")
    abstract suspend fun deleteWallLayers(ids: List<String>)

    @Query("SELECT * FROM room")
    abstract suspend fun rooms(): List<RoomRow>

    @Upsert
    abstract suspend fun upsertRooms(rows: List<RoomRow>)

    @Query("DELETE FROM room WHERE id IN (:ids)")
    abstract suspend fun deleteRooms(ids: List<String>)

    @Query("SELECT MAX(hlc) FROM op_group")
    abstract suspend fun newestStamp(): Long?

    @Query("SELECT * FROM op_group ORDER BY seq")
    abstract suspend fun groups(): List<OpGroupRow>

    @Upsert
    abstract suspend fun upsertLevels(rows: List<LevelRow>)

    @Upsert
    abstract suspend fun upsertWalls(rows: List<WallRow>)

    @Upsert
    abstract suspend fun upsertOpenings(rows: List<OpeningRow>)

    @Query("DELETE FROM level WHERE id IN (:ids)")
    abstract suspend fun deleteLevels(ids: List<String>)

    @Query("DELETE FROM wall WHERE id IN (:ids)")
    abstract suspend fun deleteWalls(ids: List<String>)

    @Query("DELETE FROM opening WHERE id IN (:ids)")
    abstract suspend fun deleteOpenings(ids: List<String>)

    @Insert
    abstract suspend fun insertGroups(rows: List<OpGroupRow>)

    @Query("UPDATE job SET updated_at = :at")
    abstract suspend fun touch(at: Long)

    @Query("UPDATE job SET name = :name, updated_at = :at")
    abstract suspend fun rename(
        name: String,
        at: Long,
    )

    // All or nothing, so the rows and the log never disagree.
    @Transaction
    open suspend fun record(
        change: Recorded,
        at: Long,
    ) {
        upsertLevels(change.levels)
        upsertWallTypes(change.wallTypes)
        upsertWallLayers(change.wallLayers)
        upsertWalls(change.walls)
        upsertOpenings(change.openings)
        upsertRooms(change.rooms)
        upsertConditions(change.conditions)
        // A deletion does not say what it deleted; an id is in one table at most.
        if (change.deleted.isNotEmpty()) {
            deleteOpenings(change.deleted)
            deleteRooms(change.deleted)
            deleteWalls(change.deleted)
            deleteLevels(change.deleted)
            deleteConditions(change.deleted)
            deleteWallLayers(change.deleted)
            deleteWallTypes(change.deleted)
        }
        insertGroups(change.groups)
        touch(at)
    }
}
