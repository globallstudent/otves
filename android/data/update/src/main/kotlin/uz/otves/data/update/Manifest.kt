package uz.otves.data.update

import kotlinx.serialization.Serializable
import kotlinx.serialization.json.Json

// update.json of a release, written by .github/workflows/release.yml (ADR 0018).
@Serializable
internal data class Manifest(
    val versionCode: Long,
    val versionName: String,
    val apk: Apk,
    val notes: Map<String, List<Note>> = emptyMap(),
) {
    @Serializable
    data class Apk(
        val url: String,
        val sha256: String,
        val size: Long,
    )

    @Serializable
    data class Note(
        val icon: String = "",
        val title: String,
        val text: String = "",
    )

    fun toRelease(): Release =
        Release(
            versionCode = versionCode,
            versionName = versionName,
            apkUrl = apk.url,
            apkSha256 = apk.sha256.lowercase(),
            apkSize = apk.size,
            notes = notes.mapValues { (_, list) -> list.map { ReleaseNote(it.icon, it.title, it.text) } },
        )

    companion object {
        // A later release may add fields; an older app must still read the ones it knows.
        private val format = Json { ignoreUnknownKeys = true }

        /** Fails with SerializationException or IllegalArgumentException on text that is not a manifest. */
        fun parse(text: String): Manifest = format.decodeFromString(serializer(), text)
    }
}
