package uz.otves.data.project

import uz.otves.core.model.Currency
import uz.otves.core.model.Money
import java.io.DataInputStream
import java.io.DataOutputStream
import java.io.File
import java.io.IOException

/**
 * What the jobs list shows of a job without opening it. The editor derives it from the job and
 * keeps it beside the job file, so losing it loses nothing: it is written again on the next edit.
 */
public data class JobCard(
    /** The floor area of the job's rooms in square metres; zero when its walls close none. */
    public val area: Double,
    /** What the take-off costs at the builder's prices; null when nothing is priced. */
    public val cost: Money?,
    /** A PNG of the plan last worked on; null for a job with no walls. */
    public val preview: File?,
)

// Two files beside `<id>.job`: `<id>.job.card` with the numbers and `<id>.job.png` with the plan.
internal class JobCardFiles(
    job: File,
) {
    private val numbers = File(job.path + ".card")
    private val picture = File(job.path + ".png")

    /** Null when the job has no card yet or it was written by a newer app. */
    fun read(): JobCard? {
        if (!numbers.exists()) return null
        return try {
            DataInputStream(numbers.inputStream().buffered()).use { input ->
                if (input.readInt() != FORMAT) return null
                val area = input.readDouble()
                val cost = if (input.readBoolean()) Money(input.readLong(), Currency.valueOf(input.readUTF())) else null
                JobCard(area, cost, picture.takeIf { it.exists() })
            }
        } catch (_: IOException) {
            null
        } catch (_: IllegalArgumentException) {
            // A currency this version does not know.
            null
        }
    }

    fun write(
        area: Double,
        cost: Money?,
        preview: ByteArray?,
    ) {
        replace(numbers) { file ->
            DataOutputStream(file.outputStream().buffered()).use { output ->
                output.writeInt(FORMAT)
                output.writeDouble(area)
                output.writeBoolean(cost != null)
                if (cost != null) {
                    output.writeLong(cost.minor)
                    output.writeUTF(cost.currency.name)
                }
            }
        }
        if (preview == null) picture.delete() else replace(picture) { it.writeBytes(preview) }
    }

    // A reader never sees half a file: it is written beside its place and moved there.
    private fun replace(
        target: File,
        write: (File) -> Unit,
    ) {
        val fresh = File(target.path + ".new")
        write(fresh)
        if (!fresh.renameTo(target)) throw IOException("cannot replace ${target.name}")
    }

    private companion object {
        const val FORMAT = 1
    }
}
