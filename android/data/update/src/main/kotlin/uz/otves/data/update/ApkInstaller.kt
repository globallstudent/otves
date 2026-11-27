package uz.otves.data.update

import android.app.PendingIntent
import android.content.ActivityNotFoundException
import android.content.BroadcastReceiver
import android.content.Context
import android.content.Intent
import android.content.pm.PackageInfo
import android.content.pm.PackageInstaller
import android.content.pm.PackageManager
import android.os.Build
import android.util.Log
import androidx.core.content.FileProvider
import androidx.core.content.IntentCompat
import androidx.core.content.pm.PackageInfoCompat
import kotlinx.coroutines.CoroutineStart
import kotlinx.coroutines.async
import kotlinx.coroutines.coroutineScope
import kotlinx.coroutines.flow.MutableSharedFlow
import kotlinx.coroutines.flow.first
import kotlinx.coroutines.withTimeoutOrNull
import java.io.File
import java.io.IOException

/** Puts a downloaded APK of this app in place of the running one. */
internal class ApkInstaller(
    private val context: Context,
) {
    private val packages = context.packageManager

    val installedVersion: Long
        get() = PackageInfoCompat.getLongVersionCode(packages.getPackageInfo(context.packageName, 0))

    fun mayInstall(): Boolean = packages.canRequestPackageInstalls()

    /** Whether [apk] is this app at [release]'s version, signed with the installed app's key. */
    fun fits(
        apk: File,
        release: Release,
    ): Boolean {
        val archive = archiveInfo(apk) ?: return false
        if (archive.packageName != context.packageName) return false
        if (PackageInfoCompat.getLongVersionCode(archive) != release.versionCode) return false
        // Some Android 10 builds do not read an archive's signers; Android checks the key itself on install.
        val theirs = archive.signingInfo?.apkContentsSigners ?: return true
        val ours =
            packages
                .getPackageInfo(context.packageName, PackageManager.GET_SIGNING_CERTIFICATES)
                .signingInfo
                ?.apkContentsSigners
                ?: return true
        return theirs.any { it in ours }
    }

    private fun archiveInfo(apk: File): PackageInfo? =
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
            packages.getPackageArchiveInfo(
                apk.path,
                PackageManager.PackageInfoFlags.of(PackageManager.GET_SIGNING_CERTIFICATES.toLong()),
            )
        } else {
            @Suppress("DEPRECATION")
            packages.getPackageArchiveInfo(apk.path, PackageManager.GET_SIGNING_CERTIFICATES)
        }

    /** Installs without a question where Android allows it (12 and later), with its question elsewhere. */
    suspend fun install(apk: File): InstallResult {
        val installer = packages.packageInstaller
        // A session left by a run that was stopped holds a copy of an older file.
        installer.mySessions.forEach { stale ->
            try {
                installer.abandonSession(stale.sessionId)
            } catch (_: SecurityException) {
                // Already gone.
            }
        }
        val params =
            PackageInstaller.SessionParams(PackageInstaller.SessionParams.MODE_FULL_INSTALL).apply {
                setAppPackageName(context.packageName)
                setSize(apk.length())
                setInstallReason(PackageManager.INSTALL_REASON_USER)
                if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S) {
                    setRequireUserAction(PackageInstaller.SessionParams.USER_ACTION_NOT_REQUIRED)
                }
            }
        val status =
            try {
                val id = installer.createSession(params)
                installer.openSession(id).use { session ->
                    session.openWrite(apk.name, 0, apk.length()).use { output ->
                        apk.inputStream().use { it.copyTo(output) }
                        session.fsync(output)
                    }
                    coroutineScope {
                        // Listening starts before the commit, so its answer cannot be missed.
                        val answer = async(start = CoroutineStart.UNDISPATCHED) { InstallStatus.of(id) }
                        session.commit(statusSender(id))
                        withTimeoutOrNull(ANSWER_TIMEOUT_MS) { answer.await() }.also { answer.cancel() }
                    }
                }
            } catch (failure: IOException) {
                Log.w(TAG, "install session failed", failure)
                null
            } catch (failure: SecurityException) {
                Log.w(TAG, "install session refused", failure)
                null
            }
        return when (status) {
            PackageInstaller.STATUS_SUCCESS -> InstallResult.Done
            PackageInstaller.STATUS_FAILURE_ABORTED -> InstallResult.Cancelled
            else -> InstallResult.Failed
        }
    }

    /** Opens Android's installer screen on [apk]. */
    fun askSystem(apk: File): InstallResult {
        val uri = FileProvider.getUriForFile(context, "${context.packageName}.update", apk)
        val view =
            Intent(Intent.ACTION_VIEW)
                .setDataAndType(uri, APK_MIME_TYPE)
                .addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION or Intent.FLAG_ACTIVITY_NEW_TASK)
        return try {
            context.startActivity(view)
            InstallResult.AskedSystem
        } catch (failure: ActivityNotFoundException) {
            Log.w(TAG, "no installer screen", failure)
            InstallResult.Failed
        }
    }

    private fun statusSender(session: Int) =
        PendingIntent
            .getBroadcast(
                context,
                session,
                Intent(context, InstallStatusReceiver::class.java),
                // Android writes the status into the intent, so it cannot be immutable.
                PendingIntent.FLAG_UPDATE_CURRENT or PendingIntent.FLAG_MUTABLE,
            ).intentSender

    private companion object {
        const val TAG = "OtvesUpdate"
        const val APK_MIME_TYPE = "application/vnd.android.package-archive"

        // Play Protect may scan the file or ask the user before the answer comes.
        const val ANSWER_TIMEOUT_MS = 5 * 60_000L
    }
}

/** The answers Android sends about install sessions, for the coroutine that waits for one. */
internal object InstallStatus {
    private val answers = MutableSharedFlow<Pair<Int, Int>>(extraBufferCapacity = 8)

    fun report(
        session: Int,
        status: Int,
    ) {
        answers.tryEmit(session to status)
    }

    suspend fun of(session: Int): Int = answers.first { it.first == session }.second
}

/** Gets a session's status: shows Android's question when it has one, passes on the final answer. */
internal class InstallStatusReceiver : BroadcastReceiver() {
    override fun onReceive(
        context: Context,
        intent: Intent,
    ) {
        val session = intent.getIntExtra(PackageInstaller.EXTRA_SESSION_ID, -1)
        val status = intent.getIntExtra(PackageInstaller.EXTRA_STATUS, PackageInstaller.STATUS_FAILURE)
        if (status == PackageInstaller.STATUS_PENDING_USER_ACTION) {
            val question = IntentCompat.getParcelableExtra(intent, Intent.EXTRA_INTENT, Intent::class.java) ?: return
            context.startActivity(question.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK))
            return
        }
        if (status != PackageInstaller.STATUS_SUCCESS) {
            Log.w("OtvesUpdate", "install status $status: ${intent.getStringExtra(PackageInstaller.EXTRA_STATUS_MESSAGE)}")
        }
        InstallStatus.report(session, status)
    }
}
