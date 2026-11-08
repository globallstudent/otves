package uz.otves.feature.projects

import com.google.common.truth.Truth.assertThat
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.ExperimentalCoroutinesApi
import kotlinx.coroutines.test.UnconfinedTestDispatcher
import kotlinx.coroutines.test.resetMain
import kotlinx.coroutines.test.setMain
import org.junit.After
import org.junit.Before
import org.junit.Test
import uz.otves.data.project.JobStore
import uz.otves.data.project.JobSummary
import uz.otves.data.project.Jobs
import kotlin.uuid.Uuid

@OptIn(ExperimentalCoroutinesApi::class)
class ProjectsViewModelTest {
    private class FakeJobs : Jobs {
        val saved =
            mutableListOf(JobSummary(Uuid.parse("019a5f00-0000-7000-8000-000000000001"), "Баня", 1_000L))

        override suspend fun list(): List<JobSummary> = saved.toList()

        override suspend fun create(name: String): JobSummary =
            JobSummary(Uuid.fromLongs(2, saved.size.toLong()), name, 2_000L).also { saved.add(0, it) }

        override suspend fun rename(
            id: Uuid,
            name: String,
        ) {
            val index = saved.indexOfFirst { it.id == id }
            saved[index] = saved[index].copy(name = name, updatedAt = 3_000L)
            saved.sortByDescending { it.updatedAt }
        }

        override suspend fun delete(id: Uuid) {
            saved.removeAll { it.id == id }
        }

        override fun open(id: Uuid): JobStore = error("not opened in these tests")
    }

    private val jobs = FakeJobs()

    @Before
    fun setUp() = Dispatchers.setMain(UnconfinedTestDispatcher())

    @After
    fun tearDown() = Dispatchers.resetMain()

    @Test
    fun theListLoadsOnRefresh() {
        val viewModel = ProjectsViewModel(jobs)
        assertThat(viewModel.uiState.value.loading).isTrue()

        viewModel.refresh()

        assertThat(viewModel.uiState.value.loading).isFalse()
        assertThat(
            viewModel.uiState.value.jobs
                .map { it.name },
        ).containsExactly("Баня")
    }

    @Test
    fun aRenamedJobLeadsTheListAndADeletedOneLeavesIt() {
        val viewModel = ProjectsViewModel(jobs)
        viewModel.refresh()
        viewModel.create("Забор")
        val bath =
            viewModel.uiState.value.jobs
                .last()
                .id

        viewModel.rename(bath, " Баня у реки ")
        assertThat(
            viewModel.uiState.value.jobs
                .map { it.name },
        ).containsExactly("Баня у реки", "Забор").inOrder()

        viewModel.delete(bath)
        assertThat(
            viewModel.uiState.value.jobs
                .map { it.name },
        ).containsExactly("Забор")
    }

    @Test
    fun aCreatedJobLeadsTheListAndOpensOnce() {
        val viewModel = ProjectsViewModel(jobs)
        viewModel.refresh()

        viewModel.create("  Дом на Чиланзаре ")

        val state = viewModel.uiState.value
        assertThat(state.jobs.map { it.name }).containsExactly("Дом на Чиланзаре", "Баня").inOrder()
        assertThat(state.created).isEqualTo(state.jobs.first().id)
        viewModel.createdOpened()
        assertThat(viewModel.uiState.value.created).isNull()
    }
}
