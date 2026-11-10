package uz.otves.core.model

import com.google.common.truth.Truth.assertThat
import org.junit.Assert.assertThrows
import org.junit.Test
import java.math.BigDecimal

class MoneyTest {
    private val brick = Money(minor = 1_950_00, currency = Currency.UZS)

    @Test
    fun multipliesByWholeQuantities() {
        assertThat(brick * 2_351).isEqualTo(Money(4_584_450_00, Currency.UZS))
    }

    @Test
    fun multipliesByMeasuredQuantitiesExactlyAndRoundsHalfUp() {
        // 5,26 m³ at 650 000 sum; 0,5 of one tiyin rounds up.
        assertThat(Money(65_000_000, Currency.UZS) * BigDecimal("5.26")).isEqualTo(Money(341_900_000, Currency.UZS))
        assertThat(Money(1, Currency.UZS) * BigDecimal("0.5")).isEqualTo(Money(1, Currency.UZS))
    }

    @Test
    fun addsWithinOneCurrency() {
        assertThat(brick + brick).isEqualTo(Money(3_900_00, Currency.UZS))
        assertThat(Money.zero(Currency.UZS) + brick).isEqualTo(brick)
    }

    @Test
    fun rejectsMixedCurrencies() {
        assertThrows(IllegalArgumentException::class.java) {
            brick + Money(100, Currency.RUB)
        }
    }
}
