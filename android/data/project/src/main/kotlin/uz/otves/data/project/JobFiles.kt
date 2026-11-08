package uz.otves.data.project

import android.content.Context
import androidx.room.Room
import androidx.sqlite.driver.bundled.BundledSQLiteDriver
import kotlinx.coroutines.CoroutineDispatcher
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.withContext
import uz.otves.core.model.ElementId
import uz.otves.engine.EngineElement
import uz.otves.engine.EngineResponse
import uz.otves.engine.Level
import uz.otves.engine.Opening
import uz.otves.engine.Wall
import java.io.File
import kotlin.random.Random
import kotlin.uuid.Uuid

/** A job on this phone. */
public data class JobSummary(
    public val id: Uuid,
    public val name: String,
    /** Unix milliseconds of the last change. */
    public val updatedAt: Long,
)

/** A saved job as the engine opens it: its elements and the newest stamp of its log. */
public data class SavedJob(
    public val elements: Map<ElementId, EngineElement>,
    public val clock: Long,
)

/** Where an open job lives while it is edited. */
public interface JobStore {
    public suspend fun load(): SavedJob

    /** Writes the response's element changes and log groups in one transaction. */
    public suspend fun record(response: EngineResponse)
}

/** The job files on this phone, one SQLite file each under [directory] (docs/adr/0007). */
public class JobFiles(
    context: Context,
    private val directory: File = File(context.filesDir, "jobs"),
    private val now: () -> Long = System::currentTimeMillis,
    private val io: CoroutineDispatcher = Dispatchers.IO,
) {
    private val context = context.applicationContext

    /** Newest change first. A file this app cannot read is left out. */
    public suspend fun list(): List<JobSummary> =
        withContext(io) {
            directory
                .listFiles { file -> file.extension == EXTENSION }
                .orEmpty()
                .mapNotNull { file -> summary(file) }
                .sortedByDescending { it.updatedAt }
        }

    public suspend fun create(name: String): JobSummary =
        withContext(io) {
            directory.mkdirs()
            val created = now()
            val job = JobRow(uuidV7(created).toString(), name, createdAt = created, updatedAt = created)
            database(file(job.id)).using { it.jobs().insertJob(job) }
            JobSummary(Uuid.parse(job.id), name, created)
        }

    /** The job for editing; close it when done. */
    public fun open(id: Uuid): OpenJob = OpenJob(database(file(id.toString())), now)

    private suspend fun summary(file: File): JobSummary? =
        database(file).using { database ->
            database.jobs().job()?.let { JobSummary(Uuid.parse(it.id), it.name, it.updatedAt) }
        }

    private fun file(id: String): File = File(directory, "$id.$EXTENSION")

    private fun database(file: File): ProjectDatabase =
        Room
            .databaseBuilder<ProjectDatabase>(context, file.absolutePath)
            .setDriver(BundledSQLiteDriver())
            .setQueryCoroutineContext(io)
            .build()

    private companion object {
        const val EXTENSION = "job"
    }
}

private inline fun <R> ProjectDatabase.using(block: (ProjectDatabase) -> R): R =
    try {
        block(this)
    } finally {
        close()
    }

// UUIDv7 (RFC 9562): Unix milliseconds, then random bits, so job ids sort by creation time.
internal fun uuidV7(
    unixMs: Long,
    random: Random = Random.Default,
): Uuid {
    val high = (unixMs shl TIMESTAMP_SHIFT) or VERSION_7 or (random.nextLong() and RANDOM_A)
    val low = (random.nextLong() and RANDOM_B) or VARIANT
    return Uuid.fromLongs(high, low)
}

private const val TIMESTAMP_SHIFT = 16
private const val VERSION_7 = 0x7000L
private const val RANDOM_A = 0x0FFFL
private const val RANDOM_B = 0x3FFF_FFFF_FFFF_FFFFL
private const val VARIANT = Long.MIN_VALUE

/** One job file open for editing. */
public class OpenJob internal constructor(
    private val database: ProjectDatabase,
    private val now: () -> Long,
) : JobStore,
    AutoCloseable {
    override suspend fun load(): SavedJob {
        val jobs = database.jobs()
        val elements =
            buildMap {
                jobs.levels().forEach { this += it.element() }
                jobs.walls().forEach { this += it.element() }
                jobs.openings().forEach { this += it.element() }
            }
        return SavedJob(elements, clock = jobs.newestStamp() ?: 0L)
    }

    override suspend fun record(response: EngineResponse) {
        if (response.changes.isEmpty() && response.log.isEmpty()) {
            return
        }
        val changed = response.changes.filter { it.element != null }
        val recorded =
            Recorded(
                levels = changed.mapNotNull { change -> (change.element as? Level)?.row(change.id) },
                walls = changed.mapNotNull { change -> (change.element as? Wall)?.row(change.id) },
                openings = changed.mapNotNull { change -> (change.element as? Opening)?.row(change.id) },
                deleted = response.changes.filter { it.element == null }.map { it.id.key() },
                groups = response.log.map { it.row() },
            )
        database.jobs().record(recorded, now())
    }

    internal suspend fun groups(): List<OpGroupRow> = database.jobs().groups()

    override fun close(): Unit = database.close()
}
