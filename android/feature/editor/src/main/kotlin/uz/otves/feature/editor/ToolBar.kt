package uz.otves.feature.editor

import androidx.compose.foundation.background
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.FlowRow
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.selection.selectable
import androidx.compose.foundation.selection.selectableGroup
import androidx.compose.foundation.shape.CircleShape
import androidx.compose.material3.Icon
import androidx.compose.material3.Surface
import androidx.compose.runtime.Composable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.res.stringResource
import androidx.compose.ui.semantics.Role
import androidx.compose.ui.semantics.contentDescription
import androidx.compose.ui.semantics.semantics
import androidx.compose.ui.unit.dp
import uz.otves.core.designsystem.OtvesDimens
import uz.otves.core.designsystem.OtvesIcons
import uz.otves.core.designsystem.OtvesTheme

// What the run of walls offers next, over undo, redo and the tools themselves.
@Composable
internal fun Tools(
    state: EditorUiState,
    actions: EditorActions,
) {
    Column(modifier = Modifier.padding(bottom = OtvesDimens.gutter), verticalArrangement = Arrangement.spacedBy(12.dp)) {
        RunActions(state, actions)
        Row(
            modifier = Modifier.fillMaxWidth().padding(horizontal = OtvesDimens.gutter),
            verticalAlignment = Alignment.CenterVertically,
            horizontalArrangement = Arrangement.spacedBy(8.dp),
        ) {
            RoundButton(OtvesIcons.Undo, stringResource(R.string.editor_undo), actions.onUndo, enabled = state.canUndo)
            RoundButton(OtvesIcons.Redo, stringResource(R.string.editor_redo), actions.onRedo, enabled = state.canRedo)
            Spacer(Modifier.weight(1f))
            ToolPill(state.tool, actions.onTool)
        }
    }
}

// Every tool stays visible as an icon; the chosen one is filled. A name here would take the room
// of a tool, and the line under the top bar already says what the next touch does.
@Composable
private fun ToolPill(
    current: Tool,
    onTool: (Tool) -> Unit,
) {
    val colors = OtvesTheme.colors
    Surface(shape = CircleShape, color = colors.ink, contentColor = colors.surface, shadowElevation = 8.dp) {
        Row(
            modifier = Modifier.height(OtvesDimens.primaryTarget).padding(horizontal = 4.dp).selectableGroup(),
            verticalAlignment = Alignment.CenterVertically,
        ) {
            Tool.entries.forEach { tool ->
                val chosen = tool == current
                val name = toolName(tool)
                Box(
                    contentAlignment = Alignment.Center,
                    modifier =
                        Modifier
                            .size(OtvesDimens.minTarget)
                            .clip(CircleShape)
                            .selectable(selected = chosen, role = Role.Tab, onClick = { onTool(tool) })
                            .semantics { contentDescription = name },
                ) {
                    Box(
                        contentAlignment = Alignment.Center,
                        modifier = Modifier.size(ChosenTool).background(if (chosen) colors.surface else colors.ink, CircleShape),
                    ) {
                        Icon(
                            toolIcon(tool),
                            contentDescription = null,
                            tint = if (chosen) colors.ink else colors.surface,
                            modifier = Modifier.size(22.dp),
                        )
                    }
                }
            }
        }
    }
}

// What a run of walls offers while it is drawn. The tool's settings are behind the options button.
@Composable
private fun RunActions(
    state: EditorUiState,
    actions: EditorActions,
) {
    if (state.tool != Tool.WALL) return
    // Wraps, so no command hides past the edge of the screen.
    FlowRow(
        modifier = Modifier.fillMaxWidth().padding(horizontal = OtvesDimens.gutter),
        horizontalArrangement = Arrangement.spacedBy(8.dp),
        verticalArrangement = Arrangement.spacedBy(8.dp),
    ) {
        // The wall just drawn: its length is typed here, as read off the tape.
        state.dimensions.find { it.id == state.drawn }?.let { drawn ->
            Pill(stringResource(R.string.length_value, grouped(drawn.length)), false, { actions.onEditLength(drawn.id) })
        }
        if (state.draft != null) {
            Pill(stringResource(R.string.measure_start), false, actions.onMeasure)
            Pill(stringResource(R.string.inspector_done), false, actions.onFinishWall)
        }
    }
}

private val ChosenTool = 44.dp
