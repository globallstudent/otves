package uz.otves.feature.projects

import com.google.common.truth.Truth.assertThat
import org.junit.Test
import uz.otves.core.model.Currency
import uz.otves.core.model.Money
import java.time.ZoneId
import java.time.ZonedDateTime
import java.util.Locale

class ProjectsTextTest {
    private val russian = Locale.forLanguageTag("ru")
    private val tashkent = ZoneId.of("Asia/Tashkent")
    private val now = ZonedDateTime.of(2026, 11, 27, 17, 0, 0, 0, tashkent)

    private fun at(
        month: Int,
        day: Int,
        hour: Int,
        minute: Int,
        year: Int = 2026,
    ) = ZonedDateTime.of(year, month, day, hour, minute, 0, 0, tashkent).toInstant().toEpochMilli()

    @Test
    fun aChangeTodayIsSaidByItsTime() {
        val changed = changed(at(11, 27, 16, 20), russian, now)

        assertThat(changed.day).isEqualTo(Day.Today)
        assertThat(changed.time).isEqualTo("16:20")
    }

    @Test
    fun aChangeLateYesterdayIsYesterdayEvenWithinADay() {
        assertThat(changed(at(11, 26, 19, 5), russian, now).day).isEqualTo(Day.Yesterday)
        assertThat(changed(at(11, 26, 0, 0), russian, now).day).isEqualTo(Day.Yesterday)
    }

    @Test
    fun anEarlierChangeIsSaidByItsDate() {
        val changed = changed(at(11, 24, 9, 0), russian, now)

        assertThat(changed.day).isEqualTo(Day.Earlier)
        assertThat(changed.date).isEqualTo("24 нояб.")
    }

    @Test
    fun aChangeInAnotherYearNamesTheYear() {
        assertThat(changed(at(12, 30, 9, 0, year = 2025), russian, now).date).isEqualTo("30 дек. 2025")
    }

    @Test
    fun aSumFromAMillionUpIsShortenedToMillions() {
        // 48 600 000 sum, in tiyin.
        assertThat(shortSum(Money(4_860_000_000, Currency.UZS), russian)).isEqualTo("48,6" to true)
        assertThat(shortSum(Money(100_000_000, Currency.UZS), russian)).isEqualTo("1,0" to true)
        assertThat(shortSum(Money(1_234_560_000_000, Currency.UZS), russian)).isEqualTo("12345,6" to true)
    }

    @Test
    fun aSumBelowAMillionIsWrittenWhole() {
        assertThat(shortSum(Money(98_000_050, Currency.UZS), russian)).isEqualTo("980 001" to false)
        assertThat(shortSum(Money(0, Currency.UZS), russian)).isEqualTo("0" to false)
    }
}
