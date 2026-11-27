package uz.otves.feature.update

import androidx.compose.animation.core.animateFloatAsState
import androidx.compose.animation.core.tween
import androidx.compose.foundation.Canvas
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.remember
import androidx.compose.ui.Modifier
import androidx.compose.ui.geometry.CornerRadius
import androidx.compose.ui.geometry.Offset
import androidx.compose.ui.geometry.Size
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.Path
import androidx.compose.ui.graphics.PathEffect
import androidx.compose.ui.graphics.StrokeCap
import androidx.compose.ui.graphics.StrokeJoin
import androidx.compose.ui.graphics.drawscope.DrawScope
import androidx.compose.ui.graphics.drawscope.Stroke
import androidx.compose.ui.graphics.lerp
import androidx.compose.ui.text.TextStyle
import androidx.compose.ui.text.drawText
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.rememberTextMeasurer
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import uz.otves.core.designsystem.OtvesColors
import uz.otves.core.designsystem.OtvesTheme

/**
 * The plumb bob of the mark over a level line that carries the new version as its level mark.
 * The bob comes down as [progress] goes from 0 to 1 and stands on the line at 1.
 */
@Composable
internal fun UpdateHero(
    progress: Float,
    versionName: String,
    modifier: Modifier = Modifier,
) {
    val colors = OtvesTheme.colors
    val measurer = rememberTextMeasurer()
    val label =
        remember(versionName, colors) {
            measurer.measure(versionName, TextStyle(color = colors.canvas, fontSize = 14.sp, fontWeight = FontWeight.SemiBold))
        }
    val drop by animateFloatAsState(progress, animationSpec = tween(DROP_MS), label = "drop")
    Canvas(modifier) {
        drawGrid(colors)
        val line = size.height * LINE_SHARE
        val centre = size.width / 2
        // On a short screen the bob starts nearer the line, so its cap stays in view.
        val gap = (line - BobHeight.toPx() - StringInView.toPx()).coerceIn(MinGap.toPx(), Gap.toPx())
        val tip = line - gap * (1f - drop)

        if (line - tip > 1.dp.toPx()) {
            drawLine(
                color = colors.inkSecondary,
                start = Offset(centre, tip),
                end = Offset(centre, line),
                strokeWidth = 1.2.dp.toPx(),
                pathEffect = PathEffect.dashPathEffect(floatArrayOf(3.dp.toPx(), 4.dp.toPx())),
            )
        }
        drawLine(
            color = colors.canvas,
            start = Offset(LineInset.toPx(), line),
            end = Offset(size.width - LineInset.toPx(), line),
            strokeWidth = 2.4.dp.toPx(),
            cap = StrokeCap.Round,
        )

        // The level mark of a drawing (ГОСТ 21.101): a triangle on the line, a shelf, the level over it.
        val mark = size.width - MarkInset.toPx()
        val head = 18.dp.toPx()
        val shelf = line - 36.dp.toPx()
        val triangle =
            Path().apply {
                moveTo(mark - 10.dp.toPx(), line - head)
                lineTo(mark + 10.dp.toPx(), line - head)
                lineTo(mark, line)
                close()
            }
        drawPath(triangle, colors.canvas, style = Stroke(1.6.dp.toPx(), join = StrokeJoin.Round))
        val leader =
            Path().apply {
                moveTo(mark, line - head)
                lineTo(mark, shelf)
                lineTo(mark + maxOf(46.dp.toPx(), label.size.width + 8.dp.toPx()), shelf)
            }
        drawPath(leader, colors.canvas, style = Stroke(1.2.dp.toPx()))
        drawText(label, topLeft = Offset(mark + 4.dp.toPx(), shelf - 4.dp.toPx() - label.size.height))

        drawBob(colors, centre, tip)
    }
}

private fun DrawScope.drawGrid(colors: OtvesColors) {
    val step = 24.dp.toPx()
    val faint = colors.surface.copy(alpha = 0.045f)
    var x = step
    while (x < size.width) {
        drawLine(faint, Offset(x, 0f), Offset(x, size.height), strokeWidth = 1.dp.toPx())
        x += step
    }
    var y = step
    while (y < size.height) {
        drawLine(faint, Offset(0f, y), Offset(size.width, y), strokeWidth = 1.dp.toPx())
        y += step
    }
}

// The bob of ic_otves_mark on its 100-unit grid, hung from the top edge with its tip at [tip].
private fun DrawScope.drawBob(
    colors: OtvesColors,
    centre: Float,
    tip: Float,
) {
    val unit = BobUnit.toPx()

    fun at(
        x: Float,
        y: Float,
    ) = Offset(centre + (x - 50f) * unit, tip + (y - TIP_Y) * unit)

    drawLine(colors.canvas, Offset(centre, 0f), at(50f, 30f), strokeWidth = 2.2f * unit, cap = StrokeCap.Round)
    drawRoundRect(
        color = lerp(colors.brass, Color.White, 0.15f),
        topLeft = at(45f, 28.5f),
        size = Size(10f * unit, 8f * unit),
        cornerRadius = CornerRadius(1.8f * unit),
    )
    drawRoundRect(
        color = lerp(colors.brass, Color.White, 0.3f),
        topLeft = at(34f, 36f),
        size = Size(32f * unit, 4.4f * unit),
        cornerRadius = CornerRadius(0.8f * unit),
    )
    val lit =
        Path().apply {
            moveTo(at(34f, 40.4f).x, at(34f, 40.4f).y)
            lineTo(at(50f, 40.4f).x, at(50f, 40.4f).y)
            lineTo(at(50f, TIP_Y).x, at(50f, TIP_Y).y)
            close()
        }
    drawPath(lit, colors.brass)
    val shaded =
        Path().apply {
            moveTo(at(50f, 40.4f).x, at(50f, 40.4f).y)
            lineTo(at(66f, 40.4f).x, at(66f, 40.4f).y)
            lineTo(at(50f, TIP_Y).x, at(50f, TIP_Y).y)
            close()
        }
    drawPath(shaded, colors.brassShade)
}

private const val DROP_MS = 400

// Where the level line is, as a share of the height.
private const val LINE_SHARE = 0.785f

// The y of the bob's tip on the mark's grid.
private const val TIP_Y = 86.6f

private val BobUnit = 1.5.dp
private val BobHeight = 88.dp
private val StringInView = 48.dp
private val Gap = 80.dp
private val MinGap = 16.dp
private val LineInset = 56.dp
private val MarkInset = 90.dp
