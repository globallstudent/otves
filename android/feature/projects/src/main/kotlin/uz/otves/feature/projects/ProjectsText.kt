package uz.otves.feature.projects

import androidx.compose.runtime.Composable
import androidx.compose.ui.res.stringResource
import uz.otves.core.model.Currency
import uz.otves.core.model.Money
import uz.otves.data.project.JobSummary
import java.math.BigDecimal
import java.math.RoundingMode
import java.time.Instant
import java.time.ZoneId
import java.time.ZonedDateTime
import java.time.format.DateTimeFormatter
import java.util.Locale

/** The job has been edited and has no walls yet. */
internal val JobSummary.isBlank: Boolean
    get() = card.let { it != null && it.preview == null }

internal enum class Day { Today, Yesterday, Earlier }

/** When a job changed, as a row says it: the day, the time on it and the date. */
internal data class Changed(
    val day: Day,
    val time: String,
    val date: String,
)

/** [updatedAt] in Unix milliseconds, seen from [now]; the year is written only when it is not this one. */
internal fun changed(
    updatedAt: Long,
    locale: Locale,
    now: ZonedDateTime = ZonedDateTime.now(),
): Changed {
    val at = Instant.ofEpochMilli(updatedAt).atZone(now.zone)
    val day =
        when (at.toLocalDate()) {
            now.toLocalDate() -> Day.Today
            now.toLocalDate().minusDays(1) -> Day.Yesterday
            else -> Day.Earlier
        }
    val pattern = if (at.year == now.year) "d MMM" else "d MMM yyyy"
    return Changed(day, at.format(DateTimeFormatter.ofPattern("HH:mm", locale)), at.format(DateTimeFormatter.ofPattern(pattern, locale)))
}

/**
 * A sum short enough for a row: millions with one decimal from a million up, whole units with
 * spaces between thousands below. The flag says which.
 */
internal fun shortSum(
    money: Money,
    locale: Locale,
): Pair<String, Boolean> {
    val whole = BigDecimal.valueOf(money.minor).movePointLeft(money.currency.minorDigits)
    if (whole < MILLION) {
        val grouped = String.format(Locale.ROOT, "%,d", whole.setScale(0, RoundingMode.HALF_UP).toLong())
        return grouped.replace(',', ' ') to false
    }
    return String.format(locale, "%.1f", whole.movePointLeft(MILLION_DIGITS).setScale(1, RoundingMode.HALF_UP)) to true
}

private val MILLION = BigDecimal.valueOf(1_000_000)
private const val MILLION_DIGITS = 6

// Below this the walls close no room worth a number, m².
private const val LEAST_AREA = 0.05

// The language of the strings, not of the phone: Russian text gets Russian numbers and dates.
@Composable
private fun stringsLocale(): Locale = Locale.forLanguageTag(stringResource(R.string.projects_date_locale))

/** «138,6 м²», or null when the job's walls close no room. */
@Composable
internal fun areaText(job: JobSummary): String? {
    val area = job.card?.area?.takeIf { it >= LEAST_AREA } ?: return null
    return stringResource(R.string.projects_area, String.format(stringsLocale(), "%.1f", area))
}

/** «48,6 млн сум», or null when nothing in the job is priced. */
@Composable
internal fun costText(job: JobSummary): String? {
    val cost = job.card?.cost ?: return null
    val (sum, millions) = shortSum(cost, stringsLocale())
    return when {
        cost.currency != Currency.UZS -> "$sum ${cost.currency.name}"
        millions -> stringResource(R.string.projects_money_millions, sum)
        else -> stringResource(R.string.projects_money, sum)
    }
}

/** «Изменён сегодня в 16:20». */
@Composable
internal fun changedText(updatedAt: Long): String {
    val changed = changed(updatedAt, stringsLocale())
    return when (changed.day) {
        Day.Today -> stringResource(R.string.projects_changed_today, changed.time)
        Day.Yesterday -> stringResource(R.string.projects_changed_yesterday, changed.time)
        Day.Earlier -> stringResource(R.string.projects_changed, changed.date)
    }
}

/** One line for a row: «24,0 м², 9,8 млн сум. Вчера в 19:05». */
@Composable
internal fun summaryText(job: JobSummary): String {
    val changed = changed(job.updatedAt, stringsLocale())
    val area = areaText(job)
    val cost = costText(job)
    val facts = area ?: cost ?: if (job.isBlank) stringResource(R.string.projects_plan_empty) else return changedText(job.updatedAt)
    val day =
        when (changed.day) {
            Day.Today -> stringResource(R.string.projects_today, changed.time)
            Day.Yesterday -> stringResource(R.string.projects_yesterday, changed.time)
            Day.Earlier -> changed.date
        }
    return if (area != null && cost != null) {
        stringResource(R.string.projects_summary_two, area, cost, day)
    } else {
        stringResource(R.string.projects_summary_one, facts, day)
    }
}
