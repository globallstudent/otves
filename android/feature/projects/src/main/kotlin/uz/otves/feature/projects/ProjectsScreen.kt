package uz.otves.feature.projects

import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.PaddingValues
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.safeDrawingPadding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.items
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.AlertDialog
import androidx.compose.material3.Button
import androidx.compose.material3.HorizontalDivider
import androidx.compose.material3.Icon
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.OutlinedTextField
import androidx.compose.material3.Surface
import androidx.compose.material3.Text
import androidx.compose.material3.TextButton
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.saveable.rememberSaveable
import androidx.compose.runtime.setValue
import androidx.compose.ui.Modifier
import androidx.compose.ui.res.stringResource
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.tooling.preview.Preview
import androidx.compose.ui.unit.dp
import androidx.lifecycle.compose.collectAsStateWithLifecycle
import androidx.lifecycle.viewmodel.compose.viewModel
import uz.otves.core.designsystem.OtvesDimens
import uz.otves.core.designsystem.OtvesIcons
import uz.otves.core.designsystem.OtvesTheme
import uz.otves.core.model.Currency
import uz.otves.core.model.Money
import uz.otves.data.project.JobCard
import uz.otves.data.project.JobSummary
import uz.otves.data.project.Jobs
import kotlin.uuid.Uuid

/** The jobs on this phone; [onOpen] opens one in the editor, also right after it is created. */
@Composable
fun ProjectsRoute(
    jobs: Jobs,
    onOpen: (Uuid) -> Unit,
    modifier: Modifier = Modifier,
) {
    val viewModel = viewModel { ProjectsViewModel(jobs) }
    val state by viewModel.uiState.collectAsStateWithLifecycle()
    // Each time the list shows again, a job may have changed.
    LaunchedEffect(Unit) { viewModel.refresh() }
    LaunchedEffect(state.created) {
        state.created?.let { created ->
            viewModel.createdOpened()
            onOpen(created)
        }
    }
    ProjectsScreen(
        state = state,
        onOpen = onOpen,
        onCreate = viewModel::create,
        onRename = viewModel::rename,
        onDelete = viewModel::delete,
        modifier = modifier,
    )
}

@Composable
internal fun ProjectsScreen(
    state: ProjectsUiState,
    onOpen: (Uuid) -> Unit,
    onCreate: (String) -> Unit,
    onRename: (Uuid, String) -> Unit,
    onDelete: (Uuid) -> Unit,
    modifier: Modifier = Modifier,
) {
    var naming by rememberSaveable { mutableStateOf(false) }
    // Ids as text, so they survive recreation.
    var actionsFor by rememberSaveable { mutableStateOf<String?>(null) }
    var renaming by rememberSaveable { mutableStateOf<String?>(null) }
    var deleting by rememberSaveable { mutableStateOf<String?>(null) }
    Surface(modifier = modifier.fillMaxSize(), color = OtvesTheme.colors.surface) {
        Column(modifier = Modifier.safeDrawingPadding()) {
            Text(
                text = stringResource(R.string.projects_title),
                style = MaterialTheme.typography.headlineMedium,
                fontWeight = FontWeight.SemiBold,
                modifier = Modifier.padding(start = OtvesDimens.gutter, end = OtvesDimens.gutter, top = 20.dp, bottom = 12.dp),
            )
            Box(modifier = Modifier.weight(1f).fillMaxWidth()) {
                when {
                    state.loading -> {
                        // Nothing until the list is read.
                    }

                    state.jobs.isEmpty() -> {
                        EmptyProjects()
                    }

                    else -> {
                        JobList(jobs = state.jobs, onOpen = onOpen, onActions = { actionsFor = it.toString() })
                    }
                }
            }
            HorizontalDivider(color = OtvesTheme.colors.divider)
            Button(
                onClick = { naming = true },
                enabled = !state.loading,
                shape = RoundedCornerShape(14.dp),
                modifier =
                    Modifier
                        .fillMaxWidth()
                        .padding(start = OtvesDimens.gutter, end = OtvesDimens.gutter, top = 12.dp, bottom = OtvesDimens.gutter)
                        .height(OtvesDimens.primaryTarget),
            ) {
                Icon(OtvesIcons.Plus, contentDescription = null, modifier = Modifier.size(20.dp))
                Spacer(Modifier.size(10.dp))
                Text(stringResource(R.string.projects_new), style = MaterialTheme.typography.titleMedium)
            }
        }
    }
    state.jobs.find { it.id.toString() == actionsFor }?.let { job ->
        JobActionsSheet(
            job = job,
            onRename = {
                actionsFor = null
                renaming = job.id.toString()
            },
            onDelete = {
                actionsFor = null
                deleting = job.id.toString()
            },
            onDismiss = { actionsFor = null },
        )
    }
    if (naming) {
        NameDialog(
            title = stringResource(R.string.projects_new),
            confirm = stringResource(R.string.projects_create),
            suggested = stringResource(R.string.projects_default_name, state.jobs.size + 1),
            onConfirm = { name ->
                naming = false
                onCreate(name)
            },
            onDismiss = { naming = false },
        )
    }
    state.jobs.find { it.id.toString() == renaming }?.let { job ->
        NameDialog(
            title = stringResource(R.string.projects_rename),
            confirm = stringResource(R.string.projects_save),
            suggested = job.name,
            onConfirm = { name ->
                renaming = null
                onRename(job.id, name)
            },
            onDismiss = { renaming = null },
        )
    }
    state.jobs.find { it.id.toString() == deleting }?.let { job ->
        AlertDialog(
            containerColor = OtvesTheme.colors.surface,
            onDismissRequest = { deleting = null },
            title = { Text(stringResource(R.string.projects_delete_title, job.name)) },
            text = { Text(stringResource(R.string.projects_delete_body)) },
            confirmButton = {
                TextButton(onClick = {
                    deleting = null
                    onDelete(job.id)
                }) { Text(stringResource(R.string.projects_delete)) }
            },
            dismissButton = {
                TextButton(onClick = { deleting = null }) { Text(stringResource(R.string.projects_cancel)) }
            },
        )
    }
}

// The job worked on last stands first, as a card with its plan; the others follow as rows.
@Composable
private fun JobList(
    jobs: List<JobSummary>,
    onOpen: (Uuid) -> Unit,
    onActions: (Uuid) -> Unit,
) {
    val latest = jobs.first()
    val others = jobs.drop(1)
    LazyColumn(contentPadding = PaddingValues(bottom = 12.dp)) {
        item(key = latest.id.toString()) {
            LatestJobCard(
                job = latest,
                onOpen = { onOpen(latest.id) },
                onActions = { onActions(latest.id) },
                modifier = Modifier.padding(horizontal = OtvesDimens.gutter),
            )
        }
        if (others.isNotEmpty()) {
            item(key = "all") {
                Text(
                    text = stringResource(R.string.projects_all),
                    style = MaterialTheme.typography.titleSmall,
                    color = OtvesTheme.colors.inkSecondary,
                    modifier = Modifier.padding(start = OtvesDimens.gutter, end = OtvesDimens.gutter, top = 24.dp, bottom = 4.dp),
                )
            }
            items(others, key = { it.id.toString() }) { job ->
                JobRow(job = job, onOpen = { onOpen(job.id) }, onActions = { onActions(job.id) })
            }
        }
    }
}

@Composable
private fun NameDialog(
    title: String,
    confirm: String,
    suggested: String,
    onConfirm: (String) -> Unit,
    onDismiss: () -> Unit,
) {
    var name by rememberSaveable { mutableStateOf(suggested) }
    AlertDialog(
        containerColor = OtvesTheme.colors.surface,
        onDismissRequest = onDismiss,
        title = { Text(title) },
        text = {
            OutlinedTextField(
                value = name,
                onValueChange = { name = it },
                label = { Text(stringResource(R.string.projects_name)) },
                singleLine = true,
            )
        },
        confirmButton = {
            TextButton(onClick = { onConfirm(name) }, enabled = name.isNotBlank()) {
                Text(confirm)
            }
        },
        dismissButton = {
            TextButton(onClick = onDismiss) { Text(stringResource(R.string.projects_cancel)) }
        },
    )
}

private fun previewJob(
    number: Long,
    name: String,
    updatedAt: Long,
    card: JobCard? = null,
) = JobSummary(Uuid.fromLongs(0x019a5f00_0000_7000L, number), name, updatedAt, card)

@Preview(widthDp = 390, heightDp = 844)
@Composable
private fun ProjectsScreenPreview() {
    OtvesTheme {
        ProjectsScreen(
            state =
                ProjectsUiState(
                    loading = false,
                    jobs =
                        listOf(
                            previewJob(1, "Дом на Чиланзаре", 1_762_600_000_000, JobCard(138.6, Money(4_860_000_000, Currency.UZS), null)),
                            previewJob(2, "Баня в Кибрае", 1_762_400_000_000, JobCard(24.0, Money(980_000_000, Currency.UZS), null)),
                            previewJob(3, "Объект 5", 1_762_300_000_000),
                            previewJob(4, "Забор у Рустама", 1_762_100_000_000, JobCard(0.0, Money(1_840_000_000, Currency.UZS), null)),
                        ),
                ),
            onOpen = {},
            onCreate = {},
            onRename = { _, _ -> },
            onDelete = {},
        )
    }
}

@Preview(widthDp = 390, heightDp = 844)
@Composable
private fun EmptyProjectsPreview() {
    OtvesTheme {
        ProjectsScreen(state = ProjectsUiState(loading = false), onOpen = {}, onCreate = {}, onRename = { _, _ -> }, onDelete = {})
    }
}
