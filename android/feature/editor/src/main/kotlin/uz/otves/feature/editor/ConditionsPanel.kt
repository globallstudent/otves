package uz.otves.feature.editor

import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.layout.width
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.ui.Modifier
import androidx.compose.ui.res.pluralStringResource
import androidx.compose.ui.res.stringResource
import androidx.compose.ui.semantics.selected
import androidx.compose.ui.unit.dp
import uz.otves.core.designsystem.OtvesTheme
import uz.otves.core.model.mm
import uz.otves.engine.Condition
import uz.otves.engine.IssueKind

// What the job is estimated under. An unset condition shows the norm's value as chosen.
@Composable
internal fun ConditionsPanel(
    state: EditorUiState,
    actions: EditorActions,
) {
    Panel {
        PanelHeader(stringResource(R.string.conditions_title)) {
            PanelAction(stringResource(R.string.inspector_done), { actions.onConditions(false) })
        }
        state.catalog.conditions.forEach { (condition, choices) ->
            val chosen = state.conditions[condition] ?: choices.default
            InspectorRow(conditionName(condition)) {
                choices.values.forEach { value ->
                    Pill(conditionValue(condition, value), value == chosen, { actions.onCondition(condition, value) })
                }
            }
        }
        Spacer(Modifier.height(8.dp))
    }
}

@Composable
private fun conditionName(condition: Condition): String =
    stringResource(
        when (condition) {
            Condition.SEISMIC_INTENSITY -> R.string.condition_seismic
            Condition.CEMENT_GRADE -> R.string.condition_cement
            Condition.MORTAR_GRADE -> R.string.condition_mortar
            Condition.UNIT_WASTE -> R.string.condition_unit_waste
            Condition.MORTAR_WASTE -> R.string.condition_mortar_waste
        },
    )

// Grades as builders write them, waste per mille as a percentage.
@Composable
private fun conditionValue(
    condition: Condition,
    value: Int,
): String =
    when (condition) {
        Condition.SEISMIC_INTENSITY -> value.toString()
        Condition.CEMENT_GRADE -> stringResource(R.string.grade_cement, value)
        Condition.MORTAR_GRADE -> stringResource(R.string.grade_mortar, value)
        Condition.UNIT_WASTE, Condition.MORTAR_WASTE -> stringResource(R.string.percent, value / PER_MILLE_IN_PERCENT)
    }

// What the shown storey is built with beyond its walls: each choice adds its materials below.
@Composable
internal fun StoreyChoices(
    state: EditorUiState,
    actions: EditorActions,
) {
    Column {
        val unchosen = state.unchosen.size
        if (unchosen > 0) {
            Text(
                pluralStringResource(R.plurals.takeoff_unchosen, unchosen, unchosen),
                style = MaterialTheme.typography.bodyMedium,
                color = OtvesTheme.colors.warning,
            )
        }
        InspectorRow(stringResource(R.string.level_footing)) {
            val footing = state.footing
            Pill(stringResource(R.string.option_none), footing == null, actions.onNoFooting)
            Pill(
                label =
                    if (footing == null) {
                        stringResource(R.string.footing_set)
                    } else {
                        stringResource(R.string.footing_section, grouped(footing.width), grouped(footing.depth))
                    },
                selected = footing != null,
                onClick = actions.onFooting,
            )
        }
        InspectorRow(stringResource(R.string.level_ring_beam)) {
            Pill(stringResource(R.string.option_none), state.ringBeam.value == 0L, { actions.onRingBeam(0.mm) })
            state.catalog.ringBeamHeights.forEach { height ->
                Pill(grouped(height), state.ringBeam == height, { actions.onRingBeam(height) })
            }
        }
        Advice(state, IssueKind.RING_BEAM_MISSING)
        InspectorRow(stringResource(R.string.level_meshes)) {
            Pill(stringResource(R.string.option_none), !state.junctionMeshes, { actions.onJunctionMeshes(false) })
            Pill(stringResource(R.string.option_yes), state.junctionMeshes, { actions.onJunctionMeshes(true) })
        }
        Advice(state, IssueKind.JUNCTION_MESHES_MISSING)
        Spacer(Modifier.height(8.dp))
    }
}

// What the norms ask of the storey and it lacks. It is said, never added: the builder decides.
@Composable
private fun Advice(
    state: EditorUiState,
    kind: IssueKind,
) {
    if (state.issues.none { it.kind == kind }) return
    Text(issueText(kind), style = MaterialTheme.typography.bodyMedium, color = OtvesTheme.colors.warning)
}

private const val PER_MILLE_IN_PERCENT = 10
