package uz.otves.feature.editor

import androidx.compose.foundation.horizontalScroll
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.heightIn
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.safeDrawingPadding
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.items
import androidx.compose.foundation.rememberScrollState
import androidx.compose.material3.FilterChip
import androidx.compose.material3.FilterChipDefaults
import androidx.compose.material3.HorizontalDivider
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Surface
import androidx.compose.material3.Text
import androidx.compose.material3.TextButton
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.res.stringResource
import androidx.compose.ui.text.style.TextOverflow
import androidx.compose.ui.tooling.preview.Preview
import androidx.compose.ui.unit.dp
import androidx.lifecycle.compose.collectAsStateWithLifecycle
import androidx.lifecycle.viewmodel.compose.viewModel
import uz.otves.core.designsystem.OtvesDimens
import uz.otves.core.designsystem.OtvesTheme
import uz.otves.core.model.ElementId
import uz.otves.core.model.Mm
import uz.otves.core.model.mm
import uz.otves.engine.Engine
import uz.otves.engine.EngineCommand
import uz.otves.engine.EngineError
import uz.otves.engine.PointM
import uz.otves.engine.PointMm
import uz.otves.engine.QuantityLine
import uz.otves.engine.QuantityUnit
import uz.otves.engine.WallOutline
import kotlin.uuid.Uuid

/** The editor for one storey, wired to the engine; [loadContent] reads the norms and rules. */
@Composable
fun EditorRoute(
    engine: Engine,
    loadContent: suspend () -> EngineCommand.LoadContent,
    modifier: Modifier = Modifier,
) {
    val viewModel = viewModel { EditorViewModel(engine, loadContent) }
    val state by viewModel.uiState.collectAsStateWithLifecycle()
    EditorScreen(
        state = state,
        onTap = viewModel::tap,
        onCancelWall = viewModel::cancelWall,
        onThickness = viewModel::selectThickness,
        onUndo = viewModel::undo,
        onRedo = viewModel::redo,
        modifier = modifier,
    )
}

@Composable
internal fun EditorScreen(
    state: EditorUiState,
    onTap: (PointMm) -> Unit,
    onCancelWall: () -> Unit,
    onThickness: (Mm) -> Unit,
    onUndo: () -> Unit,
    onRedo: () -> Unit,
    modifier: Modifier = Modifier,
) {
    Surface(modifier = modifier.fillMaxSize(), color = OtvesTheme.colors.canvas) {
        Column(modifier = Modifier.safeDrawingPadding()) {
            Row(
                modifier = Modifier.fillMaxWidth().padding(horizontal = OtvesDimens.gutter),
                verticalAlignment = Alignment.CenterVertically,
            ) {
                // One line and the same buttons always: a taller bar would shift the plan under
                // the user's finger between the two taps of a wall.
                Text(
                    text = stringResource(R.string.editor_title),
                    style = MaterialTheme.typography.titleLarge,
                    maxLines = 1,
                    overflow = TextOverflow.Ellipsis,
                    modifier = Modifier.weight(1f),
                )
                TextButton(onClick = onUndo, enabled = state.canUndo, modifier = Modifier.heightIn(min = OtvesDimens.minTarget)) {
                    Text(stringResource(R.string.editor_undo))
                }
                TextButton(onClick = onRedo, enabled = state.canRedo, modifier = Modifier.heightIn(min = OtvesDimens.minTarget)) {
                    Text(stringResource(R.string.editor_redo))
                }
            }
            Row(
                modifier =
                    Modifier
                        .fillMaxWidth()
                        .horizontalScroll(rememberScrollState())
                        .padding(horizontal = OtvesDimens.gutter),
                horizontalArrangement = Arrangement.spacedBy(8.dp),
            ) {
                val chipColors =
                    FilterChipDefaults.filterChipColors(
                        selectedContainerColor = OtvesTheme.colors.ink,
                        selectedLabelColor = OtvesTheme.colors.surface,
                    )
                WallThicknesses.forEach { thickness ->
                    FilterChip(
                        selected = thickness == state.thickness,
                        onClick = { onThickness(thickness) },
                        label = { Text(thicknessName(thickness), maxLines = 1) },
                        colors = chipColors,
                        modifier = Modifier.heightIn(min = OtvesDimens.minTarget),
                    )
                }
            }
            Box(modifier = Modifier.weight(1f).fillMaxWidth()) {
                PlanCanvas(
                    outlines = state.outlines,
                    wallStart = state.wallStart,
                    onTap = onTap,
                    modifier = Modifier.fillMaxSize(),
                )
                val notice =
                    when {
                        !state.ready -> stringResource(R.string.editor_loading)
                        state.refused != null -> refusalText(state.refused)
                        state.wallStart != null -> stringResource(R.string.editor_hint_end)
                        state.outlines.isEmpty() -> stringResource(R.string.editor_hint_start)
                        else -> null
                    }
                Column(
                    modifier = Modifier.align(Alignment.TopCenter).padding(OtvesDimens.gutter),
                    horizontalAlignment = Alignment.CenterHorizontally,
                ) {
                    if (notice != null) {
                        Text(
                            text = notice,
                            style = MaterialTheme.typography.bodyLarge,
                            color = if (state.refused != null) OtvesTheme.colors.warning else OtvesTheme.colors.inkSecondary,
                        )
                    }
                    if (state.wallStart != null) {
                        TextButton(onClick = onCancelWall, modifier = Modifier.heightIn(min = OtvesDimens.minTarget)) {
                            Text(stringResource(R.string.editor_cancel_wall))
                        }
                    }
                }
            }
            TakeoffPanel(state)
        }
    }
}

@Composable
private fun TakeoffPanel(state: EditorUiState) {
    // A fixed height keeps the plan still when lines arrive, so taps land where the user aims.
    Surface(color = OtvesTheme.colors.surface, modifier = Modifier.fillMaxWidth().height(TakeoffHeight)) {
        Column(modifier = Modifier.padding(OtvesDimens.gutter)) {
            Text(stringResource(R.string.takeoff_title), style = MaterialTheme.typography.titleMedium)
            when {
                state.takeoffFailed -> {
                    Text(stringResource(R.string.takeoff_failed), color = OtvesTheme.colors.warning)
                }

                state.takeoff.all { it.amount == 0.0 } -> {
                    Text(stringResource(R.string.takeoff_empty), color = OtvesTheme.colors.inkSecondary)
                }

                else -> {
                    LazyColumn {
                        items(state.takeoff) { line ->
                            Row(modifier = Modifier.fillMaxWidth().padding(vertical = 6.dp)) {
                                Text(itemName(line.item), modifier = Modifier.weight(1f))
                                Text("${amountText(line)} ${unitName(line.unit)}")
                            }
                            HorizontalDivider(color = OtvesTheme.colors.divider)
                        }
                    }
                }
            }
        }
    }
}

@Composable
private fun thicknessName(thickness: Mm): String =
    when (thickness) {
        120.mm -> stringResource(R.string.thickness_half_brick)
        250.mm -> stringResource(R.string.thickness_one_brick)
        380.mm -> stringResource(R.string.thickness_one_and_half_bricks)
        else -> stringResource(R.string.thickness_two_bricks)
    }

@Composable
private fun refusalText(error: EngineError): String =
    when (error) {
        EngineError.ZERO_LENGTH -> stringResource(R.string.refused_zero_length)
        EngineError.OFF_PLAN -> stringResource(R.string.refused_off_plan)
        else -> stringResource(R.string.refused_other)
    }

@Preview(widthDp = 400, heightDp = 760)
@Composable
private fun EditorScreenPreview() {
    val level = ElementId(Uuid.parse("019a5f00-0000-7000-8000-000000000001"))
    val wall = ElementId(Uuid.parse("019a5f00-0000-7000-8000-000000000002"))
    OtvesTheme {
        EditorScreen(
            state =
                EditorUiState(
                    ready = true,
                    outlines =
                        listOf(
                            WallOutline(
                                wall,
                                level,
                                listOf(PointM(-0.19, -0.19), PointM(6.59, -0.19), PointM(6.21, 0.19), PointM(0.19, 0.19)),
                            ),
                        ),
                    takeoff =
                        listOf(
                            QuantityLine("masonry", "masonry_unit", QuantityUnit.PIECE, 2_767.0),
                            QuantityLine("masonry", "mortar", QuantityUnit.CUBIC_METRE, 1.62),
                            QuantityLine("masonry", "mortar.cement", QuantityUnit.KILOGRAM, 485.4),
                            QuantityLine("lintels", "lintel.bar.l20", QuantityUnit.PIECE, 3.0),
                            QuantityLine("ring_beam", "concrete.b15", QuantityUnit.CUBIC_METRE, 0.37),
                        ),
                    canUndo = true,
                ),
            onTap = {},
            onCancelWall = {},
            onThickness = {},
            onUndo = {},
            onRedo = {},
        )
    }
}

private val TakeoffHeight = 240.dp
