package uz.otves.data.update

import android.content.Context
import android.os.SystemClock
import android.os.storage.StorageManager
import androidx.core.content.edit
import kotlinx.coroutines.CoroutineDispatcher
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.flow.Flow
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.asStateFlow
import kotlinx.coroutines.flow.conflate
import kotlinx.coroutines.flow.flowOn
import kotlinx.coroutines.flow.map
import kotlinx.coroutines.withContext
import kotlinx.serialization.SerializationException
import java.io.ByteArrayOutputStream
import java.io.File
import java.io.IOException
import java.net.HttpURLConnection
import java.net.URL

/**
 * Updates from the release feed at [feed], the address of the newest release's `update.json` (ADR 0018).
 * An empty [feed] is a build that never updates itself.
 */
public class AppUpdates(
    context: Context,
    private val feed: String,
    private val clock: () -> Long = SystemClock::elapsedRealtime,
    private val io: CoroutineDispatcher = Dispatchers.IO,
) : Updates {
    private val context = context.applicationContext
    private val installer = ApkInstaller(this.context)
    private val downloads = ApkDownload(updateFiles(this.context), ::freeSpace)
    private val preferences = this.context.getSharedPreferences(PREFERENCES, Context.MODE_PRIVATE)
    private val installed = installer.installedVersion

    private val state = MutableStateFlow<Release?>(null)
    override val required: StateFlow<Release?> = state.asStateFlow()

    // Milliseconds on [clock] when the feed last answered in this process.
    private var answeredAt: Long? = null

    override val expected: Boolean
        get() = feed.isNotEmpty() && preferences.getLong(NEWEST_SEEN, 0) > installed

    override suspend fun refresh() {
        if (feed.isEmpty()) return
        val last = answeredAt
        if (last != null && clock() - last < RECHECK_AFTER_MS) return
        val manifest =
            try {
                withContext(io) { Manifest.parse(read(feed)) }
            } catch (_: IOException) {
                return
            } catch (_: SerializationException) {
                return
            } catch (_: IllegalArgumentException) {
                return
            }
        answeredAt = clock()
        preferences.edit { putLong(NEWEST_SEEN, manifest.versionCode) }
        if (manifest.versionCode > installed) state.value = manifest.toRelease()
    }

    override fun mayInstall(): Boolean = installer.mayInstall()

    override fun download(release: Release): Flow<Download> =
        downloads
            .fetch(release)
            .map { step -> if (step is Download.Ready && !fitsOrDelete(step.apk, release)) Download.Corrupt else step }
            .flowOn(io)
            .conflate()

    private fun fitsOrDelete(
        apk: File,
        release: Release,
    ): Boolean = installer.fits(apk, release).also { fits -> if (!fits) apk.delete() }

    override suspend fun install(
        apk: File,
        systemDialog: Boolean,
    ): InstallResult = if (systemDialog) installer.askSystem(apk) else withContext(io) { installer.install(apk) }

    // Counts what Android can free from caches too, which a plain free-space figure leaves out.
    private fun freeSpace(): Long {
        val storage = context.getSystemService(StorageManager::class.java)
        return storage.getAllocatableBytes(storage.getUuidForPath(updateFiles(context).apply { mkdirs() }))
    }

    private fun read(address: String): String {
        val connection = URL(address).openConnection() as HttpURLConnection
        try {
            connection.connectTimeout = TIMEOUT_MS
            connection.readTimeout = TIMEOUT_MS
            if (connection.responseCode != HttpURLConnection.HTTP_OK) throw IOException("HTTP ${connection.responseCode}")
            return connection.inputStream.use { input ->
                val text = ByteArrayOutputStream()
                val buffer = ByteArray(8 * 1024)
                while (true) {
                    val read = input.read(buffer)
                    if (read < 0) break
                    text.write(buffer, 0, read)
                    if (text.size() > MANIFEST_LIMIT_BYTES) throw IOException("update.json is too large")
                }
                text.toString(Charsets.UTF_8.name())
            }
        } finally {
            connection.disconnect()
        }
    }

    internal companion object {
        const val PREFERENCES = "update"
        const val NEWEST_SEEN = "newest_seen_version_code"
        const val TIMEOUT_MS = 10_000
        const val MANIFEST_LIMIT_BYTES = 256 * 1024

        // A builder who leaves the app and comes back should meet a release that came out
        // meanwhile; the pause only keeps quick returns from asking again and again.
        const val RECHECK_AFTER_MS = 15 * 60 * 1000L

        /** Where the downloaded APK lives; Android may empty it when the phone runs out of space. */
        fun updateFiles(context: Context): File = File(context.cacheDir, "update")
    }
}
