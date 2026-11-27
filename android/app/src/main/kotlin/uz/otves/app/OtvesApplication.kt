package uz.otves.app

import android.app.Application
import androidx.core.content.edit
import uz.otves.data.catalog.CatalogPrices
import uz.otves.data.project.JobFiles
import uz.otves.data.update.AppUpdates
import uz.otves.data.update.Updates
import uz.otves.engine.EngineGateway
import kotlin.uuid.Uuid

class OtvesApplication : Application() {
    /** One engine for the whole process; the OS frees it with the process. */
    val engine: EngineGateway by lazy { EngineGateway(deviceId()) }

    val jobs: JobFiles by lazy { JobFiles(this) }

    /** The builder's price list, the same for every job. */
    val prices: CatalogPrices by lazy { CatalogPrices(this) }

    /** The app's own releases; every newer one has to be installed. */
    val updates: Updates by lazy { AppUpdates(this, BuildConfig.UPDATE_FEED) }

    // Stamps every operation this phone makes, so edits from two devices can be told apart.
    private fun deviceId(): Uuid {
        val preferences = getSharedPreferences(PREFERENCES, MODE_PRIVATE)
        preferences.getString(DEVICE_ID, null)?.let { return Uuid.parse(it) }
        return Uuid.random().also { id -> preferences.edit { putString(DEVICE_ID, id.toString()) } }
    }

    private companion object {
        const val PREFERENCES = "otves"
        const val DEVICE_ID = "device_id"
    }
}
