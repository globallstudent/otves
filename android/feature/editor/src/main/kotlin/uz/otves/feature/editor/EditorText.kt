package uz.otves.feature.editor

import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.layout.width
import androidx.compose.runtime.Composable
import androidx.compose.ui.graphics.vector.ImageVector
import androidx.compose.ui.res.stringResource
import uz.otves.core.designsystem.OtvesIcons
import uz.otves.core.model.Mm
import uz.otves.engine.EngineError
import uz.otves.engine.IssueKind
import uz.otves.engine.LayerFunction
import uz.otves.engine.LayerSpec
import uz.otves.engine.MasonryUnitOption
import uz.otves.engine.OpeningKind
import uz.otves.engine.Turn
import uz.otves.engine.Wall
import uz.otves.engine.WallRole

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
            IssueKind.ROOM_WITHOUT_SPACE -> R.string.issue_room_without_space
            IssueKind.ROOM_SHARES_SPACE -> R.string.issue_room_shares_space
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

@Composable
internal fun roleName(role: WallRole): String =
    stringResource(
        when (role) {
            WallRole.EXTERIOR -> R.string.role_exterior
            WallRole.BEARING -> R.string.role_bearing
            WallRole.PARTITION -> R.string.role_partition
        },
    )

// A wall type has no name of its own: it is called by what it is for and how thick it is,
// «Наружная 410».
@Composable
internal fun wallKindName(
    role: WallRole,
    thickness: Mm,
): String = stringResource(R.string.wall_kind_name, roleName(role), grouped(thickness))

// What a wall is made of from the outer face in, each layer's thickness first:
// «120 кирпич 1НФ, 100 утеплитель, 190 блок 390 × 190 × 188».
@Composable
internal fun layersText(
    layers: List<LayerSpec>,
    units: List<MasonryUnitOption>,
): String {
    val names =
        layers.map { layer ->
            val what =
                when {
                    layer.function == LayerFunction.INSULATION -> stringResource(R.string.layer_insulation)
                    layer.function == LayerFunction.GAP -> stringResource(R.string.layer_gap)
                    layer.unit != null -> unitTitle(layer.unit!!, units).replaceFirstChar { it.lowercase() }
                    else -> stringResource(R.string.layer_masonry)
                }
            stringResource(R.string.layer_line, grouped(layer.thickness), what)
        }
    return names.joinToString(", ")
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
        EngineError.TYPE_DECIDES -> stringResource(R.string.refused_type_decides)
        else -> stringResource(R.string.refused_other)
    }
