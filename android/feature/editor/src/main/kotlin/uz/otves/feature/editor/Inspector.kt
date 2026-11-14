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
import androidx.compose.ui.platform.LocalResources
import androidx.compose.ui.res.stringResource
import androidx.compose.ui.semantics.selected
import androidx.compose.ui.unit.dp
import uz.otves.core.designsystem.OtvesIcons
import uz.otves.core.designsystem.OtvesTheme
import uz.otves.core.designsystem.SegmentedControl
import uz.otves.engine.LayerSpec
import uz.otves.engine.LocationLine
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
                is Selected.WallPart -> {
                    stringResource(R.string.tool_wall)
                }

                is Selected.OpeningPart -> {
                    kindName(selected.opening.kind)
                }

                is Selected.SpacePart -> {
                    if (selected.space.room ==
                        null
                    ) {
                        stringResource(R.string.room_title)
                    } else {
                        roomKindName(selected.space.kind)
                    }
                }
            }
        PanelHeader(title) {
            when (selected) {
                is Selected.WallPart, is Selected.OpeningPart -> {
                    PanelAction(stringResource(R.string.inspector_delete), actions.onDelete)
                }

                // The walls make the space; only its name can be taken away.
                is Selected.SpacePart -> {
                    if (selected.space.room !=
                        null
                    ) {
                        PanelAction(stringResource(R.string.room_remove_name), actions.onDelete)
                    }
                }
            }
            PanelAction(stringResource(R.string.inspector_done), actions.onDeselect)
        }
        when (selected) {
            is Selected.WallPart -> {
                WallRows(selected, state, actions)
            }

            is Selected.SpacePart -> {
                SpaceRows(selected, state, actions)
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
                // The size the opening has, which opens the keys for one of the builder's own,
                // then the usual sizes of its kind.
                InspectorRow(stringResource(R.string.inspector_size)) {
                    val current = OpeningSize(opening.width, opening.height)
                    Pill(sizeName(current), true, actions.onEditOpening, trailing = OtvesIcons.ChevronDown)
                    sizesFor(opening.kind).filter { it != current }.forEach { size ->
                        Pill(sizeName(size), false, { actions.onOpeningSize(size) })
                    }
                }
                OpeningRows(selected, state, actions)
            }
        }
        Spacer(Modifier.height(8.dp))
    }
}

// How a door hangs, where the opening stands on its side of the room, and the mark it shares
// with the openings of its kind and size.
@Composable
private fun OpeningRows(
    selected: Selected.OpeningPart,
    state: EditorUiState,
    actions: EditorActions,
) {
    if (selected.opening.kind == OpeningKind.DOOR) {
        InspectorRow(stringResource(R.string.door_hinge)) {
            Pill(stringResource(R.string.door_hinge_flip), false, actions.onFlipHinge)
        }
        InspectorRow(stringResource(R.string.door_swing)) {
            Pill(stringResource(R.string.door_swing_flip), false, actions.onFlipSwing)
        }
    }
    InspectorRow(stringResource(R.string.opening_place)) {
        Pill(stringResource(R.string.opening_center), false, actions.onCenterOpening)
    }
    // The engine numbers the marks; the one of this opening came with the drawing.
    val mark = state.drawing.texts.find { it.isMark && it.element == selected.id } ?: return
    InspectorRow(stringResource(R.string.opening_mark)) {
        Text(
            stringResource(R.string.opening_mark_count, markText(LocalResources.current, mark.kind, mark.value), selected.alike),
            style = MaterialTheme.typography.titleMedium,
            modifier = Modifier.padding(vertical = 12.dp),
        )
    }
}

// A space is its name, which the builder gives, and its floor, which the walls give.
@Composable
private fun SpaceRows(
    selected: Selected.SpacePart,
    state: EditorUiState,
    actions: EditorActions,
) {
    val space = selected.space
    state.issues
        .filter { it.element != null && it.element == space.room }
        .map { it.kind }
        .distinct()
        .forEach { kind ->
            Text(issueText(kind), style = MaterialTheme.typography.bodyMedium, color = OtvesTheme.colors.warning)
        }
    InspectorRow(stringResource(R.string.room_name)) {
        Pill(
            label = if (space.room == null) stringResource(R.string.room_name_choose) else roomKindName(space.kind),
            selected = false,
            onClick = { actions.onRoomKinds(true) },
            trailing = OtvesIcons.ChevronDown,
        )
    }
    InspectorRow(stringResource(R.string.room_area)) {
        Text(areaText(space.area), style = MaterialTheme.typography.titleMedium, modifier = Modifier.padding(vertical = 12.dp))
    }
    InspectorRow(stringResource(R.string.room_perimeter)) {
        Text(metresText(space.perimeter), style = MaterialTheme.typography.titleMedium, modifier = Modifier.padding(vertical = 12.dp))
    }
    InspectorRow(stringResource(R.string.role_partition)) {
        Pill(stringResource(R.string.partition_place), selected = false, onClick = actions.onPartition)
    }
}

// A wall is what its type says, or with no type what it is laid from and how thick; then how
// long it is and which of its lines its points lie on.
@Composable
private fun WallRows(
    selected: Selected.WallPart,
    state: EditorUiState,
    actions: EditorActions,
) {
    val wall = selected.wall
    val kind = state.kinds.find { it.id == wall.type }
    InspectorRow(stringResource(R.string.wall_kind_title)) {
        Pill(
            label = if (kind == null) stringResource(R.string.wall_kind_none) else wallKindName(kind.role, kind.thickness),
            selected = false,
            onClick = { actions.onKinds(true) },
            trailing = OtvesIcons.ChevronDown,
        )
    }
    if (kind != null) {
        Text(
            layersText(kind.layers.map { (_, layer) -> LayerSpec(layer.function, layer.thickness, layer.unit) }, state.catalog.units),
            style = MaterialTheme.typography.bodyMedium,
            color = OtvesTheme.colors.inkSecondary,
        )
    } else {
        InspectorRow(stringResource(R.string.inspector_material)) {
            Pill(
                label = if (wall.unit == null) stringResource(R.string.material_choose) else unitTitle(wall.unit!!, state.catalog.units),
                selected = false,
                onClick = { actions.onMaterials(true) },
                trailing = OtvesIcons.ChevronDown,
            )
        }
    }
    state.dimensions.find { it.id == selected.id }?.let { dimension ->
        InspectorRow(stringResource(R.string.length_title)) {
            Pill(stringResource(R.string.length_value, grouped(dimension.length)), false, { actions.onEditLength(selected.id) })
        }
    }
    if (kind == null) {
        InspectorRow(stringResource(R.string.inspector_thickness)) {
            state.thicknessesOf(wall.unit).forEach { thickness ->
                Pill(grouped(thickness), thickness == wall.thickness, { actions.onWallThickness(thickness) })
            }
        }
    }
    // The outer face is the wall's right one, looking from its start, or its left one when the
    // wall is turned round.
    val outer = if (wall.mirrored) LocationLine.LEFT_FACE else LocationLine.RIGHT_FACE
    val inner = if (wall.mirrored) LocationLine.RIGHT_FACE else LocationLine.LEFT_FACE
    InspectorRow(stringResource(R.string.inspector_location)) {
        Pill(stringResource(R.string.location_inside), wall.locationLine == inner, { actions.onLocationLine(inner) })
        Pill(
            stringResource(R.string.location_axis),
            wall.locationLine == LocationLine.CENTER,
            { actions.onLocationLine(LocationLine.CENTER) },
        )
        Pill(stringResource(R.string.location_outside), wall.locationLine == outer, { actions.onLocationLine(outer) })
    }
    if (kind != null && kind.layers.size > 1) {
        InspectorRow(stringResource(R.string.inspector_layers)) {
            Pill(stringResource(R.string.inspector_flip), false, actions.onFlipWall)
        }
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
