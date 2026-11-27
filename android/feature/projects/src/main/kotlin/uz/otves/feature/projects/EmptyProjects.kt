package uz.otves.feature.projects

import androidx.compose.foundation.Canvas
import androidx.compose.foundation.background
import androidx.compose.foundation.border
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.shape.CircleShape
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.foundation.verticalScroll
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.remember
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.geometry.Offset
import androidx.compose.ui.graphics.PathEffect
import androidx.compose.ui.graphics.StrokeCap
import androidx.compose.ui.graphics.drawscope.Stroke
import androidx.compose.ui.res.stringResource
import androidx.compose.ui.semantics.clearAndSetSemantics
import androidx.compose.ui.semantics.contentDescription
import androidx.compose.ui.text.TextStyle
import androidx.compose.ui.text.drawText
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.rememberTextMeasurer
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import uz.otves.core.designsystem.OtvesDimens
import uz.otves.core.designsystem.OtvesTheme

/** A phone with no jobs yet: a plan just begun, and the three steps from a plan to an estimate. */
@Composable
internal fun EmptyProjects(modifier: Modifier = Modifier) {
    val colors = OtvesTheme.colors
    Column(modifier = modifier.fillMaxSize().verticalScroll(rememberScrollState()).padding(horizontal = OtvesDimens.gutter)) {
        BegunPlan(modifier = Modifier.fillMaxWidth().height(200.dp))
        Spacer(Modifier.height(28.dp))
        Text(
            text = stringResource(R.string.projects_empty_title),
            style = MaterialTheme.typography.titleLarge,
            fontWeight = FontWeight.SemiBold,
        )
        Spacer(Modifier.height(8.dp))
        Text(
            text = stringResource(R.string.projects_empty_body),
            style = MaterialTheme.typography.bodyLarge,
            color = colors.inkSecondary,
        )
        Spacer(Modifier.height(24.dp))
        Column(verticalArrangement = Arrangement.spacedBy(16.dp)) {
            Step(1, stringResource(R.string.projects_step_plan), stringResource(R.string.projects_step_plan_text))
            Step(2, stringResource(R.string.projects_step_materials), stringResource(R.string.projects_step_materials_text))
            Step(3, stringResource(R.string.projects_step_estimate), stringResource(R.string.projects_step_estimate_text))
        }
        Spacer(Modifier.height(16.dp))
    }
}

// The first wall of a house on squared paper, with its length over it and the rest still to draw.
@Composable
private fun BegunPlan(modifier: Modifier = Modifier) {
    val colors = OtvesTheme.colors
    val shape = RoundedCornerShape(20.dp)
    val picture = stringResource(R.string.projects_empty_picture)
    val measurer = rememberTextMeasurer()
    val length =
        remember(colors) {
            measurer.measure(WALL_LENGTH, TextStyle(color = colors.inkSecondary, fontSize = 12.sp, fontWeight = FontWeight.SemiBold))
        }
    Box(
        modifier =
            modifier
                .clip(shape)
                .background(colors.canvas)
                .squaredPaper(colors.border)
                .border(1.dp, colors.border, shape)
                .clearAndSetSemantics { contentDescription = picture },
    ) {
        Canvas(Modifier.fillMaxSize()) {
            val left = size.width * 0.29f
            val right = size.width * 0.71f
            val top = size.height * 0.26f
            val bottom = size.height * 0.8f
            val dashes = PathEffect.dashPathEffect(floatArrayOf(5.dp.toPx(), 5.dp.toPx()))
            val rest = Stroke(1.6.dp.toPx(), pathEffect = dashes)
            drawLine(colors.inkTertiary, Offset(right, top), Offset(right, bottom), rest.width, pathEffect = dashes)
            drawLine(colors.inkTertiary, Offset(right, bottom), Offset(left, bottom), rest.width, pathEffect = dashes)
            drawLine(colors.inkTertiary, Offset(left, bottom), Offset(left, top), rest.width, pathEffect = dashes)
            drawLine(colors.ink, Offset(left, top), Offset(right, top), 5.dp.toPx(), cap = StrokeCap.Butt)

            // The dimension line of the wall: ticks at its ends, the length over the middle.
            val line = top - 16.dp.toPx()
            val tick = 5.dp.toPx()
            drawLine(colors.inkTertiary, Offset(left, line), Offset(right, line), 1.dp.toPx())
            drawLine(colors.inkTertiary, Offset(left, line - tick), Offset(left, line + 2 * tick), 1.dp.toPx())
            drawLine(colors.inkTertiary, Offset(right, line - tick), Offset(right, line + 2 * tick), 1.dp.toPx())
            drawLine(colors.inkSecondary, Offset(left - tick, line + tick), Offset(left + tick, line - tick), 1.2.dp.toPx())
            drawLine(colors.inkSecondary, Offset(right - tick, line + tick), Offset(right + tick, line - tick), 1.2.dp.toPx())
            drawText(length, topLeft = Offset((left + right - length.size.width) / 2, line - 4.dp.toPx() - length.size.height))

            // Where the wall began, and where the finger is now.
            drawCircle(colors.surface, 5.dp.toPx(), Offset(left, top))
            drawCircle(colors.ink, 5.dp.toPx(), Offset(left, top), style = Stroke(2.dp.toPx()))
            drawCircle(colors.ink.copy(alpha = 0.12f), 10.dp.toPx(), Offset(right, top))
            drawCircle(colors.ink, 5.dp.toPx(), Offset(right, top))
        }
    }
}

@Composable
private fun Step(
    number: Int,
    title: String,
    text: String,
) {
    val colors = OtvesTheme.colors
    Row(horizontalArrangement = Arrangement.spacedBy(14.dp)) {
        Box(
            modifier = Modifier.size(28.dp).border(1.5.dp, colors.ink, CircleShape),
            contentAlignment = Alignment.Center,
        ) {
            Text(number.toString(), style = MaterialTheme.typography.labelLarge, fontWeight = FontWeight.SemiBold)
        }
        Column {
            Text(title, style = MaterialTheme.typography.titleMedium, fontWeight = FontWeight.SemiBold)
            Text(text, style = MaterialTheme.typography.bodyMedium, color = colors.inkSecondary)
        }
    }
}

// Millimetres, as a plan writes a length.
private const val WALL_LENGTH = "12 700"
