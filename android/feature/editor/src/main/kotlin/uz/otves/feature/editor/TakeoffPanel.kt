package uz.otves.feature.editor

import android.content.Context
import android.content.Intent
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.heightIn
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.items
import androidx.compose.material3.DropdownMenu
import androidx.compose.material3.DropdownMenuItem
import androidx.compose.material3.HorizontalDivider
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Surface
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.rememberCoroutineScope
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.res.stringResource
import androidx.compose.ui.unit.dp
import kotlinx.coroutines.launch
import uz.otves.core.designsystem.OtvesTheme
import uz.otves.core.model.Money
import uz.otves.engine.MasonryUnitOption
import uz.otves.engine.QuantityLine

// The materials the model asks for, and what the storey's walls are reinforced with: the lines
// follow these choices at once.
@Composable
internal fun TakeoffPanel(
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
                onPdf = { scope.launch { context.shareEstimate(state, actions.onSheets) } },
            )
            PanelAction(stringResource(R.string.takeoff_close), onClose)
        }
        when {
            state.takeoffFailed -> {
                Text(stringResource(R.string.takeoff_failed), color = OtvesTheme.colors.warning)
            }

            // The take-off is the whole job's, whichever storey is shown.
            state.storeys.all { it.outlines.isEmpty() } -> {
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

private fun Context.share(text: String) {
    val send =
        Intent(Intent.ACTION_SEND)
            .setType("text/plain")
            .putExtra(Intent.EXTRA_TEXT, text)
    startActivity(Intent.createChooser(send, null))
}

private val TakeoffHeight = 440.dp
