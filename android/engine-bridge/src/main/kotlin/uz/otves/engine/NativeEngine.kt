package uz.otves.engine

import java.nio.ByteBuffer

/** The JNI functions of libdesign.so (engine/bridge/src/jni.cpp). */
internal object NativeEngine {
    init {
        System.loadLibrary("design")
    }

    /** A session handle for a device id, or 0 if the engine could not start one. */
    @JvmStatic
    external fun create(
        deviceHigh: Long,
        deviceLow: Long,
    ): Long

    @JvmStatic
    external fun destroy(session: Long)

    /**
     * Runs the first [size] bytes of a direct [request] buffer. Returns the response over engine
     * memory, valid until the session's next call, or null for a bad handle or buffer.
     */
    @JvmStatic
    external fun handle(
        session: Long,
        request: ByteBuffer,
        size: Int,
    ): ByteBuffer?
}
