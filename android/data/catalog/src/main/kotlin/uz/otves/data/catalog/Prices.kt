package uz.otves.data.catalog

import android.content.Context
import androidx.room.Room
import androidx.sqlite.driver.bundled.BundledSQLiteDriver
import kotlinx.coroutines.CoroutineDispatcher
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.flow.Flow
import kotlinx.coroutines.flow.map
import uz.otves.core.model.Currency
import uz.otves.core.model.Money

/**
 * The builder's own price list: what he pays for one unit of a take-off item, such as a brick
 * (`masonry_unit.250x120x65`, per piece) or mortar (`mortar`, per m³). It is his for every job on
 * this phone; prices never ship with the app.
 */
public interface Prices {
    /** Every price he has entered, by take-off item. */
    public val all: Flow<Map<String, Money>>

    /** Sets the price of one unit of [item]; null forgets it. */
    public suspend fun set(
        item: String,
        price: Money?,
    )
}

/** The price list in the app-wide catalog database. */
public class CatalogPrices(
    context: Context,
    name: String = "catalog.db",
    private val now: () -> Long = System::currentTimeMillis,
    io: CoroutineDispatcher = Dispatchers.IO,
) : Prices,
    AutoCloseable {
    private val database: CatalogDatabase =
        Room
            .databaseBuilder<CatalogDatabase>(context.applicationContext, context.getDatabasePath(name).absolutePath)
            .setDriver(BundledSQLiteDriver())
            .setQueryCoroutineContext(io)
            .build()

    override val all: Flow<Map<String, Money>> =
        database.prices().all().map { rows ->
            // A price in a currency this app no longer knows is left out.
            rows
                .mapNotNull { row -> Currency.entries.find { it.name == row.currency }?.let { row.item to Money(row.minor, it) } }
                .toMap()
        }

    override suspend fun set(
        item: String,
        price: Money?,
    ) {
        if (price == null) {
            database.prices().delete(item)
        } else {
            database.prices().upsert(PriceRow(item, price.minor, price.currency.name, now()))
        }
    }

    override fun close(): Unit = database.close()
}
