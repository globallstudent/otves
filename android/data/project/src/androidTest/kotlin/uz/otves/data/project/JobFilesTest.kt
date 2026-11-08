package uz.otves.data.project

import androidx.test.ext.junit.runners.AndroidJUnit4
import androidx.test.platform.app.InstrumentationRegistry
import com.google.common.truth.Truth.assertThat
import kotlinx.coroutines.test.runTest
import org.junit.After
import org.junit.Test
import org.junit.runner.RunWith
import uz.otves.core.model.ElementId
import uz.otves.core.model.mm
import uz.otves.engine.ElementChange
import uz.otves.engine.EngineResponse
import uz.otves.engine.Level
import uz.otves.engine.LogEntry
import uz.otves.engine.Opening
import uz.otves.engine.PointMm
import uz.otves.engine.Wall
import java.io.File
import kotlin.uuid.Uuid

@RunWith(AndroidJUnit4::class)
class JobFilesTest {
    private val context = InstrumentationRegistry.getInstrumentation().targetContext
    private val directory = File(context.cacheDir, "jobs-test-${System.nanoTime()}")
    private var clock = 1_762_387_200_000L
    private val files = JobFiles(context, directory, now = { clock++ })

    private val level = ElementId(Uuid.parse("019a5f00-0000-7000-8000-000000000001"))
    private val wall = ElementId(Uuid.parse("019a5f00-0000-7000-8000-000000000002"))
    private val window = ElementId(Uuid.parse("019a5f00-0000-7000-8000-000000000003"))
    private val device = Uuid.parse("019a5f00-0000-7000-8000-0000000000dd")

    @After
    fun tearDown() {
        directory.deleteRecursively()
    }

    private fun entry(
        n: Long,
        hlc: Long,
    ) = LogEntry(Uuid.fromLongs(n, n), kind = 0, hlc = hlc, device = device, origin = null, ops = byteArrayOf(n.toByte()))

    private fun response(
        changes: List<ElementChange>,
        log: List<LogEntry>,
    ) = EngineResponse(results = emptyList(), changes = changes, canUndo = true, canRedo = false, log = log)

    @Test
    fun aNewJobIsListedNewestFirstAndOpensEmpty() =
        runTest {
            val first = files.create("Дом на Чиланзаре")
            val second = files.create("Баня")

            assertThat(files.list().map { it.name }).containsExactly("Баня", "Дом на Чиланзаре").inOrder()
            assertThat(first.id.toString()[14]).isEqualTo('7')
            files.open(second.id).use { job -> assertThat(job.load()).isEqualTo(SavedJob(emptyMap(), clock = 0L, name = "Баня")) }
        }

    @Test
    fun recordedChangesComeBackWhenTheJobOpensAgain() =
        runTest {
            val created = files.create("Дом")
            val storey = Level(0.mm, 3000.mm)
            val masonry = Wall(level, PointMm(0.mm, 0.mm), PointMm(6400.mm, 0.mm), 380.mm, 2800.mm)
            val glazing = Opening(wall, 1200.mm, 1500.mm, 1500.mm, 900.mm)

            files.open(created.id).use { job ->
                job.record(response(listOf(ElementChange(level, storey)), listOf(entry(1, hlc = 100))))
                job.record(
                    response(
                        listOf(ElementChange(wall, masonry), ElementChange(window, glazing)),
                        listOf(entry(2, hlc = 200), entry(3, hlc = 300)),
                    ),
                )
                job.record(response(listOf(ElementChange(window, null)), listOf(entry(4, hlc = 400))))
            }

            files.open(created.id).use { job ->
                assertThat(job.load()).isEqualTo(SavedJob(mapOf(level to storey, wall to masonry), clock = 400L, name = "Дом"))
            }
            assertThat(files.list().single().updatedAt).isGreaterThan(created.updatedAt)
        }

    @Test
    fun theLogKeepsItsOrderAndBytes() =
        runTest {
            val created = files.create("Забор")
            val job = files.open(created.id)
            job.record(response(emptyList(), listOf(entry(5, hlc = 500), entry(6, hlc = 600))))

            val stored = job.groups()
            job.close()

            assertThat(stored.map { it.hlc }).containsExactly(500L, 600L).inOrder()
            assertThat(stored.last().ops.toList()).containsExactly(6.toByte())
            assertThat(stored.first().origin).isNull()
        }
}
