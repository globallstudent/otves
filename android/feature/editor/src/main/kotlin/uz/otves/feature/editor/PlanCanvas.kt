package uz.otves.feature.editor

import androidx.compose.foundation.Canvas
import androidx.compose.foundation.gestures.awaitEachGesture
import androidx.compose.foundation.gestures.awaitFirstDown
import androidx.compose.foundation.gestures.calculateCentroid
import androidx.compose.foundation.gestures.calculatePan
import androidx.compose.foundation.gestures.calculateZoom
import androidx.compose.material3.MaterialTheme
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableFloatStateOf
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.rememberUpdatedState
import androidx.compose.runtime.setValue
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clipToBounds
import androidx.compose.ui.geometry.Offset
import androidx.compose.ui.geometry.Rect
import androidx.compose.ui.geometry.Size
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.Path
import androidx.compose.ui.graphics.PathEffect
import androidx.compose.ui.graphics.StrokeCap
import androidx.compose.ui.graphics.drawscope.DrawScope
import androidx.compose.ui.graphics.drawscope.Stroke
import androidx.compose.ui.graphics.drawscope.rotate
import androidx.compose.ui.input.pointer.pointerInput
import androidx.compose.ui.layout.onSizeChanged
import androidx.compose.ui.platform.LocalDensity
import androidx.compose.ui.platform.LocalViewConfiguration
import androidx.compose.ui.text.TextLayoutResult
import androidx.compose.ui.text.drawText
import androidx.compose.ui.text.rememberTextMeasurer
import androidx.compose.ui.unit.dp
import uz.otves.core.designsystem.OtvesTheme
import uz.otves.core.model.ElementId
import uz.otves.core.model.Mm
import uz.otves.engine.Issue
import uz.otves.engine.LocationLine
import uz.otves.engine.OpeningKind
import uz.otves.engine.OpeningOutline
import uz.otves.engine.PointM
import uz.otves.engine.PointMm
import uz.otves.engine.WallOutline
import kotlin.math.atan2
import kotlin.math.ceil
import kotlin.math.floor
import kotlin.math.roundToLong

/**
 * The plan as the engine derived it: wall and opening outlines in metres, drawn with y up, over
 * the storey below in outline. Two fingers always move and zoom the plan. One finger moves it
 * too, except while [drawing]: then it aims the wall's next point and lifting it places the point.
 * Touches go out in plan millimetres, with how far around the finger still counts as touched at
 * the current zoom.
 */
@Composable
internal fun PlanCanvas(
    outlines: List<WallOutline>,
    openings: List<OpeningOutline>,
    below: List<WallOutline>,
    selected: ElementId?,
    issues: List<Issue>,
    dimensions: List<Dimension>,
    unchosen: Set<ElementId>,
    draft: Draft?,
    draftThickness: Mm,
    drawing: Boolean,
    focus: List<PointMm>,
    fit: Int,
    onTap: (at: PointMm, reach: Mm) -> Unit,
    onAim: (at: PointMm, reach: Mm) -> Unit,
    onAimCancel: () -> Unit,
    onCommit: () -> Unit,
    modifier: Modifier = Modifier,
) {
    val density = LocalDensity.current
    val view = remember { PlanView(pixelsPerMetre = with(density) { START_SCALE.dp.toPx() }) }
    val tap by rememberUpdatedState(onTap)
    val aim by rememberUpdatedState(onAim)
    val aimCancel by rememberUpdatedState(onAimCancel)
    val commit by rememberUpdatedState(onCommit)
    val draws by rememberUpdatedState(drawing)
    val slop = LocalViewConfiguration.current.touchSlop
    val colors = OtvesTheme.colors
    val wallStroke = with(density) { 1.dp.toPx() }
    val markRadius = with(density) { 6.dp.toPx() }
    val selectionStroke = with(density) { 2.dp.toPx() }
    val touchReach = with(density) { TOUCH_REACH.dp.toPx() }
    val labelGap = with(density) { LABEL_GAP.dp.toPx() }
    val measurer = rememberTextMeasurer()
    LaunchedEffect(focus) { view.reveal(focus) }
    val shown by rememberUpdatedState(outlines)
    LaunchedEffect(fit) { if (fit > 0) view.fit(shown.flatMap { it.points }) }
    val labelStyle = MaterialTheme.typography.labelMedium.copy(color = colors.inkSecondary)
    val draftStyle = MaterialTheme.typography.titleMedium.copy(color = colors.selection)
    val dash = with(density) { 6.dp.toPx() }

    Canvas(
        modifier =
            modifier
                .clipToBounds()
                .onSizeChanged { size -> view.resize(size.width.toFloat(), size.height.toFloat()) }
                .pointerInput(Unit) {
                    awaitEachGesture {
                        val down = awaitFirstDown(requireUnconsumed = false)
                        val aiming = draws
                        var moving = false
                        var travelled = 0f
                        if (aiming) aim(view.toPlan(down.position), view.toPlanLength(touchReach))
                        while (true) {
                            val event = awaitPointerEvent()
                            val pressed = event.changes.filter { it.pressed }
                            if (pressed.isEmpty()) break
                            if (pressed.size > 1 && !moving) {
                                moving = true
                                if (aiming) aimCancel()
                            }
                            if (aiming && !moving) {
                                aim(view.toPlan(pressed.first().position), view.toPlanLength(touchReach))
                            } else {
                                val pan = event.calculatePan()
                                travelled += pan.getDistance()
                                if (travelled > slop) moving = true
                                if (moving) view.move(event.calculateCentroid(useCurrent = false), pan, event.calculateZoom())
                            }
                            event.changes.forEach { it.consume() }
                        }
                        when {
                            moving -> Unit
                            aiming -> commit()
                            else -> tap(view.toPlan(down.position), view.toPlanLength(touchReach))
                        }
                    }
                },
    ) {
        grid(view, colors.divider, colors.border)
        for (outline in below) {
            drawPath(path(view, outline), colors.inkTertiary, style = Stroke(width = wallStroke))
        }
        for (outline in outlines) {
            val path = path(view, outline)
            // A wall with no material yet is an empty outline: it is drawn but not built from anything.
            drawPath(path, if (outline.wall in unchosen) colors.surface else colors.inkTertiary)
            drawPath(path, colors.ink, style = Stroke(width = wallStroke))
        }
        val bodies = outlines.map { outline -> outline.wall to outline.points.map { view.toScreen(it.x, it.y) } }
        for (dimension in dimensions) {
            val others = bodies.filter { it.first != dimension.id }.map { it.second }
            dimension(view, dimension, measurer.measure(dimension.length.value.toString(), labelStyle), labelGap, others)
        }
        // The selection in the plan's only accent colour; openings stay cut out of a selected wall.
        outlines.find { it.wall == selected }?.let { outline ->
            val path = path(view, outline)
            drawPath(path, colors.selection.copy(alpha = SELECTION_FILL))
            drawPath(path, colors.selection, style = Stroke(width = selectionStroke))
        }
        for (opening in openings) {
            opening(view, opening, colors.canvas, colors.ink, wallStroke)
        }
        // What to fix, in the warning colour: openings that do not fit, and points where walls
        // meet wrongly.
        val faulty = issues.mapNotNull { it.element }.toSet()
        for (opening in openings.filter { it.opening in faulty }) {
            drawPath(path(view, opening.points), colors.warning, style = Stroke(width = selectionStroke))
        }
        for (at in issues.mapNotNull { it.at }) {
            val centre = view.toScreen(at.x.value / MM_PER_METRE, at.y.value / MM_PER_METRE)
            drawCircle(colors.warning, radius = markRadius, center = centre, style = Stroke(width = selectionStroke))
        }
        openings.find { it.opening == selected }?.let { opening ->
            drawPath(path(view, opening.points), colors.selection, style = Stroke(width = selectionStroke))
        }
        if (draft != null) {
            val start = view.toScreen(draft.start)
            val end = draft.end?.let(view::toScreen)
            if (end != null) {
                for (guide in draft.guides) {
                    drawLine(
                        colors.selection,
                        view.toScreen(guide.from),
                        view.toScreen(guide.to),
                        strokeWidth = wallStroke,
                        pathEffect = PathEffect.dashPathEffect(floatArrayOf(dash, dash)),
                    )
                }
                val width = (draftThickness.value / MM_PER_METRE * view.scale).toFloat()
                drawLine(colors.selection.copy(alpha = SELECTION_FILL), start, end, strokeWidth = width, cap = StrokeCap.Butt)
                drawLine(colors.selection, start, end, strokeWidth = selectionStroke)
                draft.length?.let { length ->
                    // Above the finger, where it stays readable.
                    val label = measurer.measure(length.value.toString(), draftStyle)
                    val at = end - Offset(label.size.width / 2f, label.size.height + touchReach * LABEL_LIFT)
                    drawText(label, topLeft = at)
                }
                drawCircle(colors.selection, radius = markRadius, center = end)
            }
            drawCircle(colors.selection, radius = markRadius, center = start)
        }
    }
}

/** Where the plan sits on screen: the screen point of the plan origin and the zoom. */
private class PlanView(
    pixelsPerMetre: Float,
) {
    var scale by mutableFloatStateOf(pixelsPerMetre)
    var origin by mutableStateOf(Offset.Unspecified)

    private var size = Size.Zero

    // The origin starts near the lower left, so the first walls grow up and to the right.
    fun resize(
        width: Float,
        height: Float,
    ) {
        size = Size(width, height)
        if (origin == Offset.Unspecified) {
            origin = Offset(width * START_LEFT, height * START_BOTTOM)
        }
    }

    // Shows all of [points] between the controls along the top and the bottom, as large as fits.
    fun fit(points: List<PointM>) {
        if (points.isEmpty() || size == Size.Zero) return
        val left = points.minOf { it.x }
        val right = points.maxOf { it.x }
        val bottom = points.minOf { it.y }
        val top = points.maxOf { it.y }
        val area = Rect(size.width * FIT_SIDE, size.height * FIT_TOP, size.width * (1 - FIT_SIDE), size.height * FIT_BOTTOM)
        val wide = (right - left).coerceAtLeast(MIN_EXTENT)
        val tall = (top - bottom).coerceAtLeast(MIN_EXTENT)
        scale = minOf(area.width / wide, area.height / tall).toFloat().coerceIn(MIN_SCALE, MAX_SCALE)
        origin =
            Offset(
                area.center.x - ((left + right) / 2 * scale).toFloat(),
                area.center.y + ((bottom + top) / 2 * scale).toFloat(),
            )
    }

    // Moves the plan so [points] show in the upper part of the screen, which a panel never covers,
    // zooming out when they do not fit there; points already there stay put.
    fun reveal(points: List<PointMm>) {
        if (points.isEmpty() || origin == Offset.Unspecified || size == Size.Zero) return
        val clear = Rect(size.width * REVEAL_SIDE, size.height * REVEAL_TOP, size.width * (1 - REVEAL_SIDE), size.height * REVEAL_BOTTOM)
        val wide = (points.maxOf { it.x.value } - points.minOf { it.x.value }) / MM_PER_METRE * scale
        val tall = (points.maxOf { it.y.value } - points.minOf { it.y.value }) / MM_PER_METRE * scale
        val shrink = minOf(clear.width / wide, clear.height / tall).toFloat()
        if (shrink < 1f) scale = (scale * shrink).coerceAtLeast(MIN_SCALE)
        val at = points.map(::toScreen)
        if (at.any { !clear.contains(it) }) {
            val centre = Offset((at.minOf { it.x } + at.maxOf { it.x }) / 2f, (at.minOf { it.y } + at.maxOf { it.y }) / 2f)
            origin += clear.center - centre
        }
    }

    fun move(
        centroid: Offset,
        pan: Offset,
        zoom: Float,
    ) {
        val next = (scale * zoom).coerceIn(MIN_SCALE, MAX_SCALE)
        origin = centroid - (centroid - origin) * (next / scale) + pan
        scale = next
    }

    fun toScreen(
        x: Double,
        y: Double,
    ): Offset = Offset(origin.x + (x * scale).toFloat(), origin.y - (y * scale).toFloat())

    fun toScreen(point: PointMm): Offset = toScreen(point.x.value / MM_PER_METRE, point.y.value / MM_PER_METRE)

    fun toPlanLength(pixels: Float): Mm = Mm((pixels / scale * MM_PER_METRE).roundToLong())

    fun toPlan(offset: Offset): PointMm {
        val x = (offset.x - origin.x) / scale
        val y = (origin.y - offset.y) / scale
        return PointMm(Mm((x * MM_PER_METRE).roundToLong()), Mm((y * MM_PER_METRE).roundToLong()))
    }
}

private fun path(
    view: PlanView,
    outline: WallOutline,
): Path = path(view, outline.points)

private fun path(
    view: PlanView,
    points: List<PointM>,
): Path =
    Path().apply {
        points.forEachIndexed { index, point ->
            val at = view.toScreen(point.x, point.y)
            if (index == 0) moveTo(at.x, at.y) else lineTo(at.x, at.y)
        }
        close()
    }

// The wall's length beside it, along the wall and never upside down: by its left face at the
// middle unless another wall lies there, then by the other face or nearer an end. Left out when
// the wall is too short on screen to hold it.
private fun DrawScope.dimension(
    view: PlanView,
    dimension: Dimension,
    label: TextLayoutResult,
    gap: Float,
    others: List<List<Offset>>,
) {
    val wall = dimension.wall
    val start = view.toScreen(wall.start.x.value / MM_PER_METRE, wall.start.y.value / MM_PER_METRE)
    val end = view.toScreen(wall.end.x.value / MM_PER_METRE, wall.end.y.value / MM_PER_METRE)
    val run = end - start
    val onScreen = run.getDistance()
    if (onScreen == 0f || label.size.width > onScreen * LABEL_FILL) return
    val along = run / onScreen
    // The plan's left of the wall: y runs down on screen, so it is the screen's right turn.
    val left = Offset(along.y, -along.x)
    val face =
        when (wall.locationLine) {
            LocationLine.CENTER -> wall.thickness.value / 2.0
            LocationLine.LEFT_FACE -> 0.0
            LocationLine.RIGHT_FACE -> wall.thickness.value.toDouble()
        }
    val clearance = gap + label.size.height / 2f
    val toLeft = (face / MM_PER_METRE * view.scale).toFloat() + clearance
    val toRight = ((wall.thickness.value - face) / MM_PER_METRE * view.scale).toFloat() + clearance
    val half = along * (label.size.width / 2f)
    val places =
        LABEL_PLACES.flatMap { along -> listOf(start + run * along + left * toLeft, start + run * along - left * toRight) }
    val centre =
        places.firstOrNull { at -> others.none { body -> inside(at, body) || inside(at - half, body) || inside(at + half, body) } }
            ?: places.first()
    var degrees = angle(run)
    // Upright, and on an upright wall read from the bottom up, as on drawings.
    if (degrees >= QUARTER_TURN || degrees < -QUARTER_TURN) degrees += HALF_TURN
    rotate(degrees, pivot = centre) {
        drawText(label, topLeft = centre - Offset(label.size.width / 2f, label.size.height / 2f))
    }
}

// The gap an opening leaves in its wall, with the usual plan symbol: a window's glazing line, a
// door's leaf and swing. Corners come counter-clockwise: near and far end on the right face, then
// far and near end on the left face.
private fun DrawScope.opening(
    view: PlanView,
    opening: OpeningOutline,
    gap: Color,
    ink: Color,
    stroke: Float,
) {
    if (opening.points.size != OUTLINE_CORNERS) return
    val (nearRight, farRight, farLeft, nearLeft) = opening.points.map { view.toScreen(it.x, it.y) }
    val path =
        Path().apply {
            moveTo(nearRight.x, nearRight.y)
            lineTo(farRight.x, farRight.y)
            lineTo(farLeft.x, farLeft.y)
            lineTo(nearLeft.x, nearLeft.y)
            close()
        }
    drawPath(path, gap)
    drawLine(ink, nearRight, nearLeft, stroke)
    drawLine(ink, farRight, farLeft, stroke)
    when (opening.kind) {
        OpeningKind.WINDOW -> {
            drawLine(ink, nearRight, farRight, stroke)
            drawLine(ink, nearLeft, farLeft, stroke)
            drawLine(ink, (nearRight + nearLeft) / 2f, (farRight + farLeft) / 2f, stroke)
        }

        OpeningKind.DOOR -> {
            // The leaf stands open at right angles to the wall, hinged at the near left corner.
            val width = (farLeft - nearLeft).getDistance()
            val across = nearLeft - nearRight
            val leaf = nearLeft + across / across.getDistance() * width
            drawLine(ink, nearLeft, leaf, stroke)
            val start = angle(leaf - nearLeft)
            val sweep = normalised(angle(farLeft - nearLeft) - start)
            drawArc(
                color = ink,
                startAngle = start,
                sweepAngle = sweep,
                useCenter = false,
                topLeft = nearLeft - Offset(width, width),
                size = Size(width * 2, width * 2),
                style = Stroke(width = stroke),
            )
        }

        OpeningKind.EMPTY -> {
            // The jambs alone.
        }
    }
}

// Whether a screen point lies inside a polygon, by counting the edges a ray to the right crosses.
private fun inside(
    point: Offset,
    polygon: List<Offset>,
): Boolean {
    var crossings = 0
    for (index in polygon.indices) {
        val a = polygon[index]
        val b = polygon[(index + 1) % polygon.size]
        if ((a.y > point.y) != (b.y > point.y) && point.x < a.x + (point.y - a.y) / (b.y - a.y) * (b.x - a.x)) crossings++
    }
    return crossings % 2 == 1
}

// Degrees clockwise from +x on screen, as drawArc measures them.
private fun angle(direction: Offset): Float = Math.toDegrees(atan2(direction.y, direction.x).toDouble()).toFloat()

// The shorter way round, in (-180°, 180°].
private fun normalised(degrees: Float): Float =
    when {
        degrees > HALF_TURN -> degrees - FULL_TURN
        degrees <= -HALF_TURN -> degrees + FULL_TURN
        else -> degrees
    }

// One line a metre, with the axes a shade darker.
private fun DrawScope.grid(
    view: PlanView,
    line: Color,
    axis: Color,
) {
    if (view.origin == Offset.Unspecified) return
    val left = floor(-view.origin.x / view.scale).toInt()
    val right = ceil((size.width - view.origin.x) / view.scale).toInt()
    val bottom = floor((view.origin.y - size.height) / view.scale).toInt()
    val top = ceil(view.origin.y / view.scale).toInt()
    for (metre in left..right) {
        val x = view.toScreen(metre.toDouble(), 0.0).x
        drawLine(if (metre == 0) axis else line, Offset(x, 0f), Offset(x, size.height))
    }
    for (metre in bottom..top) {
        val y = view.toScreen(0.0, metre.toDouble()).y
        drawLine(if (metre == 0) axis else line, Offset(0f, y), Offset(size.width, y))
    }
}

private const val MM_PER_METRE = 1000.0
private const val START_SCALE = 40f
private const val MIN_SCALE = 8f
private const val MAX_SCALE = 2_000f
private const val START_LEFT = 0.15f
private const val START_BOTTOM = 0.8f
private const val TOUCH_REACH = 12f
private const val SELECTION_FILL = 0.25f
private const val OUTLINE_CORNERS = 4
private const val QUARTER_TURN = 90f
private const val HALF_TURN = 180f
private const val LABEL_GAP = 4f
private const val LABEL_FILL = 0.8f
private const val FULL_TURN = 360f
private const val LABEL_LIFT = 3f

// Where along a wall its length may be written, the middle first.
private val LABEL_PLACES = listOf(0.5f, 0.3f, 0.7f)
private const val FIT_SIDE = 0.12f
private const val FIT_TOP = 0.2f
private const val FIT_BOTTOM = 0.72f

// A single short wall is not blown up to fill the screen: at least this many metres show.
private const val MIN_EXTENT = 4.0
private const val REVEAL_SIDE = 0.15f
private const val REVEAL_TOP = 0.2f
private const val REVEAL_BOTTOM = 0.38f
