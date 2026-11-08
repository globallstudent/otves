package uz.otves.feature.projects

import androidx.compose.foundation.Image
import androidx.compose.foundation.clickable
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
    ProjectsScreen(state = state, onOpen = onOpen, onCreate = viewModel::create, modifier = modifier)
}

@Composable
internal fun ProjectsScreen(
    state: ProjectsUiState,
    onOpen: (Uuid) -> Unit,
    onCreate: (String) -> Unit,
    modifier: Modifier = Modifier,
) {
    var naming by rememberSaveable { mutableStateOf(false) }
    Surface(modifier = modifier.fillMaxSize(), color = OtvesTheme.colors.canvas) {
        Column(modifier = Modifier.safeDrawingPadding()) {
            Text(
                text = stringResource(R.string.projects_title),
                style = MaterialTheme.typography.headlineSmall,
                modifier = Modifier.padding(horizontal = OtvesDimens.gutter, vertical = 12.dp),
            )
            Box(modifier = Modifier.weight(1f).fillMaxWidth()) {
                when {
                    state.loading -> Unit
                    state.jobs.isEmpty() -> EmptyProjects(modifier = Modifier.align(Alignment.Center))
                    else -> JobList(jobs = state.jobs, onOpen = onOpen)
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
            suggested = stringResource(R.string.projects_default_name, state.jobs.size + 1),
            onCreate = { name ->
                naming = false
                onCreate(name)
            },
            onDismiss = { naming = false },
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

@Composable
private fun JobList(
    jobs: List<JobSummary>,
    onOpen: (Uuid) -> Unit,
) {
    // The language of the strings, not of the phone: Russian text gets a Russian date.
    val locale = Locale.forLanguageTag(stringResource(R.string.projects_date_locale))
    val dates = DateTimeFormatter.ofLocalizedDate(FormatStyle.MEDIUM).withLocale(locale)
    LazyColumn {
        items(jobs, key = { it.id.toString() }) { job ->
            Column(
                modifier =
                    Modifier
                        .fillMaxWidth()
                        .heightIn(min = RowHeight)
                        .clickable { onOpen(job.id) }
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
            HorizontalDivider(color = OtvesTheme.colors.divider, modifier = Modifier.padding(horizontal = OtvesDimens.gutter))
        }
    }
}

@Composable
private fun NameDialog(
    suggested: String,
    onCreate: (String) -> Unit,
    onDismiss: () -> Unit,
) {
    var name by rememberSaveable { mutableStateOf(suggested) }
    AlertDialog(
        onDismissRequest = onDismiss,
        title = { Text(stringResource(R.string.projects_new)) },
        text = {
            OutlinedTextField(
                value = name,
                onValueChange = { name = it },
                label = { Text(stringResource(R.string.projects_name)) },
                singleLine = true,
            )
        },
        confirmButton = {
            TextButton(onClick = { onCreate(name) }, enabled = name.isNotBlank()) {
                Text(stringResource(R.string.projects_create))
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
        )
    }
}

@Preview(widthDp = 400, heightDp = 760)
@Composable
private fun EmptyProjectsPreview() {
    OtvesTheme {
        ProjectsScreen(state = ProjectsUiState(loading = false), onOpen = {}, onCreate = {})
    }
}
