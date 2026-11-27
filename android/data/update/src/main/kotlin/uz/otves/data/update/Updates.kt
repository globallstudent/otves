package uz.otves.data.update

import kotlinx.coroutines.flow.Flow
import kotlinx.coroutines.flow.StateFlow
import java.io.File

/** A version newer than the installed one, as the release feed describes it. */
public data class Release(
    public val versionCode: Long,
    public val versionName: String,
    public val apkUrl: String,
    /** Lower-case hex. */
    public val apkSha256: String,
    /** Bytes. */
    public val apkSize: Long,
    /** What changed, by language tag. */
    public val notes: Map<String, List<ReleaseNote>>,
) {
    /** The notes in [language], or in Russian when the release has none in it. */
    public fun notesIn(language: String): List<ReleaseNote> =
        notes[language] ?: notes[FALLBACK_LANGUAGE] ?: notes.values.firstOrNull().orEmpty()

    private companion object {
        const val FALLBACK_LANGUAGE = "ru"
    }
}

/** One change of a release. [icon] names a picture; the app draws a plain mark for a name it does not know. */
public data class ReleaseNote(
    public val icon: String,
    public val title: String,
    public val text: String,
)

/** A step of fetching a release's APK. */
public sealed interface Download {
    /** Bytes on the phone so far, of [total]. */
    public data class Progress(
        public val received: Long,
        public val total: Long,
    ) : Download

    /** The whole file, checked against the release and this app's signing key. */
    public data class Ready(
        public val apk: File,
    ) : Download

    /** The file did not match the release; it is deleted, so the next try starts over. */
    public data object Corrupt : Download

    /** The phone has less free space than the file and its installation need. */
    public data object NoSpace : Download
}

public enum class InstallResult {
    /** Android took the new version; it stops this app to replace it, so this is rarely seen. */
    Done,

    /** The user declined Android's question. */
    Cancelled,

    Failed,

    /** Android's own installer screen is open and reports nothing back. */
    AskedSystem,
}

/** The app's own releases: what is newest, and getting it onto the phone. */
public interface Updates {
    /** The release this phone has to install; null until a check finds one. */
    public val required: StateFlow<Release?>

    /** Whether an earlier run saw a newer version, so the first check is worth waiting for. */
    public val expected: Boolean

    /** Asks the release feed when a check is due. No answer leaves [required] as it was. */
    public suspend fun refresh()

    /** Whether the user has let this app install packages. */
    public fun mayInstall(): Boolean

    /** Fetches the release's APK, going on from a partial file. Fails with IOException when the connection does. */
    public fun download(release: Release): Flow<Download>

    /**
     * Hands [apk] to Android. With [systemDialog] Android's installer screen takes it over,
     * which works where the quiet way does not.
     */
    public suspend fun install(
        apk: File,
        systemDialog: Boolean,
    ): InstallResult
}
