package uz.otves.feature.projects

import android.graphics.BitmapFactory
import androidx.compose.foundation.Image
import androidx.compose.foundation.background
import androidx.compose.foundation.border
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.heightIn
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.ExperimentalMaterial3Api
import androidx.compose.material3.HorizontalDivider
import androidx.compose.material3.Icon
import androidx.compose.material3.IconButton
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.ModalBottomSheet
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.produceState
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.draw.drawBehind
import androidx.compose.ui.geometry.Offset
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.ImageBitmap
import androidx.compose.ui.graphics.asImageBitmap
import androidx.compose.ui.graphics.vector.ImageVector
import androidx.compose.ui.layout.ContentScale
import androidx.compose.ui.res.stringResource
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.style.TextOverflow
import androidx.compose.ui.unit.Dp
import androidx.compose.ui.unit.dp
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.withContext
import uz.otves.core.designsystem.OtvesDimens
import uz.otves.core.designsystem.OtvesIcons
import uz.otves.core.designsystem.OtvesTheme
import uz.otves.data.project.JobSummary

/** The job worked on last: its plan, what it measures and costs, and when it changed. */
@Composable
internal fun LatestJobCard(
    job: JobSummary,
    onOpen: () -> Unit,
    onActions: () -> Unit,
    modifier: Modifier = Modifier,
) {
    val colors = OtvesTheme.colors
    val shape = RoundedCornerShape(20.dp)
    Column(
        modifier =
            modifier
                .fillMaxWidth()
                .clip(shape)
                .border(1.dp, colors.border, shape)
                .clickable(onClick = onOpen),
    ) {
        PlanPicture(job = job, inset = 20.dp, modifier = Modifier.fillMaxWidth().height(168.dp))
        HorizontalDivider(color = colors.border)
        Row(
            modifier = Modifier.padding(start = 16.dp, top = 14.dp, bottom = 16.dp, end = 4.dp),
            verticalAlignment = Alignment.CenterVertically,
        ) {
            Column(modifier = Modifier.weight(1f), verticalArrangement = Arrangement.spacedBy(4.dp)) {
                Text(
                    text = job.name,
                    style = MaterialTheme.typography.titleLarge,
                    fontWeight = FontWeight.SemiBold,
                    maxLines = 1,
                    overflow = TextOverflow.Ellipsis,
                )
                val area = areaText(job)
                val cost = costText(job)
                if (area != null || cost != null) {
                    Row(horizontalArrangement = Arrangement.spacedBy(16.dp)) {
                        area?.let { Text(it, style = MaterialTheme.typography.titleMedium, fontWeight = FontWeight.SemiBold) }
                        cost?.let { Text(it, style = MaterialTheme.typography.titleMedium, fontWeight = FontWeight.SemiBold) }
                    }
                } else if (job.isBlank) {
                    Text(
                        text = stringResource(R.string.projects_plan_empty),
                        style = MaterialTheme.typography.bodyLarge,
                        color = colors.inkSecondary,
                    )
                }
                Text(changedText(job.updatedAt), style = MaterialTheme.typography.bodyMedium, color = colors.inkSecondary)
            }
            ActionsButton(job, onActions)
        }
    }
}

/** One of the other jobs: a small plan, the name and a line of what it measures, costs and when it changed. */
@Composable
internal fun JobRow(
    job: JobSummary,
    onOpen: () -> Unit,
    onActions: () -> Unit,
) {
    Row(
        modifier =
            Modifier
                .fillMaxWidth()
                .heightIn(min = RowHeight)
                .clickable(onClick = onOpen)
                .padding(start = OtvesDimens.gutter, end = 4.dp, top = 10.dp, bottom = 10.dp),
        verticalAlignment = Alignment.CenterVertically,
    ) {
        PlanPicture(job = job, inset = 6.dp, modifier = Modifier.size(52.dp), shape = RoundedCornerShape(12.dp), outlined = true)
        Column(modifier = Modifier.weight(1f).padding(start = 14.dp), verticalArrangement = Arrangement.spacedBy(2.dp)) {
            Text(
                text = job.name,
                style = MaterialTheme.typography.titleMedium,
                fontWeight = FontWeight.SemiBold,
                maxLines = 1,
                overflow = TextOverflow.Ellipsis,
            )
            Text(
                text = summaryText(job),
                style = MaterialTheme.typography.bodyMedium,
                color = OtvesTheme.colors.inkSecondary,
                maxLines = 1,
                overflow = TextOverflow.Ellipsis,
            )
        }
        ActionsButton(job, onActions)
    }
}

@Composable
private fun ActionsButton(
    job: JobSummary,
    onActions: () -> Unit,
) {
    IconButton(onClick = onActions, modifier = Modifier.size(OtvesDimens.minTarget)) {
        Icon(
            imageVector = OtvesIcons.More,
            contentDescription = stringResource(R.string.projects_actions, job.name),
            tint = OtvesTheme.colors.inkSecondary,
        )
    }
}

/**
 * The job's plan on squared paper, as the editor painted it for the card; a job with no walls
 * shows the paper alone.
 */
@Composable
internal fun PlanPicture(
    job: JobSummary,
    inset: Dp,
    modifier: Modifier = Modifier,
    shape: RoundedCornerShape = RoundedCornerShape(0.dp),
    outlined: Boolean = false,
) {
    val colors = OtvesTheme.colors
    val file = job.card?.preview
    // The file keeps its name when the plan changes, so the time of the change is a key too.
    val plan by produceState<ImageBitmap?>(null, file, job.updatedAt) {
        value = file?.let { withContext(Dispatchers.IO) { BitmapFactory.decodeFile(it.path)?.asImageBitmap() } }
    }
    val frame = if (outlined) Modifier.border(1.dp, colors.border, shape) else Modifier
    Box(
        modifier =
            modifier
                .clip(shape)
                .background(colors.canvas)
                .squaredPaper(colors.border)
                .then(frame),
        contentAlignment = Alignment.Center,
    ) {
        plan?.let {
            Image(bitmap = it, contentDescription = null, contentScale = ContentScale.Fit, modifier = Modifier.fillMaxSize().padding(inset))
        }
    }
}

/** The squares of drawing paper, [Grid] apart. */
internal fun Modifier.squaredPaper(line: Color): Modifier =
    drawBehind {
        val step = Grid.toPx()
        val faint = line.copy(alpha = 0.55f)
        var x = step
        while (x < size.width) {
            drawLine(faint, Offset(x, 0f), Offset(x, size.height), strokeWidth = 1f)
            x += step
        }
        var y = step
        while (y < size.height) {
            drawLine(faint, Offset(0f, y), Offset(size.width, y), strokeWidth = 1f)
            y += step
        }
    }

/** What can be done with a job besides opening it. */
@OptIn(ExperimentalMaterial3Api::class)
@Composable
internal fun JobActionsSheet(
    job: JobSummary,
    onRename: () -> Unit,
    onDelete: () -> Unit,
    onDismiss: () -> Unit,
) {
    val colors = OtvesTheme.colors
    ModalBottomSheet(
        onDismissRequest = onDismiss,
        containerColor = colors.surface,
        shape = RoundedCornerShape(topStart = 24.dp, topEnd = 24.dp),
    ) {
        Row(
            modifier = Modifier.padding(start = 20.dp, end = 20.dp, bottom = 16.dp),
            horizontalArrangement = Arrangement.spacedBy(14.dp),
            verticalAlignment = Alignment.CenterVertically,
        ) {
            PlanPicture(job = job, inset = 6.dp, modifier = Modifier.size(52.dp), shape = RoundedCornerShape(12.dp), outlined = true)
            Column(verticalArrangement = Arrangement.spacedBy(2.dp)) {
                Text(
                    text = job.name,
                    style = MaterialTheme.typography.titleMedium,
                    fontWeight = FontWeight.SemiBold,
                    maxLines = 1,
                    overflow = TextOverflow.Ellipsis,
                )
                Text(summaryText(job), style = MaterialTheme.typography.bodyMedium, color = colors.inkSecondary, maxLines = 1)
            }
        }
        HorizontalDivider(color = colors.divider)
        ActionRow(OtvesIcons.Pencil, stringResource(R.string.projects_rename), onRename)
        HorizontalDivider(color = colors.divider)
        ActionRow(OtvesIcons.Trash, stringResource(R.string.projects_delete), onDelete)
    }
}

@Composable
private fun ActionRow(
    icon: ImageVector,
    label: String,
    onClick: () -> Unit,
) {
    Row(
        modifier =
            Modifier
                .fillMaxWidth()
                .height(OtvesDimens.primaryTarget)
                .clickable(onClick = onClick)
                .padding(horizontal = 20.dp),
        horizontalArrangement = Arrangement.spacedBy(16.dp),
        verticalAlignment = Alignment.CenterVertically,
    ) {
        Icon(icon, contentDescription = null, tint = OtvesTheme.colors.ink, modifier = Modifier.size(24.dp))
        Text(label, style = MaterialTheme.typography.bodyLarge)
    }
}

private val RowHeight = 72.dp
private val Grid = 12.dp
