package uz.otves.data.catalog

import androidx.room.ColumnInfo
import androidx.room.Dao
import androidx.room.Database
import androidx.room.Entity
import androidx.room.PrimaryKey
import androidx.room.Query
import androidx.room.RoomDatabase
import androidx.room.Upsert
import kotlinx.coroutines.flow.Flow

/** The price of one unit of a take-off item, in minor units of an ISO 4217 currency. */
@Entity(tableName = "price")
internal data class PriceRow(
    @PrimaryKey val item: String,
    val minor: Long,
    val currency: String,
    /** Unix milliseconds of the last change, so an old price can be told from a fresh one. */
    @ColumnInfo(name = "updated_at") val updatedAt: Long,
)

@Dao
internal interface PriceDao {
    @Query("SELECT * FROM price")
    fun all(): Flow<List<PriceRow>>

    @Upsert
    suspend fun upsert(row: PriceRow)

    @Query("DELETE FROM price WHERE item = :item")
    suspend fun delete(item: String)
}

/** What is the same for every job on this phone. */
@Database(entities = [PriceRow::class], version = 1, exportSchema = true)
internal abstract class CatalogDatabase : RoomDatabase() {
    abstract fun prices(): PriceDao
}
