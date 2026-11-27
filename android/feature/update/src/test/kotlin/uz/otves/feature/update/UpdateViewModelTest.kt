package uz.otves.feature.update

import androidx.lifecycle.SavedStateHandle
import com.google.common.truth.Truth.assertThat
import kotlinx.coroutines.CompletableDeferred
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.ExperimentalCoroutinesApi
import kotlinx.coroutines.flow.Flow
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.flow
import kotlinx.coroutines.test.UnconfinedTestDispatcher
import kotlinx.coroutines.test.resetMain
import kotlinx.coroutines.test.setMain
import org.junit.After
import org.junit.Before
import org.junit.Test
import uz.otves.data.update.Download
import uz.otves.data.update.InstallResult
import uz.otves.data.update.Release
import uz.otves.data.update.Updates
import java.io.File
import java.io.IOException

@OptIn(ExperimentalCoroutinesApi::class)
class UpdateViewModelTest {
    private class FakeUpdates : Updates {
        override val required: StateFlow<Release?> = MutableStateFlow(null)
        override val expected = false
        var allowed = true

        /** The steps of the next download; an exception among them breaks the connection there. */
        var steps: List<Any> = listOf(Download.Progress(0, SIZE), Download.Progress(SIZE, SIZE), Download.Ready(APK))
        var installs = InstallResult.Done
        val installed = mutableListOf<Boolean>()

        /** Holds the install open until a test completes it. */
        var installing: CompletableDeferred<Unit>? = null

        override suspend fun refresh() = Unit

        override fun mayInstall() = allowed

        override fun download(release: Release): Flow<Download> =
            flow {
                steps.forEach { step -> if (step is Exception) throw step else emit(step as Download) }
            }

        override suspend fun install(
            apk: File,
            systemDialog: Boolean,
        ): InstallResult {
            installed += systemDialog
            installing?.await()
            return installs
        }
    }

    private val updates = FakeUpdates()
    private val release = Release(101, "0.1.1", "https://example.org/otves.apk", "ab", SIZE, emptyMap())

    private fun viewModel(saved: SavedStateHandle = SavedStateHandle()) = UpdateViewModel(updates, release, saved)

    @Before
    fun setUp() = Dispatchers.setMain(UnconfinedTestDispatcher())

    @After
    fun tearDown() = Dispatchers.resetMain()

    @Test
    fun theUpdateWaitsToBeStarted() {
        val state = viewModel().uiState.value

        assertThat(state.phase).isEqualTo(UpdatePhase.Offered)
        assertThat(state.progress).isEqualTo(0f)
        assertThat(updates.installed).isEmpty()
    }

    @Test
    fun theFileIsFetchedThenInstalledQuietly() {
        updates.installing = CompletableDeferred()
        val viewModel = viewModel()

        viewModel.start()

        assertThat(viewModel.uiState.value.phase).isEqualTo(UpdatePhase.Installing)
        assertThat(viewModel.uiState.value.progress).isEqualTo(1f)
        assertThat(updates.installed).containsExactly(false)
    }

    @Test
    fun progressFollowsTheBytes() {
        updates.steps = listOf(Download.Progress(250, SIZE), IOException("gone"))
        val viewModel = viewModel()

        viewModel.start()

        assertThat(viewModel.uiState.value.received).isEqualTo(250)
        assertThat(viewModel.uiState.value.progress).isEqualTo(0.25f)
    }

    @Test
    fun aBrokenConnectionOffersAnotherTryAndKeepsItsPlace() {
        updates.steps = listOf(Download.Progress(250, SIZE), IOException("gone"))
        val viewModel = viewModel()
        viewModel.start()

        assertThat(viewModel.uiState.value.phase).isEqualTo(UpdatePhase.Offered)
        assertThat(viewModel.uiState.value.failure).isEqualTo(UpdateFailure.Network)
        assertThat(updates.installed).isEmpty()

        updates.steps = listOf(Download.Progress(250, SIZE), Download.Ready(APK))
        updates.installing = CompletableDeferred()
        viewModel.start()

        assertThat(viewModel.uiState.value.failure).isNull()
        assertThat(viewModel.uiState.value.phase).isEqualTo(UpdatePhase.Installing)
    }

    @Test
    fun aFileThatDoesNotMatchStartsOver() {
        updates.steps = listOf(Download.Progress(SIZE, SIZE), Download.Corrupt)
        val viewModel = viewModel()

        viewModel.start()

        assertThat(viewModel.uiState.value.failure).isEqualTo(UpdateFailure.Corrupt)
        assertThat(viewModel.uiState.value.received).isEqualTo(0)
    }

    @Test
    fun aFullPhoneIsReported() {
        updates.steps = listOf(Download.NoSpace)
        val viewModel = viewModel()

        viewModel.start()

        assertThat(viewModel.uiState.value.failure).isEqualTo(UpdateFailure.Storage)
        assertThat(updates.installed).isEmpty()
    }

    @Test
    fun withoutThePermissionTheUserIsAskedFirst() {
        updates.allowed = false
        val viewModel = viewModel()

        viewModel.start()

        assertThat(viewModel.uiState.value.askPermission).isTrue()
        assertThat(viewModel.uiState.value.phase).isEqualTo(UpdatePhase.Offered)
    }

    @Test
    fun theUpdateGoesOnWhenThePermissionIsGiven() {
        updates.allowed = false
        updates.installing = CompletableDeferred()
        val viewModel = viewModel()
        viewModel.start()

        viewModel.resumed()
        assertThat(viewModel.uiState.value.askPermission).isTrue()

        updates.allowed = true
        viewModel.resumed()

        assertThat(viewModel.uiState.value.askPermission).isFalse()
        assertThat(viewModel.uiState.value.phase).isEqualTo(UpdatePhase.Installing)
    }

    @Test
    fun theQuestionOutlivesTheProcess() {
        updates.allowed = false
        val saved = SavedStateHandle()
        viewModel(saved).start()

        updates.allowed = true
        updates.installing = CompletableDeferred()
        val restored = viewModel(saved)
        assertThat(restored.uiState.value.askPermission).isTrue()
        restored.resumed()

        assertThat(restored.uiState.value.phase).isEqualTo(UpdatePhase.Installing)
    }

    @Test
    fun aDismissedQuestionLeavesTheOffer() {
        updates.allowed = false
        val viewModel = viewModel()
        viewModel.start()

        viewModel.permissionDismissed()
        updates.allowed = true
        viewModel.resumed()

        assertThat(viewModel.uiState.value.askPermission).isFalse()
        assertThat(viewModel.uiState.value.phase).isEqualTo(UpdatePhase.Offered)
    }

    @Test
    fun aDeclinedInstallLeavesTheOffer() {
        updates.installs = InstallResult.Cancelled
        val viewModel = viewModel()

        viewModel.start()

        assertThat(viewModel.uiState.value.phase).isEqualTo(UpdatePhase.Offered)
        assertThat(viewModel.uiState.value.failure).isNull()
    }

    @Test
    fun afterAFailedInstallTheNextTryUsesAndroidsScreen() {
        updates.installs = InstallResult.Failed
        val viewModel = viewModel()
        viewModel.start()

        assertThat(viewModel.uiState.value.failure).isEqualTo(UpdateFailure.Install)

        updates.installs = InstallResult.AskedSystem
        viewModel.start()

        assertThat(updates.installed).containsExactly(false, true).inOrder()
        assertThat(viewModel.uiState.value.phase).isEqualTo(UpdatePhase.Offered)
        assertThat(viewModel.uiState.value.failure).isNull()
    }

    @Test
    fun aSecondTapDoesNotStartASecondDownload() {
        updates.installing = CompletableDeferred()
        val viewModel = viewModel()

        viewModel.start()
        viewModel.start()

        assertThat(updates.installed).hasSize(1)
    }

    private companion object {
        const val SIZE = 1_000L
        val APK = File("otves-101.apk")
    }
}
