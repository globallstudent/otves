package uz.otves.data.project

import com.google.common.truth.Truth.assertThat
import org.junit.Rule
import org.junit.Test
import org.junit.rules.TemporaryFolder
import uz.otves.core.model.Currency
import uz.otves.core.model.Money
import java.io.File

class JobCardFilesTest {
    @get:Rule
    val folder = TemporaryFolder()

    private val job get() = File(folder.root, "019a5f00-0000-7000-8000-000000000001.job")

    @Test
    fun aJobWithoutACardHasNone() {
        assertThat(JobCardFiles(job).read()).isNull()
    }

    @Test
    fun aCardIsReadAsWritten() {
        val png = byteArrayOf(1, 2, 3)

        JobCardFiles(job).write(area = 138.6, cost = Money(4_860_000_000, Currency.UZS), preview = png)
        val card = JobCardFiles(job).read()

        assertThat(card?.area).isEqualTo(138.6)
        assertThat(card?.cost).isEqualTo(Money(4_860_000_000, Currency.UZS))
        assertThat(card?.preview?.readBytes()).isEqualTo(png)
    }

    @Test
    fun aJobWithNothingPricedHasNoCost() {
        JobCardFiles(job).write(area = 24.0, cost = null, preview = null)
        val card = JobCardFiles(job).read()

        assertThat(card?.area).isEqualTo(24.0)
        assertThat(card?.cost).isNull()
        assertThat(card?.preview).isNull()
    }

    @Test
    fun aPlanThatLostItsWallsLosesItsPicture() {
        JobCardFiles(job).write(area = 24.0, cost = null, preview = byteArrayOf(1))

        JobCardFiles(job).write(area = 0.0, cost = null, preview = null)

        assertThat(JobCardFiles(job).read()?.preview).isNull()
    }

    @Test
    fun theCardLiesBesideTheJobUnderItsName() {
        JobCardFiles(job).write(area = 24.0, cost = null, preview = byteArrayOf(1))

        // Deleting a job removes every file that starts with its name.
        assertThat(folder.root.list()!!.toList()).containsExactly("${job.name}.card", "${job.name}.png")
    }

    @Test
    fun aCardOfANewerFormatIsLeftUnread() {
        File(job.path + ".card").writeBytes(byteArrayOf(0, 0, 0, 9, 1, 2, 3))

        assertThat(JobCardFiles(job).read()).isNull()
    }

    @Test
    fun aCutOffCardIsLeftUnread() {
        File(job.path + ".card").writeBytes(byteArrayOf(0, 0, 0, 1, 64))

        assertThat(JobCardFiles(job).read()).isNull()
    }
}
