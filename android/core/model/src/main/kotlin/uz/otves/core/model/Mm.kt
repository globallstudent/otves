package uz.otves.core.model

/**
 * A length typed or measured on site, in whole millimetres.
 *
 * Mirrors the engine's `otves::Mm`: integer storage keeps measured values exact.
 */
@JvmInline
public value class Mm(
    public val value: Long,
) : Comparable<Mm> {
    public operator fun plus(other: Mm): Mm = Mm(value + other.value)

    public operator fun minus(other: Mm): Mm = Mm(value - other.value)

    public operator fun unaryMinus(): Mm = Mm(-value)

    public operator fun times(factor: Long): Mm = Mm(value * factor)

    override fun compareTo(other: Mm): Int = value.compareTo(other.value)
}

public val Int.mm: Mm get() = Mm(toLong())
