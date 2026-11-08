package uz.otves.data.project

import androidx.room.Dao
import androidx.room.Database
import androidx.room.Insert
import androidx.room.Query
import androidx.room.RoomDatabase
import androidx.room.Transaction
import androidx.room.Upsert

/** One job file (docs/adr/0007). */
@Database(
    entities = [JobRow::class, LevelRow::class, WallRow::class, OpeningRow::class, OpGroupRow::class],
    version = 1,
    exportSchema = true,
)
internal abstract class ProjectDatabase : RoomDatabase() {
    abstract fun jobs(): JobDao
}

/** What one engine response changed: rows to write, ids to delete and log groups to append. */
internal data class Recorded(
    val levels: List<LevelRow>,
    val walls: List<WallRow>,
    val openings: List<OpeningRow>,
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
        upsertWalls(change.walls)
        upsertOpenings(change.openings)
        // A deletion does not say what it deleted; an id is in one table at most.
        if (change.deleted.isNotEmpty()) {
            deleteOpenings(change.deleted)
            deleteWalls(change.deleted)
            deleteLevels(change.deleted)
        }
        insertGroups(change.groups)
        touch(at)
    }
}
