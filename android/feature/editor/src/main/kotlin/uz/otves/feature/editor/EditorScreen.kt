package uz.otves.feature.editor

import android.content.Context
import android.content.Intent
import androidx.compose.foundation.BorderStroke
import androidx.compose.foundation.background
import androidx.compose.foundation.horizontalScroll
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.FlowRow
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.heightIn
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.safeDrawingPadding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.items
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.selection.selectableGroup
import androidx.compose.foundation.shape.CircleShape
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.DropdownMenu
import androidx.compose.material3.DropdownMenuItem
import androidx.compose.material3.HorizontalDivider
import androidx.compose.material3.Icon
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.ProvideTextStyle
import androidx.compose.material3.Surface
import androidx.compose.material3.Text
import androidx.compose.material3.TextButton
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableIntStateOf
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.rememberCoroutineScope
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.vector.ImageVector
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.res.pluralStringResource
import androidx.compose.ui.res.stringResource
import androidx.compose.ui.semantics.Role
import androidx.compose.ui.semantics.contentDescription
import androidx.compose.ui.semantics.role
import androidx.compose.ui.semantics.selected
import androidx.compose.ui.semantics.semantics
import androidx.compose.ui.semantics.stateDescription
import androidx.compose.ui.text.style.TextOverflow
import androidx.compose.ui.tooling.preview.Preview
import androidx.compose.ui.unit.dp
import androidx.lifecycle.compose.collectAsStateWithLifecycle
import androidx.lifecycle.viewmodel.compose.viewModel
import kotlinx.coroutines.launch
import uz.otves.core.designsystem.OtvesDimens
import uz.otves.core.designsystem.OtvesIcons
import uz.otves.core.designsystem.OtvesTheme
import uz.otves.core.designsystem.SegmentedControl
import uz.otves.core.model.ElementId
import uz.otves.core.model.Mm
import uz.otves.core.model.Money
import uz.otves.core.model.mm
import uz.otves.data.catalog.Prices
import uz.otves.data.project.JobStore
import uz.otves.engine.Catalog
import uz.otves.engine.Condition
import uz.otves.engine.Engine
import uz.otves.engine.EngineCommand
import uz.otves.engine.EngineError
import uz.otves.engine.IssueKind
import uz.otves.engine.MasonryUnitOption
import uz.otves.engine.OpeningKind
import uz.otves.engine.OpeningOutline
import uz.otves.engine.PointM
import uz.otves.engine.PointMm
import uz.otves.engine.QuantityLine
import uz.otves.engine.Wall
import uz.otves.engine.WallOutline
import java.util.Locale
import kotlin.uuid.Uuid

/**
 * The editor of one job, wired to the engine; [loadContent] reads the norms and rules,
 * [loadCatalog] what the builder chooses from, [openJob] opens the job file, which the editor
 * closes when it leaves, and [prices] is the builder's price list.
 */
@Composable
fun EditorRoute(
    engine: Engine,
    loadContent: suspend () -> EngineCommand.LoadContent,
    loadCatalog: suspend () -> Catalog,
    openJob: () -> JobStore,
    prices: Prices,
    onBack: () -> Unit,
    modifier: Modifier = Modifier,
) {
    val viewModel = viewModel { EditorViewModel(engine, loadContent, loadCatalog, openJob(), prices) }
    val state by viewModel.uiState.collectAsStateWithLifecycle()
    EditorScreen(
        state = state,
        actions =
            EditorActions(
                onBack = onBack,
                onTap = viewModel::tap,
                onAim = viewModel::aim,
                onAimCancel = viewModel::cancelAim,
                onCommit = viewModel::commit,
                onFinishWall = viewModel::finishWall,
                onOrtho = viewModel::toggleOrtho,
                onTool = viewModel::selectTool,
                onThickness = viewModel::selectThickness,
                onWindowSize = viewModel::selectWindowSize,
                onDoorSize = viewModel::selectDoorSize,
                onLevel = viewModel::selectLevel,
                onAddLevel = viewModel::addLevel,
                onUndo = viewModel::undo,
                onRedo = viewModel::redo,
                onDeselect = viewModel::deselect,
                onWallThickness = viewModel::setWallThickness,
                onOpeningKind = viewModel::setOpeningKind,
                onOpeningSize = viewModel::setOpeningSize,
                onDelete = viewModel::removeSelected,
                onTakeoff = viewModel::showTakeoff,
                onEditLength = viewModel::editLength,
                onDigit = viewModel::typeDigit,
                onErase = viewModel::eraseDigit,
                onCancelLength = viewModel::cancelLength,
                onConfirmLength = viewModel::confirmLength,
                onMeasure = viewModel::startMeasure,
                onStopMeasure = viewModel::stopMeasure,
                onTurn = viewModel::setTurn,
                onNextMeasured = viewModel::nextMeasured,
                onCloseMeasured = viewModel::closeMeasured,
                onMaterials = viewModel::showMaterials,
                onMaterial = viewModel::chooseMaterial,
                onOwnSize = viewModel::startSize,
                onSizeField = viewModel::selectSizeField,
                onCancelSize = viewModel::cancelSize,
                onConfirmSize = viewModel::confirmSize,
                onPrice = viewModel::editPrice,
                onCancelPrice = viewModel::cancelPrice,
                onConfirmPrice = viewModel::confirmPrice,
                onConditions = viewModel::showConditions,
                onCondition = viewModel::setCondition,
                onRingBeam = viewModel::setRingBeam,
                onFooting = viewModel::startFooting,
                onNoFooting = { viewModel.setFooting(null) },
                onJunctionMeshes = viewModel::setJunctionMeshes,
            ),
        modifier = modifier,
    )
}

/** What the editor's controls do; the route wires them to the ViewModel. */
internal data class EditorActions(
    val onBack: () -> Unit = {},
    val onTap: (at: PointMm, reach: Mm) -> Unit = { _, _ -> },
    val onAim: (at: PointMm, reach: Mm) -> Unit = { _, _ -> },
    val onAimCancel: () -> Unit = {},
    val onCommit: () -> Unit = {},
    val onFinishWall: () -> Unit = {},
    val onOrtho: () -> Unit = {},
    val onTool: (Tool) -> Unit = {},
    val onThickness: (Mm) -> Unit = {},
    val onWindowSize: (OpeningSize) -> Unit = {},
    val onDoorSize: (OpeningSize) -> Unit = {},
    val onLevel: (ElementId) -> Unit = {},
    val onAddLevel: () -> Unit = {},
    val onUndo: () -> Unit = {},
    val onRedo: () -> Unit = {},
    val onDeselect: () -> Unit = {},
    val onWallThickness: (Mm) -> Unit = {},
    val onOpeningKind: (OpeningKind) -> Unit = {},
    val onOpeningSize: (OpeningSize) -> Unit = {},
    val onDelete: () -> Unit = {},
    val onTakeoff: (open: Boolean) -> Unit = {},
    val onEditLength: (ElementId) -> Unit = {},
    val onDigit: (Int) -> Unit = {},
    val onErase: () -> Unit = {},
    val onCancelLength: () -> Unit = {},
    val onConfirmLength: () -> Unit = {},
    val onMeasure: () -> Unit = {},
    val onStopMeasure: () -> Unit = {},
    val onTurn: (Turn) -> Unit = {},
    val onNextMeasured: () -> Unit = {},
    val onCloseMeasured: () -> Unit = {},
    val onMaterials: (open: Boolean) -> Unit = {},
    val onMaterial: (MasonryUnitOption) -> Unit = {},
    val onOwnSize: () -> Unit = {},
    val onSizeField: (Int) -> Unit = {},
    val onCancelSize: () -> Unit = {},
    val onConfirmSize: () -> Unit = {},
    val onPrice: (QuantityLine) -> Unit = {},
    val onCancelPrice: () -> Unit = {},
    val onConfirmPrice: () -> Unit = {},
    val onConditions: (open: Boolean) -> Unit = {},
    val onCondition: (Condition, Int) -> Unit = { _, _ -> },
    val onRingBeam: (Mm) -> Unit = {},
    val onFooting: () -> Unit = {},
    val onNoFooting: () -> Unit = {},
    val onJunctionMeshes: (Boolean) -> Unit = {},
)

// The plan fills the screen; controls float over it: where you are along the top, tools along the
// bottom. A panel (number pad, inspector, take-off) takes the tools' place, one at a time.
@Composable
internal fun EditorScreen(
    state: EditorUiState,
    actions: EditorActions,
    modifier: Modifier = Modifier,
) {
    // Counts requests to show the whole plan: the button, a job opening, a measured run ending.
    var fit by remember { mutableIntStateOf(0) }
    val measuring = state.measure != null
    LaunchedEffect(measuring, state.ready) { if (!measuring && state.ready) fit++ }
    val listOpen = state.takeoffOpen || state.materialOpen
    Surface(modifier = modifier.fillMaxSize(), color = OtvesTheme.colors.canvas) {
        Box(modifier = Modifier.safeDrawingPadding()) {
            PlanCanvas(
                outlines = state.outlines,
                openings = state.openings,
                below = state.below,
                selected = state.selected?.id,
                issues = state.issues,
                dimensions = state.dimensions,
                unchosen = state.unchosen,
                draft = state.draft,
                draftThickness = state.thickness,
                drawing = state.tool == Tool.WALL && state.entry == null && state.measure == null && !listOpen,
                fit = fit,
                // The measured wall stays in view above the number pad.
                focus =
                    state.draft
                        ?.takeIf { state.measure != null }
                        ?.let { listOfNotNull(it.start, it.end) }
                        .orEmpty(),
                onTap = actions.onTap,
                onAim = actions.onAim,
                onAimCancel = actions.onAimCancel,
                onCommit = actions.onCommit,
                modifier = Modifier.fillMaxSize(),
            )
            Column(
                modifier = Modifier.align(Alignment.TopCenter).fillMaxWidth().padding(OtvesDimens.gutter),
                horizontalAlignment = Alignment.CenterHorizontally,
                verticalArrangement = Arrangement.spacedBy(8.dp),
            ) {
                Row(modifier = Modifier.fillMaxWidth(), horizontalArrangement = Arrangement.spacedBy(8.dp)) {
                    RoundButton(OtvesIcons.Back, stringResource(R.string.editor_back), actions.onBack)
                    if (state.ready) LevelPill(state.levels, state.level, actions.onLevel, actions.onAddLevel)
                    Spacer(Modifier.weight(1f))
                    RoundButton(OtvesIcons.Fit, stringResource(R.string.editor_fit), { fit++ })
                    RoundButton(OtvesIcons.List, stringResource(R.string.takeoff_title), { actions.onTakeoff(true) })
                }
                Notice(state)
            }
            val selected = state.selected
            val entry = state.entry
            val measure = state.measure
            val sizeEntry = state.sizeEntry
            val priceEntry = state.priceEntry
            Box(modifier = Modifier.align(Alignment.BottomCenter)) {
                when {
                    entry != null -> LengthPad(entry, actions)
                    sizeEntry != null -> SizePad(sizeEntry, actions)
                    priceEntry != null -> PricePad(priceEntry, state, actions)
                    measure != null -> MeasurePad(measure, state, actions)
                    state.materialOpen -> MaterialPanel(state, actions)
                    selected != null -> Inspector(selected, state, actions)
                    state.takeoffOpen && state.conditionsOpen -> ConditionsPanel(state, actions)
                    state.takeoffOpen -> TakeoffPanel(state, actions)
                    else -> Tools(state, actions)
                }
            }
        }
    }
}

// One line that says what the next touch does, or what is wrong.
@Composable
private fun Notice(state: EditorUiState) {
    // Where walls meet wrongly; what a storey lacks by the norms is said beside its choices.
    val joinIssue = state.issues.firstOrNull { it.at != null }
    val drawing = state.tool == Tool.WALL && state.draft != null
    val text =
        when {
            !state.ready -> stringResource(R.string.editor_loading)

            state.refused != null -> refusalText(state.refused)

            // The number pad says what is being entered.
            state.measure != null -> null

            drawing -> stringResource(R.string.editor_hint_next)

            joinIssue != null -> issueText(joinIssue.kind)

            state.tool == Tool.SELECT && state.selected == null -> stringResource(R.string.editor_hint_select)

            state.tool == Tool.WINDOW -> stringResource(R.string.editor_hint_window)

            state.tool == Tool.DOOR -> stringResource(R.string.editor_hint_door)

            state.outlines.isEmpty() -> stringResource(R.string.editor_hint_start)

            else -> null
        } ?: return
    val warns = state.refused != null || (!drawing && joinIssue != null)
    val colors = OtvesTheme.colors
    Surface(shape = PanelShape, color = colors.surface, border = BorderStroke(1.dp, colors.border)) {
        Text(
            text = text,
            style = MaterialTheme.typography.bodyMedium,
            color = if (warns) colors.warning else colors.inkSecondary,
            modifier = Modifier.padding(horizontal = 12.dp, vertical = 6.dp),
        )
    }
}

// The storey the user works on; the list offers the others, the top one first, and a new one.
@Composable
private fun LevelPill(
    levels: List<ElementId>,
    current: ElementId?,
    onLevel: (ElementId) -> Unit,
    onAdd: () -> Unit,
) {
    var open by remember { mutableStateOf(false) }
    val colors = OtvesTheme.colors
    val choose = stringResource(R.string.level_choose)
    Box {
        Surface(
            onClick = { open = true },
            shape = CircleShape,
            color = colors.surface,
            border = BorderStroke(1.dp, colors.border),
            shadowElevation = FloatElevation,
            modifier = Modifier.height(OtvesDimens.minTarget).semantics { stateDescription = choose },
        ) {
            Row(
                modifier = Modifier.padding(start = 16.dp, end = 12.dp),
                verticalAlignment = Alignment.CenterVertically,
                horizontalArrangement = Arrangement.spacedBy(6.dp),
            ) {
                Text(stringResource(R.string.level_name, levels.indexOf(current) + 1), style = MaterialTheme.typography.titleMedium)
                Icon(OtvesIcons.ChevronDown, contentDescription = null, tint = colors.inkSecondary, modifier = Modifier.size(18.dp))
            }
        }
        DropdownMenu(expanded = open, onDismissRequest = { open = false }, containerColor = colors.surface) {
            levels.asReversed().forEach { level ->
                DropdownMenuItem(
                    text = {
                        Text(
                            stringResource(R.string.level_name, levels.indexOf(level) + 1),
                            color = if (level == current) colors.ink else colors.inkSecondary,
                            style = MaterialTheme.typography.titleMedium,
                        )
                    },
                    onClick = {
                        open = false
                        onLevel(level)
                    },
                )
            }
            HorizontalDivider(color = colors.divider)
            DropdownMenuItem(
                text = { Text(stringResource(R.string.level_add)) },
                leadingIcon = { Icon(OtvesIcons.Plus, contentDescription = null) },
                onClick = {
                    open = false
                    onAdd()
                },
            )
        }
    }
}

// The chosen tool's options over undo, redo and the tools themselves.
@Composable
private fun Tools(
    state: EditorUiState,
    actions: EditorActions,
) {
    Column(modifier = Modifier.padding(bottom = OtvesDimens.gutter), verticalArrangement = Arrangement.spacedBy(12.dp)) {
        ToolOptions(state, actions)
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

// Every tool stays visible; the chosen one is named.
@Composable
private fun ToolPill(
    current: Tool,
    onTool: (Tool) -> Unit,
) {
    val colors = OtvesTheme.colors
    Surface(shape = CircleShape, color = colors.ink, contentColor = colors.surface, shadowElevation = 8.dp) {
        Row(
            modifier = Modifier.height(OtvesDimens.primaryTarget).padding(horizontal = 6.dp).selectableGroup(),
            verticalAlignment = Alignment.CenterVertically,
            horizontalArrangement = Arrangement.spacedBy(2.dp),
        ) {
            Tool.entries.forEach { tool ->
                val chosen = tool == current
                val name = toolName(tool)
                Surface(
                    onClick = { onTool(tool) },
                    shape = CircleShape,
                    color = if (chosen) colors.surface else colors.ink,
                    contentColor = if (chosen) colors.ink else colors.surface,
                    modifier =
                        Modifier.height(44.dp).semantics {
                            role = Role.Tab
                            selected = chosen
                            contentDescription = name
                        },
                ) {
                    Row(
                        modifier = Modifier.padding(horizontal = 11.dp),
                        verticalAlignment = Alignment.CenterVertically,
                        horizontalArrangement = Arrangement.spacedBy(6.dp),
                    ) {
                        Icon(toolIcon(tool), contentDescription = null, modifier = Modifier.size(22.dp))
                        if (chosen) Text(name, style = MaterialTheme.typography.labelLarge, maxLines = 1)
                    }
                }
            }
        }
    }
}

@Composable
private fun ToolOptions(
    state: EditorUiState,
    actions: EditorActions,
) {
    // Wraps, so no command hides past the edge of the screen.
    FlowRow(
        modifier = Modifier.fillMaxWidth().padding(horizontal = OtvesDimens.gutter),
        horizontalArrangement = Arrangement.spacedBy(8.dp),
        verticalArrangement = Arrangement.spacedBy(8.dp),
    ) {
        when (state.tool) {
            Tool.SELECT -> {
                // Nothing to choose before something is picked.
            }

            Tool.WALL -> {
                val material = state.material
                Pill(
                    label = if (material == null) stringResource(R.string.material_choose) else unitTitle(material, state.catalog.units),
                    selected = false,
                    onClick = { actions.onMaterials(true) },
                    trailing = OtvesIcons.ChevronDown,
                )
                ThicknessPill(state.thickness, state.thicknessesOf(material), actions.onThickness)
                val lock = stringResource(if (state.ortho) R.string.draw_ortho_on else R.string.draw_ortho_off)
                Pill(stringResource(R.string.draw_ortho), state.ortho, actions.onOrtho, description = lock)
                // The wall just drawn: its length is typed here, as read off the tape.
                state.dimensions.find { it.id == state.drawn }?.let { drawn ->
                    Pill(stringResource(R.string.length_value, grouped(drawn.length)), false, { actions.onEditLength(drawn.id) })
                }
                if (state.draft != null) {
                    Pill(stringResource(R.string.measure_start), false, actions.onMeasure)
                    Pill(stringResource(R.string.inspector_done), false, actions.onFinishWall)
                }
            }

            Tool.WINDOW -> {
                WindowSizes.forEach { size -> Pill(sizeName(size), size == state.windowSize, { actions.onWindowSize(size) }) }
            }

            Tool.DOOR -> {
                DoorSizes.forEach { size -> Pill(sizeName(size), size == state.doorSize, { actions.onDoorSize(size) }) }
            }
        }
    }
}

@Composable
private fun ThicknessPill(
    current: Mm,
    thicknesses: List<Mm>,
    onThickness: (Mm) -> Unit,
) {
    var open by remember { mutableStateOf(false) }
    Box {
        Pill(stringResource(R.string.length_value, grouped(current)), false, { open = true }, trailing = OtvesIcons.ChevronDown)
        DropdownMenu(expanded = open, onDismissRequest = { open = false }, containerColor = OtvesTheme.colors.surface) {
            thicknesses.forEach { thickness ->
                DropdownMenuItem(
                    text = { Text(stringResource(R.string.length_value, grouped(thickness))) },
                    onClick = {
                        open = false
                        onThickness(thickness)
                    },
                )
            }
        }
    }
}

// A floating command or choice; the chosen one is inked.
@Composable
private fun Pill(
    label: String,
    selected: Boolean,
    onClick: () -> Unit,
    modifier: Modifier = Modifier,
    description: String? = null,
    trailing: ImageVector? = null,
) {
    val colors = OtvesTheme.colors
    Surface(
        onClick = onClick,
        shape = CircleShape,
        color = if (selected) colors.ink else colors.surface,
        contentColor = if (selected) colors.surface else colors.ink,
        border = if (selected) null else BorderStroke(1.dp, colors.border),
        shadowElevation = FloatElevation,
        modifier =
            modifier.height(OtvesDimens.minTarget).semantics {
                this.selected = selected
                if (description != null) contentDescription = description
            },
    ) {
        Row(
            modifier = Modifier.padding(horizontal = 16.dp),
            verticalAlignment = Alignment.CenterVertically,
            horizontalArrangement = Arrangement.spacedBy(6.dp),
        ) {
            Text(label, style = MaterialTheme.typography.labelLarge, maxLines = 1)
            if (trailing != null) Icon(trailing, contentDescription = null, modifier = Modifier.size(16.dp))
        }
    }
}

@Composable
private fun RoundButton(
    icon: ImageVector,
    description: String,
    onClick: () -> Unit,
    enabled: Boolean = true,
) {
    val colors = OtvesTheme.colors
    Surface(
        onClick = onClick,
        enabled = enabled,
        shape = CircleShape,
        color = colors.surface,
        contentColor = if (enabled) colors.ink else colors.inkTertiary,
        border = BorderStroke(1.dp, colors.border),
        shadowElevation = FloatElevation,
        modifier = Modifier.size(OtvesDimens.minTarget).semantics { contentDescription = description },
    ) {
        Box(contentAlignment = Alignment.Center) {
            Icon(icon, contentDescription = null, modifier = Modifier.size(22.dp))
        }
    }
}

@Composable
private fun Panel(content: @Composable () -> Unit) {
    val colors = OtvesTheme.colors
    Surface(
        color = colors.surface,
        shape = RoundedCornerShape(topStart = 16.dp, topEnd = 16.dp),
        border = BorderStroke(1.dp, colors.border),
        shadowElevation = 12.dp,
        modifier = Modifier.fillMaxWidth(),
    ) {
        Column(modifier = Modifier.padding(horizontal = OtvesDimens.gutter, vertical = 8.dp)) { content() }
    }
}

@Composable
private fun PanelHeader(
    title: String,
    actions: @Composable () -> Unit,
) {
    Row(verticalAlignment = Alignment.CenterVertically) {
        Text(title, style = MaterialTheme.typography.titleMedium, modifier = Modifier.weight(1f))
        actions()
    }
}

@Composable
private fun PanelAction(
    label: String,
    onClick: () -> Unit,
    enabled: Boolean = true,
) {
    TextButton(onClick = onClick, enabled = enabled, modifier = Modifier.heightIn(min = OtvesDimens.minTarget)) { Text(label) }
}

// A wall's exact length in millimetres, typed on large keys as it is read off the tape.
@Composable
private fun LengthPad(
    entry: LengthEntry,
    actions: EditorActions,
) {
    Panel {
        PanelHeader(stringResource(R.string.length_title)) {
            PanelAction(stringResource(R.string.length_cancel), actions.onCancelLength)
        }
        TypedLength(entry.digits) {
            Text(
                stringResource(R.string.length_was, grouped(entry.was)),
                style = MaterialTheme.typography.bodyLarge,
                color = OtvesTheme.colors.inkSecondary,
            )
        }
        NumberKeys(actions, stringResource(R.string.inspector_done), actions.onConfirmLength, enabled = entry.digits.isNotEmpty())
    }
}

// Walls entered one after another from readings: the length, then which way it goes from the wall
// before. The plan shows the wall before it is built.
@Composable
private fun MeasurePad(
    measure: Measure,
    state: EditorUiState,
    actions: EditorActions,
) {
    Panel {
        PanelHeader(stringResource(R.string.measure_title)) {
            PanelAction(stringResource(R.string.editor_undo), actions.onUndo, enabled = state.canUndo)
            PanelAction(stringResource(R.string.takeoff_close), actions.onStopMeasure)
        }
        TypedLength(measure.digits) {
            if (state.canCloseRun) Pill(stringResource(R.string.measure_close), false, actions.onCloseMeasured)
        }
        SegmentedControl(
            options = Turn.entries.map { turnName(it) },
            selected = measure.turn.ordinal,
            onSelect = { actions.onTurn(Turn.entries[it]) },
            modifier = Modifier.fillMaxWidth().padding(top = 4.dp),
        )
        NumberKeys(actions, stringResource(R.string.measure_next), actions.onNextMeasured, enabled = state.draft?.end != null)
    }
}

@Composable
private fun TypedLength(
    digits: String,
    trailing: @Composable () -> Unit,
) {
    val colors = OtvesTheme.colors
    Row(modifier = Modifier.fillMaxWidth().padding(vertical = 4.dp), verticalAlignment = Alignment.CenterVertically) {
        Text(
            text = stringResource(R.string.length_value, digits.toLongOrNull()?.let(::grouped) ?: "0"),
            style = MaterialTheme.typography.displaySmall,
            color = if (digits.isEmpty()) colors.inkTertiary else colors.ink,
            modifier = Modifier.weight(1f),
        )
        trailing()
    }
}

@Composable
private fun NumberKeys(
    actions: EditorActions,
    confirm: String,
    onConfirm: () -> Unit,
    enabled: Boolean,
) {
    Column(modifier = Modifier.padding(top = 8.dp, bottom = 8.dp), verticalArrangement = Arrangement.spacedBy(8.dp)) {
        for (row in PadRows) {
            Row(horizontalArrangement = Arrangement.spacedBy(8.dp)) {
                row.forEach { digit -> PadKey(Modifier.weight(1f), onClick = { actions.onDigit(digit) }) { Text(digit.toString()) } }
            }
        }
        Row(horizontalArrangement = Arrangement.spacedBy(8.dp)) {
            val erase = stringResource(R.string.length_erase)
            PadKey(Modifier.weight(1f).semantics { contentDescription = erase }, onClick = actions.onErase) {
                Icon(OtvesIcons.Backspace, contentDescription = null)
            }
            PadKey(Modifier.weight(1f), onClick = { actions.onDigit(0) }) { Text("0") }
            PadKey(Modifier.weight(1f), onClick = onConfirm, primary = true, enabled = enabled) {
                Text(confirm, style = MaterialTheme.typography.titleMedium)
            }
        }
    }
}

@Composable
private fun PadKey(
    modifier: Modifier,
    onClick: () -> Unit,
    primary: Boolean = false,
    enabled: Boolean = true,
    content: @Composable () -> Unit,
) {
    val colors = OtvesTheme.colors
    Surface(
        onClick = onClick,
        enabled = enabled,
        shape = RoundedCornerShape(10.dp),
        color = if (primary && enabled) colors.ink else colors.field,
        contentColor =
            when {
                !enabled -> colors.inkTertiary
                primary -> colors.surface
                else -> colors.ink
            },
        modifier = modifier.height(OtvesDimens.primaryTarget),
    ) {
        Box(contentAlignment = Alignment.Center) {
            ProvideTextStyle(MaterialTheme.typography.headlineSmall, content)
        }
    }
}

// What the select tool picked, with its settings.
@Composable
private fun Inspector(
    selected: Selected,
    state: EditorUiState,
    actions: EditorActions,
) {
    val issues = state.issues
    Panel {
        val title =
            when (selected) {
                is Selected.WallPart -> stringResource(R.string.tool_wall)
                is Selected.OpeningPart -> kindName(selected.opening.kind)
            }
        PanelHeader(title) {
            PanelAction(stringResource(R.string.inspector_delete), actions.onDelete)
            PanelAction(stringResource(R.string.inspector_done), actions.onDeselect)
        }
        when (selected) {
            is Selected.WallPart -> {
                val unit = selected.wall.unit
                InspectorRow(stringResource(R.string.inspector_material)) {
                    Pill(
                        label = if (unit == null) stringResource(R.string.material_choose) else unitTitle(unit, state.catalog.units),
                        selected = false,
                        onClick = { actions.onMaterials(true) },
                        trailing = OtvesIcons.ChevronDown,
                    )
                }
                state.dimensions.find { it.id == selected.id }?.let { dimension ->
                    InspectorRow(stringResource(R.string.length_title)) {
                        Pill(stringResource(R.string.length_value, grouped(dimension.length)), false, { actions.onEditLength(selected.id) })
                    }
                }
                InspectorRow(stringResource(R.string.inspector_thickness)) {
                    state.thicknessesOf(unit).forEach { thickness ->
                        Pill(grouped(thickness), thickness == selected.wall.thickness, { actions.onWallThickness(thickness) })
                    }
                }
            }

            is Selected.OpeningPart -> {
                val opening = selected.opening
                issues
                    .filter { it.element == selected.id }
                    .map { it.kind }
                    .distinct()
                    .forEach { kind ->
                        Text(issueText(kind), style = MaterialTheme.typography.bodyMedium, color = OtvesTheme.colors.warning)
                    }
                SegmentedControl(
                    options = OpeningKind.entries.map { kindName(it) },
                    selected = opening.kind.ordinal,
                    onSelect = { actions.onOpeningKind(OpeningKind.entries[it]) },
                    modifier = Modifier.fillMaxWidth(),
                )
                InspectorRow(stringResource(R.string.inspector_size)) {
                    val current = OpeningSize(opening.width, opening.height)
                    sizesFor(opening.kind).forEach { size ->
                        Pill(sizeName(size), size == current, { actions.onOpeningSize(size) })
                    }
                }
            }
        }
        Spacer(Modifier.height(8.dp))
    }
}

@Composable
private fun InspectorRow(
    label: String,
    content: @Composable () -> Unit,
) {
    Row(modifier = Modifier.fillMaxWidth().padding(top = 8.dp), verticalAlignment = Alignment.CenterVertically) {
        Text(
            label,
            style = MaterialTheme.typography.bodyLarge,
            color = OtvesTheme.colors.inkSecondary,
            modifier = Modifier.width(InspectorLabelWidth),
        )
        Row(
            modifier = Modifier.horizontalScroll(rememberScrollState()).padding(vertical = 4.dp),
            horizontalArrangement = Arrangement.spacedBy(8.dp),
        ) {
            content()
        }
    }
}

// What walls are laid from: the catalog's units by name and size as laid. The choice goes to the
// selected wall, or else to the walls drawn next.
@Composable
private fun MaterialPanel(
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

// A brick or block the catalog does not list: its three sizes as laid, typed one after another.
@Composable
private fun SizePad(
    entry: SizeEntry,
    actions: EditorActions,
) {
    val colors = OtvesTheme.colors
    val labels =
        if (entry.footing) {
            listOf(R.string.size_width, R.string.size_depth)
        } else {
            listOf(R.string.size_length, R.string.size_width, R.string.size_height)
        }
    Panel {
        PanelHeader(stringResource(if (entry.footing) R.string.level_footing else R.string.material_own)) {
            PanelAction(stringResource(R.string.length_cancel), actions.onCancelSize)
        }
        Row(modifier = Modifier.fillMaxWidth().selectableGroup(), horizontalArrangement = Arrangement.spacedBy(8.dp)) {
            labels.forEachIndexed { index, label ->
                val active = index == entry.active
                Surface(
                    onClick = { actions.onSizeField(index) },
                    shape = PanelShape,
                    // No boxes: the size being typed sits on the field colour, the others on the panel.
                    color = if (active) colors.field else colors.surface,
                    modifier = Modifier.weight(1f).semantics { selected = active },
                ) {
                    Column(modifier = Modifier.padding(horizontal = 12.dp, vertical = 8.dp)) {
                        Text(stringResource(label), style = MaterialTheme.typography.bodyMedium, color = colors.inkSecondary)
                        Text(
                            text = entry.digits[index].ifEmpty { "0" },
                            style = MaterialTheme.typography.headlineSmall,
                            color = if (entry.digits[index].isEmpty()) colors.inkTertiary else colors.ink,
                        )
                    }
                }
            }
        }
        NumberKeys(
            actions,
            stringResource(R.string.inspector_done),
            actions.onConfirmSize,
            enabled = entry.size != null || entry.section != null,
        )
    }
}

// What the job is estimated under. An unset condition shows the norm's value as chosen.
@Composable
private fun ConditionsPanel(
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
private fun StoreyChoices(
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

// The materials the model asks for, and what the storey's walls are reinforced with: the lines
// follow these choices at once.
@Composable
private fun TakeoffPanel(
    state: EditorUiState,
    actions: EditorActions,
) {
    val onClose = { actions.onTakeoff(false) }
    Panel {
        val lines = state.takeoff.filter { it.amount != 0.0 }
        val context = LocalContext.current
        val scope = rememberCoroutineScope()
        val shared = takeoffText(state.name, lines, state.catalog.units, state.prices)
        PanelHeader(stringResource(R.string.takeoff_title)) {
            ShareMenu(
                enabled = lines.isNotEmpty() && !state.takeoffFailed,
                onText = { context.share(shared) },
                onPdf = { scope.launch { context.shareEstimate(state) } },
            )
            PanelAction(stringResource(R.string.takeoff_close), onClose)
        }
        when {
            state.takeoffFailed -> {
                Text(stringResource(R.string.takeoff_failed), color = OtvesTheme.colors.warning)
            }

            state.outlines.isEmpty() -> {
                Text(stringResource(R.string.takeoff_empty), color = OtvesTheme.colors.inkSecondary)
            }

            else -> {
                // The storey's choices scroll with the lines they add; the total stays in sight.
                val bought = bought(lines)
                LazyColumn(modifier = Modifier.heightIn(max = TakeoffHeight)) {
                    item { StoreyChoices(state, actions) }
                    items(lines) { line -> if (line in bought) TakeoffLine(line, state, actions.onPrice) else MixLine(line, state) }
                }
                Total(lines, state.prices)
                PanelAction(stringResource(R.string.conditions_title), { actions.onConditions(true) })
            }
        }
        Spacer(Modifier.height(8.dp))
    }
}

// Whole millimetres with a no-break space between thousands, as on drawings: «6 400».
private fun grouped(value: Mm): String = grouped(value.value)

private fun grouped(value: Long): String = String.format(Locale.ROOT, "%,d", value).replace(',', '\u00A0')

// The take-off as plain text for a messenger: the job's name, then one material a line.
@Composable
private fun takeoffText(
    name: String,
    lines: List<QuantityLine>,
    units: List<MasonryUnitOption>,
    prices: Map<String, Money>,
): String {
    val title = stringResource(R.string.takeoff_share_title, name.ifBlank { stringResource(R.string.editor_title) })
    val bought = bought(lines)
    val rows =
        lines.map { line ->
            val amount = stringResource(R.string.takeoff_share_line, itemName(line.item, units), amountText(line), unitName(line.unit))
            cost(line, prices)?.takeIf { line in bought }?.let { stringResource(R.string.takeoff_share_cost, amount, moneyText(it)) }
                ?: amount
        }
    val sum = total(lines, prices)?.let { listOf("", stringResource(R.string.takeoff_share_total, moneyText(it))) }.orEmpty()
    return (listOf(title, "") + rows + sum).joinToString("\n")
}

// The materials go out as plain text for a chat, or as a sheet with the plan and the costs.
@Composable
private fun ShareMenu(
    enabled: Boolean,
    onText: () -> Unit,
    onPdf: () -> Unit,
) {
    var open by remember { mutableStateOf(false) }
    Box {
        PanelAction(stringResource(R.string.takeoff_share), { open = true }, enabled = enabled)
        DropdownMenu(expanded = open, onDismissRequest = { open = false }, containerColor = OtvesTheme.colors.surface) {
            DropdownMenuItem(
                text = { Text(stringResource(R.string.share_text)) },
                onClick = {
                    open = false
                    onText()
                },
            )
            DropdownMenuItem(
                text = { Text(stringResource(R.string.share_pdf)) },
                onClick = {
                    open = false
                    onPdf()
                },
            )
        }
    }
}

// One material: how much of it, and under that what one unit costs him and what the line comes to.
// A touch opens the price.
@Composable
private fun TakeoffLine(
    line: QuantityLine,
    state: EditorUiState,
    onPrice: (QuantityLine) -> Unit,
) {
    val colors = OtvesTheme.colors
    val price = state.prices[line.item]
    Surface(onClick = { onPrice(line) }, color = colors.surface) {
        Column {
            Row(modifier = Modifier.fillMaxWidth().padding(top = 6.dp)) {
                Text(itemName(line.item, state.catalog.units), modifier = Modifier.weight(1f))
                Text("${amountText(line)} ${unitName(line.unit)}")
            }
            Row(modifier = Modifier.fillMaxWidth().padding(bottom = 6.dp)) {
                Text(
                    text =
                        if (price == null) {
                            stringResource(R.string.price_set)
                        } else {
                            stringResource(R.string.price_per, moneyText(price), unitName(line.unit))
                        },
                    style = MaterialTheme.typography.bodyMedium,
                    color = if (price == null) colors.inkTertiary else colors.inkSecondary,
                    modifier = Modifier.weight(1f),
                )
                cost(line, state.prices)?.let { Text(moneyText(it), style = MaterialTheme.typography.bodyMedium) }
            }
            HorizontalDivider(color = colors.divider)
        }
    }
}

// A material mixed on site from the lines after it: how much of it, for the mixer; it has no price
// of its own.
@Composable
private fun MixLine(
    line: QuantityLine,
    state: EditorUiState,
) {
    val colors = OtvesTheme.colors
    Row(modifier = Modifier.fillMaxWidth().padding(vertical = 6.dp)) {
        Text(itemName(line.item, state.catalog.units), color = colors.inkSecondary, modifier = Modifier.weight(1f))
        Text("${amountText(line)} ${unitName(line.unit)}", color = colors.inkSecondary)
    }
    HorizontalDivider(color = colors.divider)
}

// What the priced materials come to, and how many still have no price.
@Composable
private fun Total(
    lines: List<QuantityLine>,
    prices: Map<String, Money>,
) {
    val sum = total(lines, prices) ?: return
    val unpriced = bought(lines).count { it.item !in prices }
    Row(modifier = Modifier.fillMaxWidth().padding(top = 10.dp), verticalAlignment = Alignment.CenterVertically) {
        Column(modifier = Modifier.weight(1f)) {
            Text(stringResource(R.string.takeoff_total), style = MaterialTheme.typography.titleMedium)
            if (unpriced > 0) {
                Text(
                    stringResource(R.string.takeoff_unpriced, unpriced),
                    style = MaterialTheme.typography.bodyMedium,
                    color = OtvesTheme.colors.inkSecondary,
                )
            }
        }
        Text(moneyText(sum), style = MaterialTheme.typography.titleMedium)
    }
}

// What the builder pays for one unit of a material, in whole units of the currency. It goes to
// his price list, so the next job already knows it; an empty price forgets it.
@Composable
private fun PricePad(
    entry: PriceEntry,
    state: EditorUiState,
    actions: EditorActions,
) {
    val colors = OtvesTheme.colors
    Panel {
        PanelHeader(itemName(entry.line.item, state.catalog.units)) {
            PanelAction(stringResource(R.string.length_cancel), actions.onCancelPrice)
        }
        Row(modifier = Modifier.fillMaxWidth().padding(vertical = 4.dp), verticalAlignment = Alignment.Bottom) {
            Text(
                text = stringResource(R.string.money_uzs, entry.digits.toLongOrNull()?.let(::grouped) ?: "0"),
                style = MaterialTheme.typography.displaySmall,
                color = if (entry.digits.isEmpty()) colors.inkTertiary else colors.ink,
                modifier = Modifier.weight(1f),
            )
            Text(
                stringResource(R.string.price_for_one, unitName(entry.line.unit)),
                style = MaterialTheme.typography.bodyLarge,
                color = colors.inkSecondary,
            )
        }
        NumberKeys(actions, stringResource(R.string.inspector_done), actions.onConfirmPrice, enabled = true)
    }
}

private fun Context.share(text: String) {
    val send =
        Intent(Intent.ACTION_SEND)
            .setType("text/plain")
            .putExtra(Intent.EXTRA_TEXT, text)
    startActivity(Intent.createChooser(send, null))
}

@Composable
private fun issueText(kind: IssueKind): String =
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
private fun kindName(kind: OpeningKind): String =
    when (kind) {
        OpeningKind.WINDOW -> stringResource(R.string.tool_window)
        OpeningKind.DOOR -> stringResource(R.string.tool_door)
        OpeningKind.EMPTY -> stringResource(R.string.kind_empty)
    }

private fun toolIcon(tool: Tool): ImageVector =
    when (tool) {
        Tool.SELECT -> OtvesIcons.Select
        Tool.WALL -> OtvesIcons.Wall
        Tool.WINDOW -> OtvesIcons.Window
        Tool.DOOR -> OtvesIcons.Door
    }

@Composable
private fun turnName(turn: Turn): String =
    stringResource(
        when (turn) {
            Turn.LEFT -> R.string.turn_left
            Turn.STRAIGHT -> R.string.turn_straight
            Turn.RIGHT -> R.string.turn_right
        },
    )

@Composable
private fun toolName(tool: Tool): String =
    when (tool) {
        Tool.SELECT -> stringResource(R.string.tool_select)
        Tool.WALL -> stringResource(R.string.tool_wall)
        Tool.WINDOW -> stringResource(R.string.tool_window)
        Tool.DOOR -> stringResource(R.string.tool_door)
    }

// Millimetres, as on drawings: «1500 × 1500».
@Composable
private fun sizeName(size: OpeningSize): String = stringResource(R.string.opening_size, size.width.value, size.height.value)

@Composable
private fun refusalText(error: EngineError): String =
    when (error) {
        EngineError.ZERO_LENGTH -> stringResource(R.string.refused_zero_length)
        EngineError.OFF_PLAN -> stringResource(R.string.refused_off_plan)
        EngineError.NO_WALL -> stringResource(R.string.refused_no_wall)
        EngineError.DOES_NOT_FIT -> stringResource(R.string.refused_does_not_fit)
        else -> stringResource(R.string.refused_other)
    }

@Preview(widthDp = 400, heightDp = 760)
@Composable
private fun EditorScreenPreview() {
    val level = ElementId(Uuid.parse("019a5f00-0000-7000-8000-000000000001"))
    val wall = ElementId(Uuid.parse("019a5f00-0000-7000-8000-000000000002"))
    val window = ElementId(Uuid.parse("019a5f00-0000-7000-8000-000000000003"))
    val drawn = Wall(level, PointMm(0.mm, 0.mm), PointMm(6_400.mm, 0.mm), 380.mm, 2_800.mm)
    OtvesTheme {
        EditorScreen(
            state =
                EditorUiState(
                    ready = true,
                    levels = listOf(level),
                    level = level,
                    outlines =
                        listOf(
                            WallOutline(
                                wall,
                                level,
                                listOf(PointM(-0.19, -0.19), PointM(6.59, -0.19), PointM(6.21, 0.19), PointM(0.19, 0.19)),
                                length = 6_400.mm,
                            ),
                        ),
                    dimensions = listOf(Dimension(wall, drawn, 6_400.mm)),
                    openings =
                        listOf(
                            OpeningOutline(
                                window,
                                level,
                                OpeningKind.WINDOW,
                                listOf(PointM(2.25, -0.19), PointM(3.75, -0.19), PointM(3.75, 0.19), PointM(2.25, 0.19)),
                            ),
                        ),
                    draft = Draft(PointMm(6_400.mm, 0.mm), PointMm(6_400.mm, 4_200.mm)),
                    drawn = wall,
                    canUndo = true,
                ),
            actions = EditorActions(),
        )
    }
}

@Preview(widthDp = 400, heightDp = 760)
@Composable
private fun LengthPadPreview() {
    val wall = ElementId(Uuid.parse("019a5f00-0000-7000-8000-000000000002"))
    OtvesTheme {
        EditorScreen(
            state = EditorUiState(ready = true, entry = LengthEntry(wall, was = 6_437.mm, digits = "6400")),
            actions = EditorActions(),
        )
    }
}

private const val PER_MILLE_IN_PERCENT = 10
private val TakeoffHeight = 440.dp
private val InspectorLabelWidth = 124.dp
private val FloatElevation = 2.dp
private val PanelShape = RoundedCornerShape(10.dp)
private val PadRows = listOf(listOf(7, 8, 9), listOf(4, 5, 6), listOf(1, 2, 3))
