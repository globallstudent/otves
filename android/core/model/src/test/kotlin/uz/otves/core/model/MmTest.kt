package uz.otves.core.model

import com.google.common.truth.Truth.assertThat
import org.junit.Test

class MmTest {
    @Test
    fun keepsMeasuredValuesExact() {
        assertThat(6400.mm + 600.mm).isEqualTo(7000.mm)
        assertThat(6400.mm - 6400.mm).isEqualTo(0.mm)
    }

    @Test
    fun scalesAndNegates() {
        assertThat(380.mm * 2).isEqualTo(760.mm)
        assertThat(-(120.mm)).isEqualTo(Mm(-120))
    }

    @Test
    fun ordersLikeIntegers() {
        assertThat(380.mm).isLessThan(510.mm)
    }
}
