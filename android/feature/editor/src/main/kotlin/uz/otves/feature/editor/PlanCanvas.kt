package uz.otves.feature.editor

import androidx.compose.foundation.Canvas
import androidx.compose.foundation.gestures.detectTapGestures
import androidx.compose.foundation.gestures.detectTransformGestures
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableFloatStateOf
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.rememberUpdatedState
import androidx.compose.runtime.setValue
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clipToBounds
import androidx.compose.ui.geometry.Offset
import androidx.compose.ui.geometry.Size
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.Path
import androidx.compose.ui.graphics.drawscope.DrawScope
import androidx.compose.ui.graphics.drawscope.Stroke
import androidx.compose.ui.input.pointer.pointerInput
import androidx.compose.ui.layout.onSizeChanged
import androidx.compose.ui.platform.LocalDensity
import androidx.compose.ui.unit.dp
import uz.otves.core.designsystem.OtvesTheme
import uz.otves.core.model.ElementId
import uz.otves.core.model.Mm
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
 * the storey below in outline. Pinch to zoom, drag to pan, tap to build. Taps go out in plan
 * millimetres, with how far around the finger still counts as touched at the current zoom.
 */
@Composable
internal fun PlanCanvas(
    outlines: List<WallOutline>,
    openings: List<OpeningOutline>,
    below: List<WallOutline>,
    selected: ElementId?,
    wallStart: PointMm?,
    onTap: (at: PointMm, reach: Mm) -> Unit,
    modifier: Modifier = Modifier,
) {
    val density = LocalDensity.current
    val view = remember { PlanView(pixelsPerMetre = with(density) { START_SCALE.dp.toPx() }) }
    val tap by rememberUpdatedState(onTap)
    val colors = OtvesTheme.colors
    val wallStroke = with(density) { 1.dp.toPx() }
    val markRadius = with(density) { 6.dp.toPx() }
    val selectionStroke = with(density) { 2.dp.toPx() }
    val touchReach = with(density) { TOUCH_REACH.dp.toPx() }

    Canvas(
        modifier =
            modifier
                .clipToBounds()
                .onSizeChanged { size -> view.centre(size.width.toFloat(), size.height.toFloat()) }
                .pointerInput(Unit) {
                    detectTransformGestures { centroid, pan, zoom, _ -> view.move(centroid, pan, zoom) }
                }.pointerInput(Unit) {
                    detectTapGestures { offset -> tap(view.toPlan(offset), view.toPlanLength(touchReach)) }
                },
    ) {
        grid(view, colors.divider, colors.border)
        for (outline in below) {
            drawPath(path(view, outline), colors.inkTertiary, style = Stroke(width = wallStroke))
        }
        for (outline in outlines) {
            val path = path(view, outline)
            drawPath(path, colors.inkTertiary)
            drawPath(path, colors.ink, style = Stroke(width = wallStroke))
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
        openings.find { it.opening == selected }?.let { opening ->
            drawPath(path(view, opening.points), colors.selection, style = Stroke(width = selectionStroke))
        }
        if (wallStart != null) {
            val at = view.toScreen(wallStart.x.value / MM_PER_METRE, wallStart.y.value / MM_PER_METRE)
            drawCircle(colors.selection, radius = markRadius, center = at)
        }
    }
}

/** Where the plan sits on screen: the screen point of the plan origin and the zoom. */
private class PlanView(
    pixelsPerMetre: Float,
) {
    var scale by mutableFloatStateOf(pixelsPerMetre)
    var origin by mutableStateOf(Offset.Unspecified)

    // The origin starts near the lower left, so the first walls grow up and to the right.
    fun centre(
        width: Float,
        height: Float,
    ) {
        if (origin == Offset.Unspecified) {
            origin = Offset(width * START_LEFT, height * START_BOTTOM)
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
private const val HALF_TURN = 180f
private const val FULL_TURN = 360f
