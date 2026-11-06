package uz.otves.core.model

/** ISO 4217 currencies the app prices in, with the number of minor-unit digits. */
public enum class Currency(
    public val minorDigits: Int,
) {
    UZS(minorDigits = 2),
    RUB(minorDigits = 2),
    USD(minorDigits = 2),
    EUR(minorDigits = 2),
}

/**
 * An amount in the currency's minor units (tiyin, kopeck, cent).
 *
 * Money never passes through floating point; mixing currencies is a programming error.
 */
public data class Money(
    public val minor: Long,
    public val currency: Currency,
) : Comparable<Money> {
    public operator fun plus(other: Money): Money = Money(minor + other.sameCurrency().minor, currency)

    public operator fun minus(other: Money): Money = Money(minor - other.sameCurrency().minor, currency)

    public operator fun times(quantity: Long): Money = Money(minor * quantity, currency)

    override fun compareTo(other: Money): Int = minor.compareTo(other.sameCurrency().minor)

    private fun Money.sameCurrency(): Money {
        require(currency == this@Money.currency) { "Cannot combine ${this@Money.currency} with $currency" }
        return this
    }

    public companion object {
        public fun zero(currency: Currency): Money = Money(0, currency)
    }
}
