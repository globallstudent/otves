package uz.otves.feature.editor

import androidx.compose.foundation.background
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.heightIn
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.shape.CircleShape
import androidx.compose.material3.HorizontalDivider
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Surface
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.res.stringResource
import androidx.compose.ui.semantics.selected
import androidx.compose.ui.semantics.semantics
import androidx.compose.ui.unit.dp
import uz.otves.core.designsystem.OtvesDimens
import uz.otves.core.designsystem.OtvesTheme

// What walls are laid from: the catalog's units by name and size as laid. The choice goes to the
// selected wall, or else to the walls drawn next.
@Composable
internal fun MaterialPanel(
    state: EditorUiState,
    actions: EditorActions,
) {
    val colors = OtvesTheme.colors
    val current = (state.selected as? Selected.WallPart)?.let { it.wall.unit } ?: state.material.takeIf { state.selected == null }
    Panel {
        PanelHeader(stringResource(R.string.material_title)) {
            PanelAction(stringResource(R.string.takeoff_close), { actions.onMaterials(false) })
        }
        state.catalog.units.forEach { option ->
            val chosen = option.size == current
            Surface(
                onClick = { actions.onMaterial(option) },
                color = colors.surface,
                modifier = Modifier.fillMaxWidth().semantics { selected = chosen },
            ) {
                Row(
                    modifier = Modifier.heightIn(min = OtvesDimens.primaryTarget).padding(vertical = 6.dp),
                    verticalAlignment = Alignment.CenterVertically,
                ) {
                    Column(modifier = Modifier.weight(1f)) {
                        Text(unitTitle(option.size, state.catalog.units), style = MaterialTheme.typography.titleMedium)
                        Text(unitSize(option.size), style = MaterialTheme.typography.bodyMedium, color = colors.inkSecondary)
                    }
                    if (chosen) Box(Modifier.size(10.dp).background(colors.ink, CircleShape))
                }
            }
            HorizontalDivider(color = colors.divider)
        }
        PanelAction(stringResource(R.string.material_own), actions.onOwnSize)
        Spacer(Modifier.height(8.dp))
    }
}
