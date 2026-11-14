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
import uz.otves.engine.Hand
import uz.otves.engine.TextKind
import uz.otves.engine.Turn
import kotlin.math.abs
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
        val title =
            when {
                refining -> stringResource(R.string.sketch_wall_of, entry.step, entry.of)
                entry.size?.kind == TextKind.PIER -> stringResource(R.string.size_pier)
                entry.size?.kind == TextKind.OPENING_WIDTH -> stringResource(R.string.size_opening_width)
                entry.size?.kind == TextKind.CLEAR -> stringResource(R.string.size_clear)
                else -> stringResource(R.string.length_title)
            }
        PanelHeader(title) {
            PanelAction(stringResource(if (refining) R.string.takeoff_close else R.string.length_cancel), actions.onCancelLength)
        }
        TypedLength(entry.digits, standing = entry.was.takeIf { refining }) {
            Text(
                stringResource(R.string.length_was, was),
                style = MaterialTheme.typography.bodyLarge,
                color = OtvesTheme.colors.inkSecondary,
            )
        }
        // A room's size lies between two walls: the builder says which of them gives way.
        entry.size?.takeIf { it.kind == TextKind.CLEAR }?.let { size ->
            val across = abs(size.along.x) > abs(size.along.y)
            InspectorRow(stringResource(R.string.size_moves)) {
                Pill(
                    stringResource(if (across) R.string.size_wall_right else R.string.size_wall_top),
                    !entry.swapped,
                    { actions.onSizeWall(false) },
                )
                Pill(
                    stringResource(if (across) R.string.size_wall_left else R.string.size_wall_bottom),
                    entry.swapped,
                    { actions.onSizeWall(true) },
                )
            }
        }
        NumberKeys(
            actions,
            stringResource(if (refining) R.string.measure_next else R.string.inspector_done),
            actions.onConfirmLength,
            enabled = refining || entry.digits.isNotEmpty(),
        )
    }
}

// A wall across the selected room: how far it stands from one of the room's walls, and from
// which. The engine runs it from wall to wall.
@Composable
internal fun PartitionPad(
    entry: PartitionEntry,
    actions: EditorActions,
) {
    Panel {
        PanelHeader(stringResource(R.string.partition_title)) {
            PanelAction(stringResource(R.string.length_cancel), actions.onCancelPartition)
        }
        TypedLength(entry.digits) {}
        if (entry.refused) {
            Text(
                stringResource(R.string.partition_refused),
                style = MaterialTheme.typography.bodyMedium,
                color = OtvesTheme.colors.warning,
            )
        }
        SegmentedControl(
            options = Hand.entries.map { handName(it) },
            selected = entry.from.ordinal,
            onSelect = { actions.onPartitionWall(Hand.entries[it]) },
            modifier = Modifier.fillMaxWidth().padding(top = 4.dp),
        )
        NumberKeys(actions, stringResource(R.string.partition_build), actions.onConfirmPartition, enabled = entry.digits.isNotEmpty())
    }
}

@Composable
private fun handName(hand: Hand): String =
    stringResource(
        when (hand) {
            Hand.TOP -> R.string.partition_from_top
            Hand.BOTTOM -> R.string.partition_from_bottom
            Hand.LEFT -> R.string.partition_from_left
            Hand.RIGHT -> R.string.partition_from_right
        },
    )

// An opening's own sizes: its width, its height and, for one that is no door, how high over
// the floor it starts. A size nobody types stays as it was.
@Composable
internal fun OpeningPad(
    entry: OpeningEntry,
    actions: EditorActions,
) {
    val colors = OtvesTheme.colors
    Panel {
        PanelHeader(stringResource(R.string.opening_size_title)) {
            PanelAction(stringResource(R.string.length_cancel), actions.onCancelOpening)
        }
        Row(modifier = Modifier.fillMaxWidth().selectableGroup(), horizontalArrangement = Arrangement.spacedBy(8.dp)) {
            listOf(R.string.size_width, R.string.size_height, R.string.size_sill).take(entry.was.size).forEachIndexed { index, label ->
                val active = index == entry.active
                Surface(
                    onClick = { actions.onOpeningField(index) },
                    shape = PanelShape,
                    color = if (active) colors.field else colors.surface,
                    modifier = Modifier.weight(1f).semantics { selected = active },
                ) {
                    Column(modifier = Modifier.padding(horizontal = 12.dp, vertical = 8.dp)) {
                        Text(stringResource(label), style = MaterialTheme.typography.bodyMedium, color = colors.inkSecondary)
                        Text(
                            text = grouped(entry.sizes[index]),
                            style = MaterialTheme.typography.headlineSmall,
                            color = if (entry.digits[index].isEmpty()) colors.inkTertiary else colors.ink,
                        )
                    }
                }
            }
        }
        NumberKeys(actions, stringResource(R.string.inspector_done), actions.onConfirmOpening, enabled = entry.sizes != entry.was)
    }
}

// How thick a layer of a wall type is, typed for insulation and for a gap.
@Composable
internal fun LayerPad(
    entry: LayerEntry,
    actions: EditorActions,
) {
    Panel {
        PanelHeader(stringResource(R.string.layer_thickness_title)) {
            PanelAction(stringResource(R.string.length_cancel), actions.onCancelLayer)
        }
        TypedLength(entry.digits, standing = entry.was) {}
        NumberKeys(actions, stringResource(R.string.inspector_done), actions.onConfirmLayer, enabled = entry.digits.isNotEmpty())
    }
}

// A room by its clear size: its width across the plan and its depth up it, typed one after the
// other, and which way it lies from the corner the builder touched.
@Composable
internal fun RoomPad(
    entry: RoomEntry,
    actions: EditorActions,
) {
    val colors = OtvesTheme.colors
    Panel {
        PanelHeader(stringResource(R.string.room_by_size)) {
            PanelAction(stringResource(R.string.length_cancel), actions.onCancelRoom)
        }
        Row(modifier = Modifier.fillMaxWidth().selectableGroup(), horizontalArrangement = Arrangement.spacedBy(8.dp)) {
            listOf(R.string.size_width, R.string.size_depth).forEachIndexed { index, label ->
                val active = index == entry.active
                Surface(
                    onClick = { actions.onRoomSize(index) },
                    shape = PanelShape,
                    color = if (active) colors.field else colors.surface,
                    modifier = Modifier.weight(1f).semantics { selected = active },
                ) {
                    Column(modifier = Modifier.padding(horizontal = 12.dp, vertical = 8.dp)) {
                        Text(stringResource(label), style = MaterialTheme.typography.bodyMedium, color = colors.inkSecondary)
                        Text(
                            text = entry.digits[index].toLongOrNull()?.let(::grouped) ?: "0",
                            style = MaterialTheme.typography.headlineSmall,
                            color = if (entry.digits[index].isEmpty()) colors.inkTertiary else colors.ink,
                        )
                    }
                }
            }
        }
        if (entry.refused) {
            Text(stringResource(R.string.room_refused), style = MaterialTheme.typography.bodyMedium, color = colors.warning)
        }
        // From the corner: up and to the right, up and to the left, down and to the left, down
        // and to the right.
        val ways = listOf(true to true, false to true, false to false, true to false)
        SegmentedControl(
            options =
                listOf(R.string.room_way_right_up, R.string.room_way_left_up, R.string.room_way_left_down, R.string.room_way_right_down)
                    .map { stringResource(it) },
            selected = ways.indexOf(entry.right to entry.up),
            onSelect = { actions.onRoomWay(ways[it].first, ways[it].second) },
            modifier = Modifier.fillMaxWidth().padding(top = 4.dp),
        )
        val typed = entry.sizes != null
        NumberKeys(
            actions,
            stringResource(if (typed) R.string.partition_build else R.string.measure_next),
            actions.onConfirmRoom,
            enabled = entry.digits[entry.active].isNotEmpty(),
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
