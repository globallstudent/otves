package uz.otves.feature.editor

import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.selection.selectableGroup
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Surface
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.res.stringResource
import androidx.compose.ui.semantics.selected
import androidx.compose.ui.semantics.semantics
import androidx.compose.ui.tooling.preview.Preview
import androidx.compose.ui.unit.dp
import uz.otves.core.designsystem.OtvesTheme
import uz.otves.core.designsystem.SegmentedControl
import uz.otves.core.model.ElementId
import uz.otves.core.model.Mm
import uz.otves.core.model.mm
import uz.otves.engine.Turn
import kotlin.uuid.Uuid

// A wall's exact length in millimetres, typed on large keys as it is read off the tape.
@Composable
internal fun LengthPad(
    entry: LengthEntry,
    actions: EditorActions,
) {
    // In a pass over sketched walls the length the sketch gave stands until another is typed.
    val refining = entry.refining
    val was = if (refining) "${stringResource(R.string.length_about_mark)} ${grouped(entry.was)}" else grouped(entry.was)
    Panel {
        PanelHeader(
            if (refining) stringResource(R.string.sketch_wall_of, entry.step, entry.of) else stringResource(R.string.length_title),
        ) {
            PanelAction(stringResource(if (refining) R.string.takeoff_close else R.string.length_cancel), actions.onCancelLength)
        }
        TypedLength(entry.digits, standing = entry.was.takeIf { refining }) {
            Text(
                stringResource(R.string.length_was, was),
                style = MaterialTheme.typography.bodyLarge,
                color = OtvesTheme.colors.inkSecondary,
            )
        }
        NumberKeys(
            actions,
            stringResource(if (refining) R.string.measure_next else R.string.inspector_done),
            actions.onConfirmLength,
            enabled = refining || entry.digits.isNotEmpty(),
        )
    }
}

// Walls entered one after another from readings: the length, then which way it goes from the wall
// before. The plan shows the wall before it is built.
@Composable
internal fun MeasurePad(
    measure: Measure,
    state: EditorUiState,
    actions: EditorActions,
) {
    Panel {
        PanelHeader(stringResource(R.string.measure_title)) {
            PanelAction(stringResource(R.string.editor_undo), actions.onUndo, enabled = state.canUndo)
            PanelAction(stringResource(R.string.takeoff_close), actions.onStopMeasure)
        }
        TypedLength(measure.digits) {
            if (state.canCloseRun) Pill(stringResource(R.string.measure_close), false, actions.onCloseMeasured)
        }
        SegmentedControl(
            options = Turn.entries.map { turnName(it) },
            selected = measure.turn.ordinal,
            onSelect = { actions.onTurn(Turn.entries[it]) },
            modifier = Modifier.fillMaxWidth().padding(top = 4.dp),
        )
        NumberKeys(actions, stringResource(R.string.measure_next), actions.onNextMeasured, enabled = state.draft?.to != null)
    }
}

// The length typed so far; before a digit, the length that stands if none is typed, or zero.
@Composable
private fun TypedLength(
    digits: String,
    standing: Mm? = null,
    trailing: @Composable () -> Unit,
) {
    val colors = OtvesTheme.colors
    Row(modifier = Modifier.fillMaxWidth().padding(vertical = 4.dp), verticalAlignment = Alignment.CenterVertically) {
        Text(
            text = stringResource(R.string.length_value, grouped(digits.toLongOrNull() ?: standing?.value ?: 0)),
            style = MaterialTheme.typography.displaySmall,
            color = if (digits.isEmpty()) colors.inkTertiary else colors.ink,
            modifier = Modifier.weight(1f),
        )
        trailing()
    }
}

// A brick or block the catalog does not list: its three sizes as laid, typed one after another.
@Composable
internal fun SizePad(
    entry: SizeEntry,
    actions: EditorActions,
) {
    val colors = OtvesTheme.colors
    val labels =
        if (entry.footing) {
            listOf(R.string.size_width, R.string.size_depth)
        } else {
            listOf(R.string.size_length, R.string.size_width, R.string.size_height)
        }
    Panel {
        PanelHeader(stringResource(if (entry.footing) R.string.level_footing else R.string.material_own)) {
            PanelAction(stringResource(R.string.length_cancel), actions.onCancelSize)
        }
        Row(modifier = Modifier.fillMaxWidth().selectableGroup(), horizontalArrangement = Arrangement.spacedBy(8.dp)) {
            labels.forEachIndexed { index, label ->
                val active = index == entry.active
                Surface(
                    onClick = { actions.onSizeField(index) },
                    shape = PanelShape,
                    // No boxes: the size being typed sits on the field colour, the others on the panel.
                    color = if (active) colors.field else colors.surface,
                    modifier = Modifier.weight(1f).semantics { selected = active },
                ) {
                    Column(modifier = Modifier.padding(horizontal = 12.dp, vertical = 8.dp)) {
                        Text(stringResource(label), style = MaterialTheme.typography.bodyMedium, color = colors.inkSecondary)
                        Text(
                            text = entry.digits[index].ifEmpty { "0" },
                            style = MaterialTheme.typography.headlineSmall,
                            color = if (entry.digits[index].isEmpty()) colors.inkTertiary else colors.ink,
                        )
                    }
                }
            }
        }
        NumberKeys(
            actions,
            stringResource(R.string.inspector_done),
            actions.onConfirmSize,
            enabled = entry.size != null || entry.section != null,
        )
    }
}

// What the builder pays for one unit of a material, in whole units of the currency. It goes to
// his price list, so the next job already knows it; an empty price forgets it.
@Composable
internal fun PricePad(
    entry: PriceEntry,
    state: EditorUiState,
    actions: EditorActions,
) {
    val colors = OtvesTheme.colors
    Panel {
        PanelHeader(itemName(entry.line.item, state.catalog.units)) {
            PanelAction(stringResource(R.string.length_cancel), actions.onCancelPrice)
        }
        Row(modifier = Modifier.fillMaxWidth().padding(vertical = 4.dp), verticalAlignment = Alignment.Bottom) {
            Text(
                text = stringResource(R.string.money_uzs, entry.digits.toLongOrNull()?.let(::grouped) ?: "0"),
                style = MaterialTheme.typography.displaySmall,
                color = if (entry.digits.isEmpty()) colors.inkTertiary else colors.ink,
                modifier = Modifier.weight(1f),
            )
            Text(
                stringResource(R.string.price_for_one, unitName(entry.line.unit)),
                style = MaterialTheme.typography.bodyLarge,
                color = colors.inkSecondary,
            )
        }
        NumberKeys(actions, stringResource(R.string.inspector_done), actions.onConfirmPrice, enabled = true)
    }
}

@Preview(widthDp = 400, heightDp = 760)
@Composable
private fun LengthPadPreview() {
    val wall = ElementId(Uuid.parse("019a5f00-0000-7000-8000-000000000002"))
    OtvesTheme {
        EditorScreen(
            state = EditorUiState(ready = true, entry = LengthEntry(wall, was = 6_437.mm, digits = "6400")),
            actions = EditorActions(),
        )
    }
}
