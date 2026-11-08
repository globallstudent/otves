package uz.otves.feature.projects

import androidx.compose.foundation.Image
import androidx.compose.foundation.combinedClickable
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.heightIn
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.safeDrawingPadding
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.items
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.AlertDialog
import androidx.compose.material3.Button
import androidx.compose.material3.DropdownMenu
import androidx.compose.material3.DropdownMenuItem
import androidx.compose.material3.HorizontalDivider
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.OutlinedTextField
import androidx.compose.material3.Surface
import androidx.compose.material3.Text
import androidx.compose.material3.TextButton
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.saveable.rememberSaveable
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.res.painterResource
import androidx.compose.ui.res.stringResource
import androidx.compose.ui.text.style.TextAlign
import androidx.compose.ui.text.style.TextOverflow
import androidx.compose.ui.tooling.preview.Preview
import androidx.compose.ui.unit.dp
import androidx.lifecycle.compose.collectAsStateWithLifecycle
import androidx.lifecycle.viewmodel.compose.viewModel
import uz.otves.core.designsystem.OtvesDimens
import uz.otves.core.designsystem.OtvesTheme
import uz.otves.data.project.JobSummary
import uz.otves.data.project.Jobs
import java.time.Instant
import java.time.ZoneId
import java.time.format.DateTimeFormatter
import java.time.format.FormatStyle
import java.util.Locale
import kotlin.uuid.Uuid
import uz.otves.core.designsystem.R as DesignR

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
    var renaming by rememberSaveable { mutableStateOf<String?>(null) }
    var deleting by rememberSaveable { mutableStateOf<String?>(null) }
    Surface(modifier = modifier.fillMaxSize(), color = OtvesTheme.colors.canvas) {
        Column(modifier = Modifier.safeDrawingPadding()) {
            Text(
                text = stringResource(R.string.projects_title),
                style = MaterialTheme.typography.headlineSmall,
                modifier = Modifier.padding(horizontal = OtvesDimens.gutter, vertical = 12.dp),
            )
            Box(modifier = Modifier.weight(1f).fillMaxWidth()) {
                when {
                    state.loading -> {
                        // Nothing until the list is read.
                    }

                    state.jobs.isEmpty() -> {
                        EmptyProjects(modifier = Modifier.align(Alignment.Center))
                    }

                    else -> {
                        JobList(
                            jobs = state.jobs,
                            onOpen = onOpen,
                            onRename = { renaming = it.toString() },
                            onDelete = { deleting = it.toString() },
                        )
                    }
                }
            }
            Button(
                onClick = { naming = true },
                enabled = !state.loading,
                shape = RoundedCornerShape(12.dp),
                modifier =
                    Modifier
                        .fillMaxWidth()
                        .padding(OtvesDimens.gutter)
                        .height(OtvesDimens.primaryTarget),
            ) {
                Text(stringResource(R.string.projects_new), style = MaterialTheme.typography.titleMedium)
            }
        }
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

@Composable
private fun EmptyProjects(modifier: Modifier = Modifier) {
    Column(
        modifier = modifier.padding(OtvesDimens.gutter),
        horizontalAlignment = Alignment.CenterHorizontally,
        verticalArrangement = Arrangement.Center,
    ) {
        Image(painter = painterResource(DesignR.drawable.ic_otves_mark), contentDescription = null)
        Spacer(Modifier.height(24.dp))
        Text(
            text = stringResource(R.string.projects_empty_title),
            style = MaterialTheme.typography.headlineSmall,
            textAlign = TextAlign.Center,
        )
        Spacer(Modifier.height(8.dp))
        Text(
            text = stringResource(R.string.projects_empty_body),
            style = MaterialTheme.typography.bodyLarge,
            color = OtvesTheme.colors.inkSecondary,
            textAlign = TextAlign.Center,
        )
    }
}

// A tap opens a job; a long press offers to rename or delete it.
@Composable
private fun JobList(
    jobs: List<JobSummary>,
    onOpen: (Uuid) -> Unit,
    onRename: (Uuid) -> Unit,
    onDelete: (Uuid) -> Unit,
) {
    // The language of the strings, not of the phone: Russian text gets a Russian date.
    val locale = Locale.forLanguageTag(stringResource(R.string.projects_date_locale))
    val dates = DateTimeFormatter.ofLocalizedDate(FormatStyle.MEDIUM).withLocale(locale)
    var menuFor by remember { mutableStateOf<Uuid?>(null) }
    LazyColumn {
        items(jobs, key = { it.id.toString() }) { job ->
            Box {
                JobRow(job, dates, onOpen = { onOpen(job.id) }, onMenu = { menuFor = job.id })
                DropdownMenu(expanded = menuFor == job.id, onDismissRequest = { menuFor = null }) {
                    DropdownMenuItem(
                        text = { Text(stringResource(R.string.projects_rename)) },
                        onClick = {
                            menuFor = null
                            onRename(job.id)
                        },
                    )
                    DropdownMenuItem(
                        text = { Text(stringResource(R.string.projects_delete)) },
                        onClick = {
                            menuFor = null
                            onDelete(job.id)
                        },
                    )
                }
            }
            HorizontalDivider(color = OtvesTheme.colors.divider, modifier = Modifier.padding(horizontal = OtvesDimens.gutter))
        }
    }
}

@Composable
private fun JobRow(
    job: JobSummary,
    dates: DateTimeFormatter,
    onOpen: () -> Unit,
    onMenu: () -> Unit,
) {
    Column(
        modifier =
            Modifier
                .fillMaxWidth()
                .heightIn(min = RowHeight)
                .combinedClickable(onClick = onOpen, onLongClick = onMenu)
                .padding(horizontal = OtvesDimens.gutter, vertical = 12.dp),
        verticalArrangement = Arrangement.Center,
    ) {
        Text(job.name, style = MaterialTheme.typography.titleMedium, maxLines = 1, overflow = TextOverflow.Ellipsis)
        val changed = Instant.ofEpochMilli(job.updatedAt).atZone(ZoneId.systemDefault()).format(dates)
        Text(
            text = stringResource(R.string.projects_changed, changed),
            style = MaterialTheme.typography.bodyMedium,
            color = OtvesTheme.colors.inkSecondary,
        )
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

private val RowHeight = 64.dp

@Preview(widthDp = 400, heightDp = 760)
@Composable
private fun ProjectsScreenPreview() {
    OtvesTheme {
        ProjectsScreen(
            state =
                ProjectsUiState(
                    loading = false,
                    jobs =
                        listOf(
                            JobSummary(Uuid.parse("019a5f00-0000-7000-8000-000000000001"), "Дом на Чиланзаре", 1_762_600_000_000),
                            JobSummary(Uuid.parse("019a5f00-0000-7000-8000-000000000002"), "Баня", 1_762_400_000_000),
                        ),
                ),
            onOpen = {},
            onCreate = {},
            onRename = { _, _ -> },
            onDelete = {},
        )
    }
}

@Preview(widthDp = 400, heightDp = 760)
@Composable
private fun EmptyProjectsPreview() {
    OtvesTheme {
        ProjectsScreen(state = ProjectsUiState(loading = false), onOpen = {}, onCreate = {}, onRename = { _, _ -> }, onDelete = {})
    }
}
