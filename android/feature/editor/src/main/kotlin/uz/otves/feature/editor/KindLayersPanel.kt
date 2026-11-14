package uz.otves.feature.editor

import androidx.compose.foundation.horizontalScroll
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.rememberScrollState
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.ui.Modifier
import androidx.compose.ui.res.stringResource
import androidx.compose.ui.unit.dp
import uz.otves.core.designsystem.OtvesTheme
import uz.otves.engine.LayerFunction
import uz.otves.engine.WallLayer

// One wall type's layers from the outer face in (docs/adr/0014). A masonry layer is as thick
// as its unit builds; insulation and a gap are as thick as the builder types. Every wall of
// the type follows, and so does the take-off.
@Composable
internal fun KindLayersPanel(
    kind: WallKind,
    state: EditorUiState,
    actions: EditorActions,
) {
    val colors = OtvesTheme.colors
    Panel {
        PanelHeader(wallKindName(kind.role, kind.thickness)) {
            PanelAction(stringResource(R.string.inspector_done), { actions.onEditKind(null) })
        }
        Text(stringResource(R.string.kind_layers_order), style = MaterialTheme.typography.bodyMedium, color = colors.inkSecondary)
        kind.layers.forEach { (id, layer) ->
            Column(modifier = Modifier.padding(top = 12.dp)) {
                Text(layerTitle(layer, state), style = MaterialTheme.typography.bodyLarge)
                Row(
                    modifier = Modifier.horizontalScroll(rememberScrollState()).padding(top = 4.dp),
                    horizontalArrangement = Arrangement.spacedBy(8.dp),
                ) {
                    val unit = layer.unit
                    if (unit == null) {
                        Pill(stringResource(R.string.length_value, grouped(layer.thickness)), false, { actions.onEditLayer(id) })
                    } else {
                        // The thicknesses the unit builds, and the layer's own if it is another.
                        val choices = (state.thicknessesOf(unit) + layer.thickness).distinct().sortedBy { it.value }
                        choices.forEach { choice ->
                            Pill(grouped(choice), choice == layer.thickness, { actions.onLayerThickness(id, choice) })
                        }
                    }
                }
            }
        }
        Spacer(Modifier.height(8.dp))
    }
}

// What the layer is for and, for masonry, what it is laid from: «Облицовка, кирпич 1НФ».
@Composable
private fun layerTitle(
    layer: WallLayer,
    state: EditorUiState,
): String {
    val purpose =
        stringResource(
            when (layer.function) {
                LayerFunction.FACING -> R.string.layer_facing
                LayerFunction.STRUCTURE -> R.string.legend_masonry
                LayerFunction.INSULATION -> R.string.legend_insulation
                LayerFunction.GAP -> R.string.layer_gap_title
            },
        )
    val unit = layer.unit ?: return purpose
    return stringResource(R.string.layer_of, purpose, unitTitle(unit, state.catalog.units).replaceFirstChar { it.lowercase() })
}
