package uz.otves.engine

import java.nio.ByteBuffer
import kotlin.uuid.Uuid

/**
 * One open job in the engine (docs/adr/0006). Not thread-safe: call it from the engine thread only,
 * and read each response before the next call, which reuses its memory.
 */
public class EngineSession(
    device: Uuid,
) : AutoCloseable {
    private var handle: Long = device.toLongs { high, low -> NativeEngine.create(high, low) }
    private var request: ByteBuffer = ByteBuffer.allocateDirect(INITIAL_CAPACITY)

    init {
        check(handle != 0L) { "The engine could not open a session" }
    }

    /** Runs [commands] in order; one that fails changes nothing and the rest still run. */
    public fun send(commands: List<EngineCommand>): EngineResponse = ResponseDecoder.decode(handle(RequestEncoder.encode(commands)))

    /** Sends the bytes of a schema `Request`; returns a read-only schema `Response`. */
    public fun handle(bytes: ByteBuffer): ByteBuffer {
        check(handle != 0L) { "The session is closed" }
        val size = bytes.remaining()
        if (request.capacity() < size) {
            request = ByteBuffer.allocateDirect(maxOf(size, request.capacity() * 2))
        }
        request.clear()
        request.put(bytes.duplicate())
        val response = checkNotNull(NativeEngine.handle(handle, request, size)) { "The engine rejected the request buffer" }
        return response.asReadOnlyBuffer()
    }

    override fun close() {
        if (handle != 0L) {
            NativeEngine.destroy(handle)
            handle = 0L
        }
    }

    private companion object {
        const val INITIAL_CAPACITY = 64 * 1024
    }
}
