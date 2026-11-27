package uz.otves.feature.update

import androidx.compose.runtime.Immutable
import androidx.lifecycle.SavedStateHandle
import androidx.lifecycle.ViewModel
import androidx.lifecycle.viewModelScope
import kotlinx.coroutines.Job
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.asStateFlow
import kotlinx.coroutines.flow.update
import kotlinx.coroutines.launch
import uz.otves.data.update.Download
import uz.otves.data.update.InstallResult
import uz.otves.data.update.Release
import uz.otves.data.update.Updates
import java.io.File
import java.io.IOException

internal enum class UpdatePhase { Offered, Downloading, Installing }

internal enum class UpdateFailure { Network, Storage, Corrupt, Install }

@Immutable
internal data class UpdateUiState(
    /** Bytes of the APK. */
    val size: Long,
    val phase: UpdatePhase = UpdatePhase.Offered,
    /** Bytes on the phone so far. */
    val received: Long = 0,
    /** Why the last try stopped; the user may try again. */
    val failure: UpdateFailure? = null,
    /** Android has to be told to let the app install packages. */
    val askPermission: Boolean = false,
) {
    /** How far the update is, 0 to 1. */
    val progress: Float
        get() = if (phase == UpdatePhase.Installing || size <= 0) 1f else (received.toFloat() / size).coerceIn(0f, 1f)
}

internal class UpdateViewModel(
    private val updates: Updates,
    private val release: Release,
    private val saved: SavedStateHandle,
) : ViewModel() {
    // Android 11 stops the app when the permission is granted, so the question outlives the process.
    private val state = MutableStateFlow(UpdateUiState(size = release.apkSize, askPermission = saved[ASKING] ?: false))
    val uiState: StateFlow<UpdateUiState> = state.asStateFlow()

    private var work: Job? = null

    // The quiet install failed once; the next try goes through Android's installer screen.
    private var systemDialog = false

    /** The user asked to update, or to try again. */
    fun start() {
        if (work?.isActive == true) return
        if (!updates.mayInstall()) {
            ask(true)
            return
        }
        ask(false)
        work = viewModelScope.launch { downloadAndInstall() }
    }

    fun permissionDismissed() = ask(false)

    /** The screen is in front again, perhaps after Android's settings. */
    fun resumed() {
        if (state.value.askPermission && updates.mayInstall()) start()
    }

    private fun ask(asking: Boolean) {
        saved[ASKING] = asking
        state.update { it.copy(askPermission = asking) }
    }

    private suspend fun downloadAndInstall() {
        state.update { it.copy(phase = UpdatePhase.Downloading, failure = null) }
        var apk: File? = null
        try {
            updates.download(release).collect { step ->
                when (step) {
                    is Download.Progress -> state.update { it.copy(received = step.received) }
                    is Download.Ready -> apk = step.apk
                    Download.Corrupt -> stop(UpdateFailure.Corrupt, received = 0)
                    Download.NoSpace -> stop(UpdateFailure.Storage)
                }
            }
        } catch (_: IOException) {
            stop(UpdateFailure.Network)
        }
        val file = apk ?: return
        state.update { it.copy(phase = UpdatePhase.Installing, received = release.apkSize) }
        val result = updates.install(file, systemDialog)
        // After Done Android stops the app to replace it; until then the screen keeps saying so.
        if (result == InstallResult.Cancelled || result == InstallResult.AskedSystem) {
            state.update { it.copy(phase = UpdatePhase.Offered) }
        } else if (result == InstallResult.Failed) {
            systemDialog = true
            stop(UpdateFailure.Install)
        }
    }

    private fun stop(
        failure: UpdateFailure,
        received: Long = state.value.received,
    ) = state.update { it.copy(phase = UpdatePhase.Offered, failure = failure, received = received) }

    private companion object {
        const val ASKING = "asking_permission"
    }
}
