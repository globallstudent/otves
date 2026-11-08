package uz.otves.feature.projects

import androidx.compose.runtime.Immutable
import androidx.lifecycle.ViewModel
import androidx.lifecycle.viewModelScope
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.asStateFlow
import kotlinx.coroutines.flow.update
import kotlinx.coroutines.launch
import uz.otves.data.project.JobSummary
import uz.otves.data.project.Jobs
import kotlin.uuid.Uuid

@Immutable
internal data class ProjectsUiState(
    val loading: Boolean = true,
    /** Newest change first. */
    val jobs: List<JobSummary> = emptyList(),
    /** A job just created, for the screen to open once. */
    val created: Uuid? = null,
)

internal class ProjectsViewModel(
    private val jobs: Jobs,
) : ViewModel() {
    private val state = MutableStateFlow(ProjectsUiState())
    val uiState: StateFlow<ProjectsUiState> = state.asStateFlow()

    /** Reads the list again, such as when the user comes back from a job. */
    fun refresh() {
        viewModelScope.launch {
            val list = jobs.list()
            state.update { it.copy(loading = false, jobs = list) }
        }
    }

    fun create(name: String) {
        viewModelScope.launch {
            val job = jobs.create(name.trim())
            state.update { it.copy(jobs = listOf(job) + it.jobs, created = job.id) }
        }
    }

    fun createdOpened() = state.update { it.copy(created = null) }
}
