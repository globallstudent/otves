package uz.otves.core.model

import com.google.common.truth.Truth.assertThat
import org.junit.Assert.assertThrows
import org.junit.Test

class MoneyTest {
    private val brick = Money(minor = 1_950_00, currency = Currency.UZS)

    @Test
    fun multipliesByWholeQuantities() {
        assertThat(brick * 2_351).isEqualTo(Money(4_584_450_00, Currency.UZS))
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
