package uz.otves.feature.editor

import androidx.compose.animation.core.animateOffsetAsState
import androidx.compose.animation.core.snap
import androidx.compose.animation.core.spring
import androidx.compose.foundation.BorderStroke
import androidx.compose.foundation.background
import androidx.compose.foundation.clickable
import androidx.compose.foundation.gestures.detectDragGestures
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.BoxWithConstraints
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.heightIn
import androidx.compose.foundation.layout.offset
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.selection.selectable
import androidx.compose.foundation.selection.toggleable
import androidx.compose.foundation.shape.CircleShape
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.HorizontalDivider
import androidx.compose.material3.Icon
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.RadioButton
import androidx.compose.material3.RadioButtonDefaults
import androidx.compose.material3.Surface
import androidx.compose.material3.Switch
import androidx.compose.material3.SwitchDefaults
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.saveable.listSaver
import androidx.compose.runtime.saveable.rememberSaveable
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.alpha
import androidx.compose.ui.geometry.Offset
import androidx.compose.ui.input.pointer.pointerInput
import androidx.compose.ui.platform.LocalDensity
import androidx.compose.ui.res.stringResource
import androidx.compose.ui.semantics.Role
import androidx.compose.ui.semantics.contentDescription
import androidx.compose.ui.semantics.semantics
import androidx.compose.ui.semantics.stateDescription
import androidx.compose.ui.unit.IntOffset
import androidx.compose.ui.unit.IntRect
import androidx.compose.ui.unit.IntSize
import androidx.compose.ui.unit.LayoutDirection
import androidx.compose.ui.unit.dp
import androidx.compose.ui.window.Popup
import androidx.compose.ui.window.PopupPositionProvider
import androidx.compose.ui.window.PopupProperties
import uz.otves.core.designsystem.OtvesDimens
import uz.otves.core.designsystem.OtvesIcons
import uz.otves.core.designsystem.OtvesTheme
import uz.otves.core.designsystem.SegmentedControl
import kotlin.math.roundToInt

/**
 * Where the options button rests: against the right or the left edge, [up] of the way from the
 * tools to the top bar. The builder drags it to the hand that holds the phone.
 */
private data class Rest(
    val right: Boolean = true,
    val up: Float = 0f,
)

private val RestSaver = listSaver<Rest, Any>(save = { listOf(it.right, it.up) }, restore = { Rest(it[0] as Boolean, it[1] as Float) })

/** Whether [tool] has settings to offer. */
internal fun hasOptions(tool: Tool): Boolean = tool != Tool.SELECT

/**
 * One button for the chosen tool's settings, as a drawing program's options bar: a tap opens
 * them beside it, a drag moves it. It fades while a wall is aimed, so it never hides the line.
 */
@Composable
internal fun ToolOptionsButton(
    state: EditorUiState,
    actions: EditorActions,
    modifier: Modifier = Modifier,
) {
    var rest by rememberSaveable(stateSaver = RestSaver) { mutableStateOf(Rest()) }
    var dragged by remember { mutableStateOf<Offset?>(null) }
    var open by remember { mutableStateOf(false) }
    val colors = OtvesTheme.colors
    BoxWithConstraints(modifier = modifier.fillMaxSize()) {
        val bounds =
            with(LocalDensity.current) {
                Bounds(
                    width = maxWidth.toPx(),
                    top = TopInset.toPx(),
                    bottom = maxHeight.toPx() - BottomInset.toPx() - ButtonSize.toPx(),
                    edge = EdgeGap.toPx(),
                    size = ButtonSize.toPx(),
                )
            }
        val at by animateOffsetAsState(
            targetValue = dragged ?: bounds.place(rest),
            animationSpec = if (dragged == null) spring() else snap(),
            label = "options button",
        )
        val aiming = state.tool == Tool.WALL && state.draft?.to != null
        val description = stringResource(R.string.tool_options)
        val opened = stringResource(R.string.tool_options_open)
        Box(
            modifier =
                Modifier
                    .offset { IntOffset(at.x.roundToInt(), at.y.roundToInt()) }
                    .alpha(if (aiming && !open) AIMING_ALPHA else 1f)
                    .pointerInput(bounds) {
                        detectDragGestures(
                            onDragStart = {
                                open = false
                                dragged = bounds.place(rest)
                            },
                            onDrag = { change, by ->
                                change.consume()
                                dragged = dragged?.let { bounds.within(it + by) }
                            },
                            onDragEnd = {
                                dragged?.let { rest = bounds.rest(it) }
                                dragged = null
                            },
                            onDragCancel = { dragged = null },
                        )
                    },
        ) {
            Surface(
                onClick = { open = !open },
                shape = CircleShape,
                color = if (open) colors.ink else colors.surface,
                contentColor = if (open) colors.surface else colors.ink,
                border = BorderStroke(1.5.dp, colors.ink),
                shadowElevation = 6.dp,
                modifier =
                    Modifier.size(ButtonSize).semantics {
                        contentDescription = description
                        if (open) stateDescription = opened
                    },
            ) {
                Box(contentAlignment = Alignment.Center) {
                    Icon(if (open) OtvesIcons.Close else OtvesIcons.Tune, contentDescription = null, modifier = Modifier.size(22.dp))
                }
            }
            // A wall drawn without a material is left out of the take-off; the dot says where to choose one.
            val builds = state.tool == Tool.WALL || state.tool == Tool.PENCIL
            if (builds && state.material == null && !open) {
                Box(Modifier.align(Alignment.TopEnd).size(12.dp).background(colors.warning, CircleShape))
            }
            if (open) {
                Popup(
                    popupPositionProvider = remember { BesideButton },
                    onDismissRequest = { open = false },
                    properties = PopupProperties(focusable = true),
                ) {
                    OptionsPanel(state, actions, onClose = { open = false })
                }
            }
        }
    }
}

// The screen the button may rest on, in pixels.
private data class Bounds(
    val width: Float,
    val top: Float,
    val bottom: Float,
    val edge: Float,
    val size: Float,
) {
    fun place(rest: Rest): Offset = Offset(if (rest.right) width - edge - size else edge, bottom - (bottom - top) * rest.up)

    fun within(at: Offset): Offset = Offset(at.x.coerceIn(edge, width - edge - size), at.y.coerceIn(top, bottom))

    // The nearer edge, at the height it was let go.
    fun rest(at: Offset): Rest =
        Rest(
            right = at.x + size / 2 > width / 2,
            up = if (bottom > top) ((bottom - at.y) / (bottom - top)).coerceIn(0f, 1f) else 0f,
        )
}

// Above the button when there is room, else below it; flush with the button's outer side.
private object BesideButton : PopupPositionProvider {
    override fun calculatePosition(
        anchorBounds: IntRect,
        windowSize: IntSize,
        layoutDirection: LayoutDirection,
        popupContentSize: IntSize,
    ): IntOffset {
        val onRight = anchorBounds.center.x > windowSize.width / 2
        val x = if (onRight) anchorBounds.right - popupContentSize.width else anchorBounds.left
        val above = anchorBounds.top - PANEL_GAP - popupContentSize.height
        val y = if (above >= 0) above else anchorBounds.bottom + PANEL_GAP
        return IntOffset(
            x.coerceIn(0, (windowSize.width - popupContentSize.width).coerceAtLeast(0)),
            y.coerceIn(0, (windowSize.height - popupContentSize.height).coerceAtLeast(0)),
        )
    }
}

@Composable
private fun OptionsPanel(
    state: EditorUiState,
    actions: EditorActions,
    onClose: () -> Unit,
) {
    val colors = OtvesTheme.colors
    Surface(
        color = colors.surface,
        shape = RoundedCornerShape(16.dp),
        border = BorderStroke(1.dp, colors.border),
        shadowElevation = 12.dp,
        modifier = Modifier.width(PanelWidth),
    ) {
        Column {
            Text(
                toolName(state.tool),
                style = MaterialTheme.typography.labelLarge,
                color = colors.inkSecondary,
                modifier = Modifier.padding(horizontal = 16.dp, vertical = 12.dp),
            )
            HorizontalDivider(color = colors.divider)
            when (state.tool) {
                Tool.SELECT -> {
                    // Nothing to choose before something is picked.
                }

                Tool.PENCIL, Tool.WALL -> {
                    WallOptions(state, actions, onClose)
                }

                Tool.WINDOW -> {
                    SizeChoices(WindowSizes, state.windowSize, actions.onWindowSize)
                }

                Tool.DOOR -> {
                    SizeChoices(DoorSizes, state.doorSize, actions.onDoorSize)
                }
            }
        }
    }
}

// What the walls drawn next are built like, by the wall tool or from a sketch.
@Composable
private fun WallOptions(
    state: EditorUiState,
    actions: EditorActions,
    onClose: () -> Unit,
) {
    val colors = OtvesTheme.colors
    val material = state.material
    OptionRow(
        label = stringResource(R.string.inspector_material),
        onClick = {
            onClose()
            actions.onMaterials(true)
        },
    ) {
        Text(
            if (material == null) stringResource(R.string.material_choose) else unitTitle(material, state.catalog.units),
            style = MaterialTheme.typography.titleSmall,
            color = if (material == null) colors.warning else colors.ink,
            maxLines = 1,
        )
        Icon(OtvesIcons.ChevronRight, contentDescription = null, tint = colors.inkTertiary, modifier = Modifier.size(18.dp))
    }
    HorizontalDivider(color = colors.divider)
    val thicknesses = state.thicknessesOf(material)
    Column(modifier = Modifier.padding(horizontal = 16.dp, vertical = 10.dp)) {
        Text(
            stringResource(R.string.inspector_thickness),
            style = MaterialTheme.typography.bodyMedium,
            color = colors.inkSecondary,
        )
        SegmentedControl(
            options = thicknesses.map { grouped(it) },
            selected = thicknesses.indexOf(state.thickness),
            onSelect = { actions.onThickness(thicknesses[it]) },
            modifier = Modifier.padding(top = 8.dp).fillMaxWidth(),
        )
    }
    HorizontalDivider(color = colors.divider)
    val ortho = stringResource(R.string.options_ortho)
    Row(
        verticalAlignment = Alignment.CenterVertically,
        modifier =
            Modifier
                .fillMaxWidth()
                .heightIn(min = RowHeight)
                .toggleable(value = state.ortho, role = Role.Switch, onValueChange = { actions.onOrtho() })
                .padding(horizontal = 16.dp),
    ) {
        Text(ortho, style = MaterialTheme.typography.bodyLarge, modifier = Modifier.weight(1f))
        Switch(
            checked = state.ortho,
            onCheckedChange = null,
            colors =
                SwitchDefaults.colors(
                    checkedTrackColor = colors.ink,
                    checkedThumbColor = colors.surface,
                    uncheckedTrackColor = colors.field,
                    uncheckedThumbColor = colors.inkTertiary,
                    uncheckedBorderColor = colors.border,
                ),
        )
    }
}

@Composable
private fun OptionRow(
    label: String,
    onClick: () -> Unit,
    value: @Composable () -> Unit,
) {
    Row(
        verticalAlignment = Alignment.CenterVertically,
        modifier =
            Modifier
                .fillMaxWidth()
                .height(RowHeight)
                .clickable(onClick = onClick)
                .padding(start = 16.dp, end = 10.dp),
    ) {
        Text(label, style = MaterialTheme.typography.bodyLarge, modifier = Modifier.weight(1f))
        value()
    }
}

// The sizes a new window or door is offered in; the chosen one is marked.
@Composable
private fun SizeChoices(
    sizes: List<OpeningSize>,
    chosen: OpeningSize,
    onSize: (OpeningSize) -> Unit,
) {
    val colors = OtvesTheme.colors
    sizes.forEachIndexed { index, size ->
        if (index > 0) HorizontalDivider(color = colors.divider)
        Row(
            verticalAlignment = Alignment.CenterVertically,
            modifier =
                Modifier
                    .fillMaxWidth()
                    .height(RowHeight)
                    .selectable(selected = size == chosen, role = Role.RadioButton, onClick = { onSize(size) })
                    .padding(horizontal = 16.dp),
        ) {
            Text(sizeName(size), style = MaterialTheme.typography.bodyLarge, modifier = Modifier.weight(1f))
            RadioButton(
                selected = size == chosen,
                onClick = null,
                colors = RadioButtonDefaults.colors(selectedColor = colors.ink, unselectedColor = colors.inkTertiary),
            )
        }
    }
}

private val ButtonSize = 52.dp
private val EdgeGap = 8.dp

// Clear of the top bar with its line of advice, and of the tools with the run's commands over them.
private val TopInset = 124.dp
private val BottomInset = 148.dp
private val PanelWidth = 296.dp
private val RowHeight = OtvesDimens.minTarget
private const val PANEL_GAP = 16
private const val AIMING_ALPHA = 0.4f
