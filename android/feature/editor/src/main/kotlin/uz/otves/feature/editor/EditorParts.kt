package uz.otves.feature.editor

import androidx.compose.foundation.BorderStroke
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.heightIn
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.shape.CircleShape
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.Icon
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.ProvideTextStyle
import androidx.compose.material3.Surface
import androidx.compose.material3.Text
import androidx.compose.material3.TextButton
import androidx.compose.runtime.Composable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.vector.ImageVector
import androidx.compose.ui.res.stringResource
import androidx.compose.ui.semantics.contentDescription
import androidx.compose.ui.semantics.selected
import androidx.compose.ui.semantics.semantics
import androidx.compose.ui.unit.dp
import uz.otves.core.designsystem.OtvesDimens
import uz.otves.core.designsystem.OtvesIcons
import uz.otves.core.designsystem.OtvesTheme
import uz.otves.core.model.Mm
import java.util.Locale

// A floating command or choice; the chosen one is inked.
@Composable
internal fun Pill(
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
internal fun RoundButton(
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
internal fun Panel(content: @Composable () -> Unit) {
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
internal fun PanelHeader(
    title: String,
    actions: @Composable () -> Unit,
) {
    Row(verticalAlignment = Alignment.CenterVertically) {
        Text(title, style = MaterialTheme.typography.titleMedium, modifier = Modifier.weight(1f))
        actions()
    }
}

@Composable
internal fun PanelAction(
    label: String,
    onClick: () -> Unit,
    enabled: Boolean = true,
) {
    TextButton(onClick = onClick, enabled = enabled, modifier = Modifier.heightIn(min = OtvesDimens.minTarget)) { Text(label) }
}

@Composable
internal fun NumberKeys(
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

// Whole millimetres with a no-break space between thousands, as on drawings: «6 400».
internal fun grouped(value: Mm): String = grouped(value.value)

internal fun grouped(value: Long): String = String.format(Locale.ROOT, "%,d", value).replace(',', '\u00A0')

internal val FloatElevation = 2.dp

internal val PanelShape = RoundedCornerShape(10.dp)

private val PadRows = listOf(listOf(7, 8, 9), listOf(4, 5, 6), listOf(1, 2, 3))
