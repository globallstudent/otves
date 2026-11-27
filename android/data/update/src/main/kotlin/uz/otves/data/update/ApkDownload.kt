package uz.otves.data.update

import kotlinx.coroutines.currentCoroutineContext
import kotlinx.coroutines.ensureActive
import kotlinx.coroutines.flow.Flow
import kotlinx.coroutines.flow.FlowCollector
import kotlinx.coroutines.flow.flow
import java.io.File
import java.io.FileOutputStream
import java.io.IOException
import java.net.HttpURLConnection
import java.net.URL
import java.security.MessageDigest

/**
 * Fetches a release's APK into [directory], which holds nothing else.
 *
 * A broken connection leaves `<name>.part`; the next call asks the server for the rest.
 * [freeSpace] is the bytes [directory] can still take.
 */
internal class ApkDownload(
    private val directory: File,
    private val freeSpace: () -> Long,
) {
    fun fetch(release: Release): Flow<Download> =
        flow {
            directory.mkdirs()
            val apk = File(directory, "otves-${release.versionCode}.apk")
            val part = File(directory, "${apk.name}.part")
            directory.listFiles()?.filter { it != apk && it != part }?.forEach { it.delete() }

            if (apk.exists()) {
                if (matches(apk, release)) {
                    emit(Download.Ready(apk))
                    return@flow
                }
                apk.delete()
            }
            if (part.length() > release.apkSize) part.delete()

            // The installer copies the file once more before it replaces the app.
            val missing = release.apkSize - part.length()
            if (freeSpace() < missing + release.apkSize) {
                emit(Download.NoSpace)
                return@flow
            }

            receive(release, part)
            if (!matches(part, release)) {
                part.delete()
                emit(Download.Corrupt)
                return@flow
            }
            if (!part.renameTo(apk)) throw IOException("cannot rename ${part.name}")
            emit(Download.Ready(apk))
        }

    private suspend fun FlowCollector<Download>.receive(
        release: Release,
        part: File,
    ) {
        val connection = URL(release.apkUrl).openConnection() as HttpURLConnection
        try {
            connection.connectTimeout = CONNECT_TIMEOUT_MS
            connection.readTimeout = READ_TIMEOUT_MS
            val have = part.length()
            if (have > 0) connection.setRequestProperty("Range", "bytes=$have-")
            val resumed =
                when (val code = connection.responseCode) {
                    HttpURLConnection.HTTP_PARTIAL -> true

                    HttpURLConnection.HTTP_OK -> false

                    // The part is already the whole file, or longer than the server's.
                    HTTP_RANGE_NOT_SATISFIABLE -> return

                    else -> throw IOException("HTTP $code")
                }
            var received = if (resumed) have else 0L
            emit(Download.Progress(received, release.apkSize))
            connection.inputStream.use { input ->
                FileOutputStream(part, resumed).use { output ->
                    val buffer = ByteArray(BUFFER_BYTES)
                    while (true) {
                        currentCoroutineContext().ensureActive()
                        val read = input.read(buffer)
                        if (read < 0) break
                        output.write(buffer, 0, read)
                        received += read
                        emit(Download.Progress(received, release.apkSize))
                    }
                }
            }
        } finally {
            connection.disconnect()
        }
    }

    private fun matches(
        file: File,
        release: Release,
    ): Boolean = file.length() == release.apkSize && sha256(file) == release.apkSha256

    private companion object {
        const val CONNECT_TIMEOUT_MS = 15_000
        const val READ_TIMEOUT_MS = 30_000
        const val BUFFER_BYTES = 64 * 1024
        const val HTTP_RANGE_NOT_SATISFIABLE = 416
    }
}

/** Lower-case hex. */
internal fun sha256(file: File): String {
    val digest = MessageDigest.getInstance("SHA-256")
    file.inputStream().use { input ->
        val buffer = ByteArray(64 * 1024)
        while (true) {
            val read = input.read(buffer)
            if (read < 0) break
            digest.update(buffer, 0, read)
        }
    }
    return digest.digest().toHexString()
}
