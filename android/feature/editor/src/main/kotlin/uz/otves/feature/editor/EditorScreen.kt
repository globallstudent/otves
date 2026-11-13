package uz.otves.feature.editor

import androidx.compose.foundation.BorderStroke
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.safeDrawingPadding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.shape.CircleShape
import androidx.compose.material3.DropdownMenu
import androidx.compose.material3.DropdownMenuItem
import androidx.compose.material3.HorizontalDivider
import androidx.compose.material3.Icon
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Surface
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableIntStateOf
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.res.stringResource
import androidx.compose.ui.semantics.contentDescription
import androidx.compose.ui.semantics.selected
import androidx.compose.ui.semantics.semantics
import androidx.compose.ui.semantics.stateDescription
import androidx.compose.ui.tooling.preview.Preview
import androidx.compose.ui.unit.dp
import androidx.lifecycle.compose.collectAsStateWithLifecycle
import androidx.lifecycle.viewmodel.compose.viewModel
import uz.otves.core.designsystem.OtvesDimens
import uz.otves.core.designsystem.OtvesIcons
import uz.otves.core.designsystem.OtvesTheme
import uz.otves.core.model.ElementId
import uz.otves.core.model.Mm
import uz.otves.core.model.mm
import uz.otves.data.catalog.Prices
import uz.otves.data.project.JobStore
import uz.otves.engine.Aimed
import uz.otves.engine.Catalog
import uz.otves.engine.Condition
import uz.otves.engine.Drawing
import uz.otves.engine.DrawnLine
import uz.otves.engine.DrawnPolygon
import uz.otves.engine.DrawnText
import uz.otves.engine.Engine
import uz.otves.engine.EngineCommand
import uz.otves.engine.EngineError
import uz.otves.engine.FillKind
import uz.otves.engine.MasonryUnitOption
import uz.otves.engine.OpeningKind
import uz.otves.engine.PointM
import uz.otves.engine.PointMm
import uz.otves.engine.QuantityLine
import uz.otves.engine.StrokeKind
import uz.otves.engine.TextKind
import uz.otves.engine.Turn
import uz.otves.engine.Wall
import uz.otves.engine.WallOutline
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
                onSketch = viewModel::sketch,
                onRefine = viewModel::refine,
                onLabelSize = viewModel::setLabelSize,
                onSheets = viewModel::sheets,
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
    val onSketch: (points: List<PointMm>, reach: Mm) -> Unit = { _, _ -> },
    val onRefine: () -> Unit = {},
    val onLabelSize: (LabelSize) -> Unit = {},
    /** Every storey's drawing with label text of the given sizes, for a sheet. */
    val onSheets: suspend (List<LabelSize?>) -> List<Drawing?> = { emptyList() },
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
    // Counts requests to show the whole plan: the button, a job opening, the end of a measured run
    // or of a pass over sketched walls, which both move the plan to keep a wall above the keys.
    var fit by remember { mutableIntStateOf(0) }
    val measuring = state.measure != null
    val refining = state.entry?.refining == true
    LaunchedEffect(measuring, refining, state.ready) { if (!measuring && !refining && state.ready) fit++ }
    val listOpen = state.takeoffOpen || state.materialOpen
    Surface(modifier = modifier.fillMaxSize(), color = OtvesTheme.colors.canvas) {
        Box(modifier = Modifier.safeDrawingPadding()) {
            // The wall whose length is typed is marked as a selected one is.
            val sized = state.dimensions.find { it.id == state.entry?.wall }
            PlanCanvas(
                plan = state.drawing,
                outlines = state.outlines,
                selected = sized?.id ?: state.selected?.id,
                issues = state.issues,
                draft = state.draft,
                draftThickness = state.thickness,
                drawing = state.tool == Tool.WALL && !state.typing && !listOpen,
                sketching = state.tool == Tool.PENCIL && !state.typing && !listOpen,
                fit = fit,
                // The wall being measured or sized stays in view above the number pad.
                focus =
                    sized?.let { listOf(it.wall.start, it.wall.end) }
                        ?: state.draft
                            ?.takeIf { state.measure != null }
                            ?.let { listOfNotNull(it.start, it.to?.point) }
                            .orEmpty(),
                onTap = actions.onTap,
                onAim = actions.onAim,
                onAimCancel = actions.onAimCancel,
                onCommit = actions.onCommit,
                onSketch = actions.onSketch,
                onLabelSize = actions.onLabelSize,
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
            val idle = entry == null && sizeEntry == null && priceEntry == null && measure == null
            val bare = selected == null && !state.materialOpen && !state.takeoffOpen
            if (state.ready && idle && bare && hasOptions(state.tool)) ToolOptionsButton(state, actions)
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

            // A stroke too short to be a wall is not a wall whose ends coincide.
            state.refused == EngineError.ZERO_LENGTH && state.tool == Tool.PENCIL -> stringResource(R.string.sketch_too_short)

            state.refused != null -> refusalText(state.refused)

            // The number pad says what is being entered.
            state.measure != null -> null

            drawing -> stringResource(R.string.editor_hint_next)

            joinIssue != null -> issueText(joinIssue.kind)

            state.tool == Tool.SELECT && state.selected == null -> stringResource(R.string.editor_hint_select)

            state.tool == Tool.PENCIL && state.rough.isNotEmpty() -> stringResource(R.string.sketch_rough)

            state.tool == Tool.PENCIL -> stringResource(R.string.editor_hint_pencil)

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

@Preview(widthDp = 400, heightDp = 760)
@Composable
private fun EditorScreenPreview() {
    val level = ElementId(Uuid.parse("019a5f00-0000-7000-8000-000000000001"))
    val wall = ElementId(Uuid.parse("019a5f00-0000-7000-8000-000000000002"))
    val window = ElementId(Uuid.parse("019a5f00-0000-7000-8000-000000000003"))
    val drawn = Wall(level, PointMm(0.mm, 0.mm), PointMm(6_400.mm, 0.mm), 380.mm, 2_800.mm)
    val body = listOf(PointM(-0.19, -0.19), PointM(6.59, -0.19), PointM(6.21, 0.19), PointM(0.19, 0.19))
    val gap = listOf(PointM(2.25, -0.19), PointM(3.75, -0.19), PointM(3.75, 0.19), PointM(2.25, 0.19))
    OtvesTheme {
        EditorScreen(
            state =
                EditorUiState(
                    ready = true,
                    levels = listOf(level),
                    level = level,
                    drawing =
                        Drawing(
                            polygons =
                                listOf(
                                    DrawnPolygon(wall, FillKind.WALL, StrokeKind.WALL_EDGE, body),
                                    DrawnPolygon(window, FillKind.GAP, StrokeKind.NONE, gap),
                                ),
                            lines =
                                listOf(
                                    DrawnLine(window, StrokeKind.WALL_EDGE, gap[0], gap[3]),
                                    DrawnLine(window, StrokeKind.WALL_EDGE, gap[1], gap[2]),
                                    DrawnLine(window, StrokeKind.FRAME, PointM(2.25, 0.0), PointM(3.75, 0.0)),
                                ),
                            texts = listOf(DrawnText(wall, TextKind.LENGTH, 6_400.mm, PointM(3.2, 0.42), PointM(1.0, 0.0))),
                        ),
                    outlines = listOf(WallOutline(wall, level, body, length = 6_400.mm)),
                    dimensions = listOf(Dimension(wall, drawn, 6_400.mm)),
                    draft = Draft(PointMm(6_400.mm, 0.mm), Aimed(PointMm(6_400.mm, 4_200.mm), length = 4_200.mm)),
                    drawn = wall,
                    canUndo = true,
                ),
            actions = EditorActions(),
        )
    }
}
