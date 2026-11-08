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
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.Path
import androidx.compose.ui.graphics.drawscope.DrawScope
import androidx.compose.ui.graphics.drawscope.Stroke
import androidx.compose.ui.input.pointer.pointerInput
import androidx.compose.ui.layout.onSizeChanged
import androidx.compose.ui.platform.LocalDensity
import androidx.compose.ui.unit.dp
import uz.otves.core.designsystem.OtvesTheme
import uz.otves.core.model.Mm
import uz.otves.engine.PointMm
import uz.otves.engine.WallOutline
import kotlin.math.ceil
import kotlin.math.floor
import kotlin.math.roundToLong

/**
 * The plan as the engine derived it: wall outlines in metres, drawn with y up. Pinch to zoom,
 * drag to pan, tap to place a wall end. Taps go out in plan millimetres.
 */
@Composable
internal fun PlanCanvas(
    outlines: List<WallOutline>,
    wallStart: PointMm?,
    onTap: (PointMm) -> Unit,
    modifier: Modifier = Modifier,
) {
    val density = LocalDensity.current
    val view = remember { PlanView(pixelsPerMetre = with(density) { START_SCALE.dp.toPx() }) }
    val tap by rememberUpdatedState(onTap)
    val colors = OtvesTheme.colors
    val wallStroke = with(density) { 1.dp.toPx() }
    val markRadius = with(density) { 6.dp.toPx() }

    Canvas(
        modifier =
            modifier
                .clipToBounds()
                .onSizeChanged { size -> view.centre(size.width.toFloat(), size.height.toFloat()) }
                .pointerInput(Unit) {
                    detectTransformGestures { centroid, pan, zoom, _ -> view.move(centroid, pan, zoom) }
                }.pointerInput(Unit) {
                    detectTapGestures { offset -> tap(view.toPlan(offset)) }
                },
    ) {
        grid(view, colors.divider, colors.border)
        for (outline in outlines) {
            val path =
                Path().apply {
                    outline.points.forEachIndexed { index, point ->
                        val at = view.toScreen(point.x, point.y)
                        if (index == 0) moveTo(at.x, at.y) else lineTo(at.x, at.y)
                    }
                    close()
                }
            drawPath(path, colors.inkTertiary)
            drawPath(path, colors.ink, style = Stroke(width = wallStroke))
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

    fun toPlan(offset: Offset): PointMm {
        val x = (offset.x - origin.x) / scale
        val y = (origin.y - offset.y) / scale
        return PointMm(Mm((x * MM_PER_METRE).roundToLong()), Mm((y * MM_PER_METRE).roundToLong()))
    }
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
