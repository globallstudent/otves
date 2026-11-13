package uz.otves.feature.editor

import androidx.compose.foundation.horizontalScroll
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.rememberScrollState
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.res.stringResource
import androidx.compose.ui.semantics.selected
import androidx.compose.ui.unit.dp
import uz.otves.core.designsystem.OtvesIcons
import uz.otves.core.designsystem.OtvesTheme
import uz.otves.core.designsystem.SegmentedControl
import uz.otves.engine.OpeningKind

// What the select tool picked, with its settings.
@Composable
internal fun Inspector(
    selected: Selected,
    state: EditorUiState,
    actions: EditorActions,
) {
    val issues = state.issues
    Panel {
        val title =
            when (selected) {
                is Selected.WallPart -> stringResource(R.string.tool_wall)
                is Selected.OpeningPart -> kindName(selected.opening.kind)
            }
        PanelHeader(title) {
            PanelAction(stringResource(R.string.inspector_delete), actions.onDelete)
            PanelAction(stringResource(R.string.inspector_done), actions.onDeselect)
        }
        when (selected) {
            is Selected.WallPart -> {
                val unit = selected.wall.unit
                InspectorRow(stringResource(R.string.inspector_material)) {
                    Pill(
                        label = if (unit == null) stringResource(R.string.material_choose) else unitTitle(unit, state.catalog.units),
                        selected = false,
                        onClick = { actions.onMaterials(true) },
                        trailing = OtvesIcons.ChevronDown,
                    )
                }
                state.dimensions.find { it.id == selected.id }?.let { dimension ->
                    InspectorRow(stringResource(R.string.length_title)) {
                        Pill(stringResource(R.string.length_value, grouped(dimension.length)), false, { actions.onEditLength(selected.id) })
                    }
                }
                InspectorRow(stringResource(R.string.inspector_thickness)) {
                    state.thicknessesOf(unit).forEach { thickness ->
                        Pill(grouped(thickness), thickness == selected.wall.thickness, { actions.onWallThickness(thickness) })
                    }
                }
            }

            is Selected.OpeningPart -> {
                val opening = selected.opening
                issues
                    .filter { it.element == selected.id }
                    .map { it.kind }
                    .distinct()
                    .forEach { kind ->
                        Text(issueText(kind), style = MaterialTheme.typography.bodyMedium, color = OtvesTheme.colors.warning)
                    }
                SegmentedControl(
                    options = OpeningKind.entries.map { kindName(it) },
                    selected = opening.kind.ordinal,
                    onSelect = { actions.onOpeningKind(OpeningKind.entries[it]) },
                    modifier = Modifier.fillMaxWidth(),
                )
                InspectorRow(stringResource(R.string.inspector_size)) {
                    val current = OpeningSize(opening.width, opening.height)
                    sizesFor(opening.kind).forEach { size ->
                        Pill(sizeName(size), size == current, { actions.onOpeningSize(size) })
                    }
                }
            }
        }
        Spacer(Modifier.height(8.dp))
    }
}

@Composable
internal fun InspectorRow(
    label: String,
    content: @Composable () -> Unit,
) {
    Row(modifier = Modifier.fillMaxWidth().padding(top = 8.dp), verticalAlignment = Alignment.CenterVertically) {
        Text(
            label,
            style = MaterialTheme.typography.bodyLarge,
            color = OtvesTheme.colors.inkSecondary,
            modifier = Modifier.width(InspectorLabelWidth),
        )
        Row(
            modifier = Modifier.horizontalScroll(rememberScrollState()).padding(vertical = 4.dp),
            horizontalArrangement = Arrangement.spacedBy(8.dp),
        ) {
            content()
        }
    }
}

private val InspectorLabelWidth = 124.dp
