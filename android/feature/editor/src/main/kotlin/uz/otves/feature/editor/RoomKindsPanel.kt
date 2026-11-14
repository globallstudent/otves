package uz.otves.feature.editor

import androidx.compose.foundation.background
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.heightIn
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.selection.selectable
import androidx.compose.foundation.selection.selectableGroup
import androidx.compose.foundation.shape.CircleShape
import androidx.compose.foundation.verticalScroll
import androidx.compose.material3.HorizontalDivider
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.res.stringResource
import androidx.compose.ui.semantics.Role
import androidx.compose.ui.tooling.preview.Preview
import androidx.compose.ui.unit.dp
import uz.otves.core.designsystem.OtvesDimens
import uz.otves.core.designsystem.OtvesTheme
import uz.otves.core.model.mm
import uz.otves.engine.PointM
import uz.otves.engine.PointMm
import uz.otves.engine.RoomKind
import uz.otves.engine.Space

// The names a room can have. The choice names the selected space, or renames its room.
@Composable
internal fun RoomKindsPanel(
    state: EditorUiState,
    actions: EditorActions,
) {
    val colors = OtvesTheme.colors
    val space = (state.selected as? Selected.SpacePart)?.space
    val current = space?.kind?.takeIf { space.room != null }
    Panel {
        PanelHeader(stringResource(R.string.room_kinds_title)) {
            PanelAction(stringResource(R.string.takeoff_close), { actions.onRoomKinds(false) })
        }
        Column(modifier = Modifier.heightIn(max = ListHeight).verticalScroll(rememberScrollState()).selectableGroup()) {
            RoomKind.entries.forEach { kind ->
                val chosen = kind == current
                Row(
                    verticalAlignment = Alignment.CenterVertically,
                    modifier =
                        Modifier
                            .fillMaxWidth()
                            .heightIn(min = OtvesDimens.primaryTarget)
                            .selectable(selected = chosen, role = Role.RadioButton, onClick = { actions.onRoomKind(kind) }),
                ) {
                    Text(roomKindName(kind), style = MaterialTheme.typography.titleMedium, modifier = Modifier.weight(1f))
                    if (chosen) Box(Modifier.size(10.dp).background(colors.ink, CircleShape))
                }
                HorizontalDivider(color = colors.divider)
            }
        }
        Spacer(Modifier.height(8.dp))
    }
}

private val ListHeight = 420.dp

@Preview(widthDp = 400, heightDp = 760)
@Composable
private fun RoomKindsPanelPreview() {
    val floor = listOf(PointM(0.2, 0.2), PointM(4.0, 0.2), PointM(4.0, 3.0), PointM(0.2, 3.0))
    val bedroom = Space(room = null, kind = RoomKind.BEDROOM, area = 11.2, perimeter = 13.6, outline = floor)
    OtvesTheme {
        RoomKindsPanel(
            EditorUiState(ready = true, selected = Selected.SpacePart(PointMm(2_000.mm, 1_500.mm), bedroom)),
            EditorActions(),
        )
    }
}
