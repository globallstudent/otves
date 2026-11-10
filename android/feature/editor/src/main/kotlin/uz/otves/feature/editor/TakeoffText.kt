package uz.otves.feature.editor

import android.content.res.Resources
import androidx.compose.runtime.Composable
import androidx.compose.ui.platform.LocalResources
import uz.otves.core.model.Currency
import uz.otves.core.model.Mm
import uz.otves.core.model.Money
import uz.otves.engine.MasonryUnitKind
import uz.otves.engine.MasonryUnitOption
import uz.otves.engine.QuantityLine
import uz.otves.engine.QuantityUnit
import uz.otves.engine.UnitSize
import java.math.BigDecimal
import java.math.RoundingMode
import java.text.DecimalFormat
import java.text.DecimalFormatSymbols
import java.util.Locale

/**
 * The name of a take-off item in the UI language. Item ids come from content/rules; some carry a
 * parameter: `concrete.b15`, `rebar.a500c.d12`, `lintel.bar.l22`.
 */
internal fun itemName(
    resources: Resources,
    item: String,
    units: List<MasonryUnitOption>,
): String {
    val bags = Regex("""mortar\.cement\.bag(\d+)""").matchEntire(item)
    val masonry = Regex("""masonry_unit\.(\d+)x(\d+)x(\d+)""").matchEntire(item)
    val concrete = Regex("""concrete\.b(\d+)(?:_(\d+))?""").matchEntire(item)
    val rebar = Regex("""rebar\.a(\d+)c\.d(\d+)""").matchEntire(item)
    val lintel = Regex("""lintel\.bar\.l(\d+)""").matchEntire(item)
    return when {
        masonry != null -> {
            val (length, width, height) = masonry.destructured
            unitTitle(resources, UnitSize(Mm(length.toLong()), Mm(width.toLong()), Mm(height.toLong())), units)
        }

        item == "mortar" -> {
            resources.getString(R.string.item_mortar)
        }

        item == "mortar.cement" -> {
            resources.getString(R.string.item_cement)
        }

        item == "mortar.sand" -> {
            resources.getString(R.string.item_sand)
        }

        bags != null -> {
            resources.getString(R.string.item_cement_bags, bags.groupValues[1])
        }

        item == "mesh.junction" -> {
            resources.getString(R.string.item_junction_mesh)
        }

        concrete != null -> {
            val (whole, part) = concrete.destructured
            resources.getString(R.string.item_concrete, if (part.isEmpty()) whole else "$whole,$part")
        }

        rebar != null -> {
            resources.getString(R.string.item_rebar, rebar.groupValues[1], rebar.groupValues[2])
        }

        lintel != null -> {
            resources.getString(R.string.item_lintel, decimetres(lintel.groupValues[1]))
        }

        else -> {
            item
        }
    }
}

/** A masonry unit as builders call it: «Кирпич 1НФ», or with its size when it has no mark. */
internal fun unitTitle(
    resources: Resources,
    size: UnitSize,
    units: List<MasonryUnitOption>,
): String {
    val option = units.find { it.size == size }
    val kind =
        resources.getString(
            when (option?.kind) {
                MasonryUnitKind.BRICK -> R.string.unit_brick
                MasonryUnitKind.STONE -> R.string.unit_stone
                MasonryUnitKind.BLOCK, null -> R.string.unit_block
            },
        )
    return if (option == null || option.mark.isEmpty()) "$kind ${unitSize(resources, size)}" else "$kind ${option.mark}"
}

/** Millimetres as laid: «250 × 120 × 65». */
internal fun unitSize(
    resources: Resources,
    size: UnitSize,
): String = resources.getString(R.string.unit_size, size.length.value, size.width.value, size.height.value)

internal fun unitName(
    resources: Resources,
    unit: QuantityUnit,
): String =
    resources.getString(
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
    val digits = shownDigits(line.unit)
    val symbols =
        DecimalFormatSymbols(Locale.ROOT).apply {
            groupingSeparator = ' '
            decimalSeparator = ','
        }
    val pattern = if (digits == 0) "#,##0" else "#,##0." + "0".repeat(digits)
    return DecimalFormat(pattern, symbols).format(line.amount)
}

private fun shownDigits(unit: QuantityUnit): Int =
    when (unit) {
        QuantityUnit.PIECE -> 0
        QuantityUnit.KILOGRAM -> 1
        QuantityUnit.METRE, QuantityUnit.SQUARE_METRE, QuantityUnit.CUBIC_METRE -> 2
    }

/**
 * The lines the builder buys. A line whose item other lines continue after a dot is made of them
 * (`mortar` of `mortar.cement` and `mortar.sand`), so the parts are bought and priced, not the whole.
 */
internal fun bought(lines: List<QuantityLine>): List<QuantityLine> =
    lines.filter { line -> lines.none { it.item.startsWith(line.item + ".") } }

/** What the line costs at the builder's price: the amount as it is shown, times the price of one. */
internal fun cost(
    line: QuantityLine,
    prices: Map<String, Money>,
): Money? = prices[line.item]?.let { it * BigDecimal.valueOf(line.amount).setScale(shownDigits(line.unit), RoundingMode.HALF_UP) }

/** The sum of the priced lines the builder buys, or null when none has a price or their currencies differ. */
internal fun total(
    lines: List<QuantityLine>,
    prices: Map<String, Money>,
): Money? {
    val costs = bought(lines).mapNotNull { cost(it, prices) }
    if (costs.isEmpty() || costs.any { it.currency != costs.first().currency }) return null
    return costs.reduce(Money::plus)
}

/** Whole units of the currency with its name: «12 534 000 сум». */
internal fun moneyText(
    resources: Resources,
    money: Money,
): String {
    val whole = BigDecimal.valueOf(money.minor).movePointLeft(money.currency.minorDigits).setScale(0, RoundingMode.HALF_UP)
    val grouped = String.format(Locale.ROOT, "%,d", whole.toLong()).replace(',', '\u00A0')
    return if (money.currency == Currency.UZS) resources.getString(R.string.money_uzs, grouped) else "$grouped ${money.currency.name}"
}

// "22" decimetres reads as "2,2" metres.
private fun decimetres(value: String): String {
    val number = value.toInt()
    return "${number / DECIMETRES_PER_METRE},${number % DECIMETRES_PER_METRE}"
}

private const val DECIMETRES_PER_METRE = 10

// The same texts inside a composition, in its resources.

@Composable
internal fun itemName(
    item: String,
    units: List<MasonryUnitOption>,
): String = itemName(LocalResources.current, item, units)

@Composable
internal fun unitTitle(
    size: UnitSize,
    units: List<MasonryUnitOption>,
): String = unitTitle(LocalResources.current, size, units)

@Composable
internal fun unitSize(size: UnitSize): String = unitSize(LocalResources.current, size)

@Composable
internal fun unitName(unit: QuantityUnit): String = unitName(LocalResources.current, unit)

@Composable
internal fun moneyText(money: Money): String = moneyText(LocalResources.current, money)
