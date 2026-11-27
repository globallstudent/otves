package uz.otves.feature.update

import android.content.ActivityNotFoundException
import android.content.Context
import android.content.Intent
import android.net.Uri
import android.provider.Settings
import androidx.compose.animation.core.animateFloatAsState
import androidx.compose.foundation.background
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxHeight
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.navigationBarsPadding
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.progressSemantics
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.foundation.verticalScroll
import androidx.compose.material3.Button
import androidx.compose.material3.Icon
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Surface
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.graphics.vector.ImageVector
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.res.stringResource
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.tooling.preview.Preview
import androidx.compose.ui.unit.dp
import androidx.lifecycle.Lifecycle
import androidx.lifecycle.compose.LifecycleEventEffect
import androidx.lifecycle.compose.collectAsStateWithLifecycle
import androidx.lifecycle.createSavedStateHandle
import androidx.lifecycle.viewmodel.compose.viewModel
import uz.otves.core.designsystem.OtvesDimens
import uz.otves.core.designsystem.OtvesIcons
import uz.otves.core.designsystem.OtvesTheme
import uz.otves.data.update.Release
import uz.otves.data.update.ReleaseNote
import uz.otves.data.update.Updates
import java.util.Locale

/** The screen that stands in for the app until [release] is installed. */
@Composable
fun UpdateRoute(
    updates: Updates,
    release: Release,
    modifier: Modifier = Modifier,
) {
    val viewModel = viewModel(key = "update-${release.versionCode}") { UpdateViewModel(updates, release, createSavedStateHandle()) }
    val state by viewModel.uiState.collectAsStateWithLifecycle()
    val context = LocalContext.current
    LifecycleEventEffect(Lifecycle.Event.ON_RESUME) { viewModel.resumed() }
    UpdateScreen(
        state = state,
        versionName = release.versionName,
        notes = release.notesIn(stringResource(R.string.update_notes_locale)),
        onStart = viewModel::start,
        onOpenSettings = { openInstallSettings(context) },
        onDismissPermission = viewModel::permissionDismissed,
        modifier = modifier,
    )
}

// Android's switch that lets this app install packages.
private fun openInstallSettings(context: Context) {
    val app = Uri.fromParts("package", context.packageName, null)
    try {
        context.startActivity(Intent(Settings.ACTION_MANAGE_UNKNOWN_APP_SOURCES, app))
    } catch (_: ActivityNotFoundException) {
        // A phone without that screen has the switch in the app's own settings.
        context.startActivity(Intent(Settings.ACTION_APPLICATION_DETAILS_SETTINGS, app))
    }
}

@Composable
internal fun UpdateScreen(
    state: UpdateUiState,
    versionName: String,
    notes: List<ReleaseNote>,
    onStart: () -> Unit,
    onOpenSettings: () -> Unit,
    onDismissPermission: () -> Unit,
    modifier: Modifier = Modifier,
) {
    val colors = OtvesTheme.colors
    Box(modifier = modifier.fillMaxSize().background(colors.ink)) {
        UpdateHero(
            progress = state.progress,
            versionName = versionName,
            modifier = Modifier.fillMaxWidth().fillMaxHeight(HERO_SHARE),
        )
        Surface(
            color = colors.surface,
            shape = RoundedCornerShape(topStart = SheetCorner, topEnd = SheetCorner),
            modifier = Modifier.align(Alignment.BottomCenter).fillMaxWidth().fillMaxHeight(SHEET_SHARE),
        ) {
            Column(modifier = Modifier.navigationBarsPadding().padding(start = 20.dp, end = 20.dp, top = 28.dp, bottom = 20.dp)) {
                Column(modifier = Modifier.weight(1f).verticalScroll(rememberScrollState())) {
                    Text(
                        text = stringResource(R.string.update_title, versionName),
                        style = MaterialTheme.typography.headlineSmall,
                        fontWeight = FontWeight.SemiBold,
                    )
                    Spacer(Modifier.height(8.dp))
                    Text(
                        text = stringResource(R.string.update_body),
                        style = MaterialTheme.typography.bodyLarge,
                        color = colors.inkSecondary,
                    )
                    if (notes.isNotEmpty()) {
                        Spacer(Modifier.height(24.dp))
                        Text(
                            text = stringResource(R.string.update_whats_new),
                            style = MaterialTheme.typography.titleSmall,
                            color = colors.inkSecondary,
                        )
                        Spacer(Modifier.height(14.dp))
                        Column(verticalArrangement = Arrangement.spacedBy(16.dp)) {
                            notes.forEach { note -> NoteRow(note) }
                        }
                    }
                    Spacer(Modifier.height(16.dp))
                }
                Status(state)
                Spacer(Modifier.height(12.dp))
                Action(state, onStart)
            }
        }
    }
    if (state.askPermission) {
        InstallPermissionSheet(onOpenSettings = onOpenSettings, onDismiss = onDismissPermission)
    }
}

@Composable
private fun NoteRow(note: ReleaseNote) {
    Row(horizontalArrangement = Arrangement.spacedBy(14.dp)) {
        Icon(
            imageVector = noteIcon(note.icon),
            contentDescription = null,
            tint = OtvesTheme.colors.ink,
            modifier = Modifier.padding(top = 1.dp).size(24.dp),
        )
        Column {
            Text(note.title, style = MaterialTheme.typography.titleMedium, fontWeight = FontWeight.SemiBold)
            if (note.text.isNotEmpty()) {
                Text(note.text, style = MaterialTheme.typography.bodyMedium, color = OtvesTheme.colors.inkSecondary)
            }
        }
    }
}

// The pictures a release note may name in content/releases; a later name falls back to a plain mark.
private fun noteIcon(name: String): ImageVector =
    when (name) {
        "sheet" -> OtvesIcons.Sheet
        "dimension" -> OtvesIcons.Ruler
        "opening" -> OtvesIcons.Window
        "door" -> OtvesIcons.Door
        "wall" -> OtvesIcons.Wall
        "sketch" -> OtvesIcons.Pencil
        "estimate" -> OtvesIcons.List
        else -> OtvesIcons.Dot
    }

// What happens to the user's work and the file, or why the last try stopped.
@Composable
private fun Status(state: UpdateUiState) {
    val size = megabytes(state.size)
    val received = megabytes(state.received)
    val failure = state.failure
    Column(verticalArrangement = Arrangement.spacedBy(2.dp)) {
        if (failure != null) {
            Text(
                text = stringResource(failureTitle(failure)),
                style = MaterialTheme.typography.bodyLarge,
                fontWeight = FontWeight.SemiBold,
                color = OtvesTheme.colors.warning,
            )
        }
        val note =
            when {
                failure == UpdateFailure.Network -> {
                    stringResource(R.string.update_failed_network_note, received, size)
                }

                // The installer copies the file, so the phone has to hold it twice.
                failure == UpdateFailure.Storage -> {
                    stringResource(R.string.update_failed_storage_note, megabytes(state.size * 2))
                }

                failure == UpdateFailure.Corrupt -> {
                    stringResource(R.string.update_failed_corrupt_note)
                }

                failure == UpdateFailure.Install -> {
                    stringResource(R.string.update_failed_install_note)
                }

                state.phase == UpdatePhase.Downloading -> {
                    stringResource(R.string.update_downloaded, received, size)
                }

                state.phase == UpdatePhase.Installing -> {
                    stringResource(R.string.update_installing_note)
                }

                else -> {
                    stringResource(R.string.update_keeps, size)
                }
            }
        Text(note, style = MaterialTheme.typography.bodySmall, color = OtvesTheme.colors.inkSecondary)
    }
}

// In the language of the strings, not of the phone, so a Russian line gets a Russian number.
@Composable
private fun megabytes(bytes: Long): String {
    val locale = Locale.forLanguageTag(stringResource(R.string.update_notes_locale))
    return stringResource(R.string.update_megabytes, String.format(locale, "%.1f", bytes / BYTES_IN_MEGABYTE))
}

private fun failureTitle(failure: UpdateFailure): Int =
    when (failure) {
        UpdateFailure.Network -> R.string.update_failed_network
        UpdateFailure.Storage -> R.string.update_failed_storage
        UpdateFailure.Corrupt -> R.string.update_failed_corrupt
        UpdateFailure.Install -> R.string.update_failed_install
    }

// The one command of the screen; while the update runs it is the progress bar.
@Composable
private fun Action(
    state: UpdateUiState,
    onStart: () -> Unit,
) {
    val colors = OtvesTheme.colors
    val shape = RoundedCornerShape(14.dp)
    val bar = Modifier.fillMaxWidth().height(OtvesDimens.primaryTarget)
    when (state.phase) {
        UpdatePhase.Offered -> {
            Button(onClick = onStart, shape = shape, modifier = bar) {
                Text(
                    text = stringResource(if (state.failure == null) R.string.update_start else R.string.update_retry),
                    style = MaterialTheme.typography.titleMedium,
                )
            }
        }

        UpdatePhase.Downloading -> {
            val filled by animateFloatAsState(state.progress, label = "filled")
            Box(
                modifier = bar.clip(shape).background(colors.ink).progressSemantics(state.progress),
                contentAlignment = Alignment.Center,
            ) {
                Box(
                    Modifier
                        .align(Alignment.CenterStart)
                        .fillMaxHeight()
                        .fillMaxWidth(filled)
                        .background(colors.inkSecondary),
                )
                Text(
                    text = stringResource(R.string.update_downloading, (state.progress * PERCENT).toInt()),
                    style = MaterialTheme.typography.titleMedium,
                    color = colors.surface,
                )
            }
        }

        UpdatePhase.Installing -> {
            Box(
                modifier = bar.clip(shape).background(colors.inkSecondary).progressSemantics(),
                contentAlignment = Alignment.Center,
            ) {
                Text(
                    text = stringResource(R.string.update_installing),
                    style = MaterialTheme.typography.titleMedium,
                    color = colors.surface,
                )
            }
        }
    }
}

private const val HERO_SHARE = 0.44f
private const val SHEET_SHARE = 0.6f
private const val PERCENT = 100
private const val BYTES_IN_MEGABYTE = 1_000_000.0
private val SheetCorner = 24.dp

private val PreviewNotes =
    listOf(
        ReleaseNote("sheet", "Листы в стандартном масштабе", "План сразу ложится на лист"),
        ReleaseNote("opening", "Проёмы точно по замерам", "Ширина, высота и низ вводятся числом"),
        ReleaseNote("dimension", "Размеры читаются чище", "Длина стены не наезжает на размеры"),
    )

@Composable
private fun PreviewScreen(state: UpdateUiState) {
    OtvesTheme {
        UpdateScreen(
            state = state,
            versionName = "0.1.1",
            notes = PreviewNotes,
            onStart = {},
            onOpenSettings = {},
            onDismissPermission = {},
        )
    }
}

private const val PREVIEW_SIZE = 9_634_512L

@Preview(widthDp = 390, heightDp = 844)
@Composable
private fun OfferedPreview() = PreviewScreen(UpdateUiState(size = PREVIEW_SIZE))

@Preview(widthDp = 390, heightDp = 844)
@Composable
private fun DownloadingPreview() = PreviewScreen(UpdateUiState(size = PREVIEW_SIZE, phase = UpdatePhase.Downloading, received = 4_335_530))

@Preview(widthDp = 390, heightDp = 844)
@Composable
private fun InstallingPreview() = PreviewScreen(UpdateUiState(size = PREVIEW_SIZE, phase = UpdatePhase.Installing, received = PREVIEW_SIZE))

@Preview(widthDp = 390, heightDp = 844)
@Composable
private fun NoInternetPreview() = PreviewScreen(UpdateUiState(size = PREVIEW_SIZE, received = 4_335_530, failure = UpdateFailure.Network))

@Preview(widthDp = 390, heightDp = 844)
@Composable
private fun PermissionPreview() = PreviewScreen(UpdateUiState(size = PREVIEW_SIZE, askPermission = true))
