package uz.otves.feature.editor

import androidx.compose.runtime.Composable
import androidx.compose.ui.res.stringResource
import uz.otves.engine.QuantityLine
import uz.otves.engine.QuantityUnit
import java.text.DecimalFormat
import java.text.DecimalFormatSymbols
import java.util.Locale

/**
 * The name of a take-off item in the UI language. Item ids come from content/rules; some carry a
 * parameter: `concrete.b15`, `rebar.a500c.d12`, `lintel.bar.l22`.
 */
@Composable
internal fun itemName(item: String): String {
    val concrete = Regex("""concrete\.b(\d+)(?:_(\d+))?""").matchEntire(item)
    val rebar = Regex("""rebar\.a(\d+)c\.d(\d+)""").matchEntire(item)
    val lintel = Regex("""lintel\.bar\.l(\d+)""").matchEntire(item)
    return when {
        item == "masonry_unit" -> {
            stringResource(R.string.item_masonry_unit)
        }

        item == "mortar" -> {
            stringResource(R.string.item_mortar)
        }

        item == "mortar.cement" -> {
            stringResource(R.string.item_cement)
        }

        item == "mesh.junction" -> {
            stringResource(R.string.item_junction_mesh)
        }

        concrete != null -> {
            val (whole, part) = concrete.destructured
            stringResource(R.string.item_concrete, if (part.isEmpty()) whole else "$whole,$part")
        }

        rebar != null -> {
            stringResource(R.string.item_rebar, rebar.groupValues[1], rebar.groupValues[2])
        }

        lintel != null -> {
            stringResource(R.string.item_lintel, decimetres(lintel.groupValues[1]))
        }

        else -> {
            item
        }
    }
}

@Composable
internal fun unitName(unit: QuantityUnit): String =
    stringResource(
        when (unit) {
            QuantityUnit.PIECE -> R.string.unit_piece
            QuantityUnit.METRE -> R.string.unit_metre
            QuantityUnit.SQUARE_METRE -> R.string.unit_square_metre
            QuantityUnit.CUBIC_METRE -> R.string.unit_cubic_metre
            QuantityUnit.KILOGRAM -> R.string.unit_kilogram
        },
    )

/** `6 400`, `1,39`: a no-break space between thousands and a decimal comma, as builders write. */
internal fun amountText(line: QuantityLine): String {
    val digits =
        when (line.unit) {
            QuantityUnit.PIECE -> 0
            QuantityUnit.KILOGRAM -> 1
            QuantityUnit.METRE, QuantityUnit.SQUARE_METRE, QuantityUnit.CUBIC_METRE -> 2
        }
    val symbols =
        DecimalFormatSymbols(Locale.ROOT).apply {
            groupingSeparator = ' '
            decimalSeparator = ','
        }
    val pattern = if (digits == 0) "#,##0" else "#,##0." + "0".repeat(digits)
    return DecimalFormat(pattern, symbols).format(line.amount)
}

// "22" decimetres reads as "2,2" metres.
private fun decimetres(value: String): String {
    val number = value.toInt()
    return "${number / DECIMETRES_PER_METRE},${number % DECIMETRES_PER_METRE}"
}

private const val DECIMETRES_PER_METRE = 10
