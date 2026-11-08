package uz.otves.engine

import kotlinx.coroutines.asCoroutineDispatcher
import kotlinx.coroutines.withContext
import java.util.concurrent.Executors
import kotlin.uuid.Uuid

/**
 * The app's one door to the engine: a session that lives on its own engine thread
 * (docs/adr/0006). Every call hops to that thread, so callers may use any dispatcher.
 */
public class EngineGateway(
    device: Uuid,
) : Engine,
    AutoCloseable {
    private val executor = Executors.newSingleThreadExecutor { task -> Thread(task, "otves-engine") }
    private val engineThread = executor.asCoroutineDispatcher()
    private val session = EngineSession(device)

    override suspend fun send(commands: List<EngineCommand>): EngineResponse = withContext(engineThread) { session.send(commands) }

    override fun close() {
        executor.execute { session.close() }
        executor.shutdown()
    }
}
