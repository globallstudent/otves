package uz.otves.feature.editor

import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.layout.width
import androidx.compose.runtime.Composable
import androidx.compose.ui.graphics.vector.ImageVector
import androidx.compose.ui.res.stringResource
import uz.otves.core.designsystem.OtvesIcons
import uz.otves.engine.EngineError
import uz.otves.engine.IssueKind
import uz.otves.engine.OpeningKind
import uz.otves.engine.Turn
import uz.otves.engine.Wall

@Composable
internal fun issueText(kind: IssueKind): String =
    stringResource(
        when (kind) {
            IssueKind.OPENING_OUTSIDE_WALL -> R.string.issue_outside_wall
            IssueKind.OPENING_OVERLAP -> R.string.issue_overlap
            IssueKind.OPENING_WITHOUT_WALL -> R.string.issue_without_wall
            IssueKind.SHORT_BEARING -> R.string.issue_short_bearing
            IssueKind.NO_ROOM_FOR_LINTEL -> R.string.issue_no_room_for_lintel
            IssueKind.WALL_OVERLAP -> R.string.issue_wall_overlap
            IssueKind.SHARP_ANGLE -> R.string.issue_sharp_angle
            IssueKind.MATERIAL_NOT_CHOSEN -> R.string.issue_material
            IssueKind.RING_BEAM_MISSING -> R.string.issue_ring_beam
            IssueKind.JUNCTION_MESHES_MISSING -> R.string.issue_meshes
            IssueKind.UNKNOWN -> R.string.issue_unknown
        },
    )

@Composable
internal fun kindName(kind: OpeningKind): String =
    when (kind) {
        OpeningKind.WINDOW -> stringResource(R.string.tool_window)
        OpeningKind.DOOR -> stringResource(R.string.tool_door)
        OpeningKind.EMPTY -> stringResource(R.string.kind_empty)
    }

internal fun toolIcon(tool: Tool): ImageVector =
    when (tool) {
        Tool.SELECT -> OtvesIcons.Select
        Tool.PENCIL -> OtvesIcons.Pencil
        Tool.WALL -> OtvesIcons.Wall
        Tool.WINDOW -> OtvesIcons.Window
        Tool.DOOR -> OtvesIcons.Door
    }

@Composable
internal fun turnName(turn: Turn): String =
    stringResource(
        when (turn) {
            Turn.LEFT -> R.string.turn_left
            Turn.STRAIGHT -> R.string.turn_straight
            Turn.RIGHT -> R.string.turn_right
        },
    )

@Composable
internal fun toolName(tool: Tool): String =
    when (tool) {
        Tool.SELECT -> stringResource(R.string.tool_select)
        Tool.PENCIL -> stringResource(R.string.tool_pencil)
        Tool.WALL -> stringResource(R.string.tool_wall)
        Tool.WINDOW -> stringResource(R.string.tool_window)
        Tool.DOOR -> stringResource(R.string.tool_door)
    }

// Millimetres, as on drawings: «1500 × 1500».
@Composable
internal fun sizeName(size: OpeningSize): String = stringResource(R.string.opening_size, size.width.value, size.height.value)

@Composable
internal fun refusalText(error: EngineError): String =
    when (error) {
        EngineError.ZERO_LENGTH -> stringResource(R.string.refused_zero_length)
        EngineError.OFF_PLAN -> stringResource(R.string.refused_off_plan)
        EngineError.NO_WALL -> stringResource(R.string.refused_no_wall)
        EngineError.DOES_NOT_FIT -> stringResource(R.string.refused_does_not_fit)
        EngineError.BREAKS_JOIN -> stringResource(R.string.refused_breaks_join)
        else -> stringResource(R.string.refused_other)
    }
