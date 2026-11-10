package uz.otves.data.catalog

import androidx.test.ext.junit.runners.AndroidJUnit4
import androidx.test.platform.app.InstrumentationRegistry
import com.google.common.truth.Truth.assertThat
import kotlinx.coroutines.flow.first
import kotlinx.coroutines.test.runTest
import org.junit.After
import org.junit.Test
import org.junit.runner.RunWith
import uz.otves.core.model.Currency
import uz.otves.core.model.Money

@RunWith(AndroidJUnit4::class)
class CatalogPricesTest {
    private val context = InstrumentationRegistry.getInstrumentation().targetContext
    private val name = "catalog-test-${System.nanoTime()}.db"
    private val prices = CatalogPrices(context, name, now = { 1_762_387_200_000L })

    @After
    fun tearDown() {
        prices.close()
        context.deleteDatabase(name)
    }

    @Test
    fun aPriceIsKeptChangedAndForgotten() =
        runTest {
            assertThat(prices.all.first()).isEmpty()

            prices.set("masonry_unit.250x120x65", Money(145_000, Currency.UZS))
            prices.set("mortar", Money(65_000_000, Currency.UZS))
            prices.set("masonry_unit.250x120x65", Money(150_000, Currency.UZS))
            assertThat(prices.all.first())
                .containsExactly("masonry_unit.250x120x65", Money(150_000, Currency.UZS), "mortar", Money(65_000_000, Currency.UZS))

            prices.set("mortar", null)
            assertThat(prices.all.first().keys).containsExactly("masonry_unit.250x120x65")
        }
}
