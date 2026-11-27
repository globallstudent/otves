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
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.selection.selectable
import androidx.compose.foundation.selection.selectableGroup
import androidx.compose.foundation.shape.CircleShape
import androidx.compose.foundation.verticalScroll
import androidx.compose.material3.HorizontalDivider
import androidx.compose.material3.Icon
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.res.stringResource
import androidx.compose.ui.semantics.Role
import androidx.compose.ui.text.style.TextOverflow
import androidx.compose.ui.tooling.preview.Preview
import androidx.compose.ui.unit.dp
import uz.otves.core.designsystem.OtvesDimens
import uz.otves.core.designsystem.OtvesIcons
import uz.otves.core.designsystem.OtvesTheme
import uz.otves.core.model.ElementId
import uz.otves.core.model.mm
import uz.otves.engine.Catalog
import uz.otves.engine.LayerFunction
import uz.otves.engine.LayerSpec
import uz.otves.engine.MasonryUnitKind
import uz.otves.engine.MasonryUnitOption
import uz.otves.engine.UnitSize
import uz.otves.engine.WallLayer
import uz.otves.engine.WallRole
import uz.otves.engine.WallTypeTemplate
import kotlin.uuid.Uuid

// The job's wall types, then the catalog's make-ups it has not used yet. The choice goes to the
// selected wall, or else to the walls drawn next.
@Composable
internal fun WallKindsPanel(
    state: EditorUiState,
    actions: EditorActions,
) {
    val colors = OtvesTheme.colors
    val wall = state.plan.selected as? Selected.WallPart
    val current = if (wall != null) wall.wall.type else state.tools.kind
    val fresh = state.catalog.wallTypes.filter { template -> state.plan.kinds.none { it.matches(template) } }
    val units = state.catalog.units
    Panel {
        PanelHeader(stringResource(R.string.wall_kind_title)) {
            PanelAction(stringResource(R.string.takeoff_close), { actions.onKinds(false) })
        }
        Column(modifier = Modifier.heightIn(max = ListHeight).verticalScroll(rememberScrollState()).selectableGroup()) {
            state.plan.kinds.forEach { kind ->
                KindRow(
                    name = wallKindName(kind.role, kind.thickness),
                    made = layersText(kind.layers.map { (_, layer) -> LayerSpec(layer.function, layer.thickness, layer.unit) }, units),
                    chosen = kind.id == current,
                    onClick = { actions.onKind(kind.id) },
                    onEdit = { actions.onEditKind(kind.id) },
                )
            }
            // A wall that has a type keeps one; only the walls drawn next may go without.
            if (wall == null) {
                KindRow(
                    name = stringResource(R.string.wall_kind_none),
                    made = stringResource(R.string.wall_kind_none_made),
                    chosen = current == null,
                    onClick = { actions.onKind(null) },
                )
            }
            if (fresh.isNotEmpty()) {
                Text(
                    stringResource(R.string.wall_kind_catalog),
                    style = MaterialTheme.typography.labelLarge,
                    color = colors.inkSecondary,
                    modifier = Modifier.padding(top = 16.dp, bottom = 4.dp),
                )
            }
            fresh.forEach { template ->
                KindRow(
                    name = wallKindName(template.role, template.layers.fold(0.mm) { sum, layer -> sum + layer.thickness }),
                    made = layersText(template.layers, units),
                    chosen = false,
                    onClick = { actions.onAddKind(template) },
                    isNew = true,
                )
            }
        }
        Spacer(Modifier.height(8.dp))
    }
}

// A wall type by its name and what it is made of; the chosen one is marked, a new one adds.
@Composable
private fun KindRow(
    name: String,
    made: String,
    chosen: Boolean,
    onClick: () -> Unit,
    isNew: Boolean = false,
    onEdit: (() -> Unit)? = null,
) {
    val colors = OtvesTheme.colors
    Row(
        verticalAlignment = Alignment.CenterVertically,
        modifier =
            Modifier
                .fillMaxWidth()
                .heightIn(min = OtvesDimens.primaryTarget)
                .selectable(selected = chosen, role = Role.RadioButton, onClick = onClick)
                .padding(vertical = 6.dp),
    ) {
        Column(modifier = Modifier.weight(1f)) {
            Text(name, style = MaterialTheme.typography.titleMedium)
            Text(
                made,
                style = MaterialTheme.typography.bodyMedium,
                color = colors.inkSecondary,
                maxLines = 2,
                overflow = TextOverflow.Ellipsis,
            )
        }
        // A type of the job can be changed: its layers open.
        if (onEdit != null) PanelAction(stringResource(R.string.kind_edit), onEdit)
        when {
            isNew -> Icon(OtvesIcons.Plus, contentDescription = null, tint = colors.inkSecondary, modifier = Modifier.size(20.dp))
            chosen -> Box(Modifier.size(10.dp).background(colors.ink, CircleShape))
        }
    }
    HorizontalDivider(color = colors.divider)
}

private val ListHeight = 420.dp

@Preview(widthDp = 400, heightDp = 760)
@Composable
private fun WallKindsPanelPreview() {
    val brick = UnitSize(250.mm, 120.mm, 65.mm)
    val block = UnitSize(390.mm, 190.mm, 188.mm)
    val type = ElementId(Uuid.parse("019a5f00-0000-7000-8000-000000000007"))
    val layer = { n: Long -> ElementId(Uuid.fromLongs(n, n)) }
    val outside =
        WallKind(
            type,
            WallRole.EXTERIOR,
            listOf(
                layer(1) to WallLayer(type, 0, LayerFunction.FACING, 120.mm, brick),
                layer(2) to WallLayer(type, 1, LayerFunction.INSULATION, 100.mm),
                layer(3) to WallLayer(type, 2, LayerFunction.STRUCTURE, 190.mm, block),
            ),
        )
    val catalog =
        Catalog(
            units =
                listOf(
                    MasonryUnitOption("brick.1nf", MasonryUnitKind.BRICK, "1НФ", brick, listOf(120.mm, 250.mm, 380.mm)),
                    MasonryUnitOption("block", MasonryUnitKind.BLOCK, "", block, listOf(190.mm, 390.mm)),
                ),
            wallTypes =
                listOf(
                    WallTypeTemplate("bearing.brick380", WallRole.BEARING, listOf(LayerSpec(LayerFunction.STRUCTURE, 380.mm, brick))),
                    WallTypeTemplate("partition.brick120", WallRole.PARTITION, listOf(LayerSpec(LayerFunction.STRUCTURE, 120.mm, brick))),
                ),
        )
    OtvesTheme {
        EditorScreen(
            state =
                EditorUiState(
                    ready = true,
                    catalog = catalog,
                    plan = PlanState(kinds = listOf(outside)),
                    tools = ToolState(kind = type),
                    panel = PanelState(kindsOpen = true),
                ),
            actions = EditorActions(),
        )
    }
}
