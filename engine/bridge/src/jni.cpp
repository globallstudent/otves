// JNI entry points of libdesign.so for uz.otves.engine.NativeEngine. Kotlin holds a session as a
// jlong handle from create() until destroy() and calls it from one engine thread only.

#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>

#include <jni.h>

#include <otves/bridge/session.h>
#include <otves/core/uuid.h>

namespace {

otves::bridge::Session* session_of(jlong handle) {
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast,performance-no-int-to-ptr)
  return reinterpret_cast<otves::bridge::Session*>(handle);
}

}  // namespace

// The JVM finds native methods by these exact names.
// NOLINTBEGIN(readability-identifier-naming)
extern "C" {

JNIEXPORT jlong JNICALL Java_uz_otves_engine_NativeEngine_create(JNIEnv* /*env*/, jclass /*type*/,
                                                                 jlong device_high,
                                                                 jlong device_low) noexcept {
  try {
    const otves::DeviceId device{otves::Uuid{static_cast<std::uint64_t>(device_high),
                                             static_cast<std::uint64_t>(device_low)}};
    // Kotlin owns the session through the handle until destroy().
    auto session = std::make_unique<otves::bridge::Session>(device);
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    return reinterpret_cast<jlong>(session.release());
  } catch (...) {
    return 0;
  }
}

JNIEXPORT void JNICALL Java_uz_otves_engine_NativeEngine_destroy(JNIEnv* /*env*/, jclass /*type*/,
                                                                 jlong handle) noexcept {
  const std::unique_ptr<otves::bridge::Session> owned{session_of(handle)};
}

// Returns a direct, read-only-by-contract ByteBuffer over the session's response, valid until the
// session's next call, or null if the handle or the request buffer is invalid.
JNIEXPORT jobject JNICALL Java_uz_otves_engine_NativeEngine_handle(JNIEnv* env, jclass /*type*/,
                                                                   jlong handle, jobject request,
                                                                   jint size) noexcept {
  otves::bridge::Session* session = session_of(handle);
  const void* data = env->GetDirectBufferAddress(request);
  if (session == nullptr || data == nullptr || size < 0 ||
      env->GetDirectBufferCapacity(request) < size) {
    return nullptr;
  }
  const std::span<const std::uint8_t> response =
      session->handle({static_cast<const std::uint8_t*>(data), static_cast<std::size_t>(size)});
  if (response.empty()) {
    return nullptr;
  }
  // JNI takes a mutable pointer; Kotlin reads the buffer as read-only.
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast)
  void* bytes = const_cast<std::uint8_t*>(response.data());
  return env->NewDirectByteBuffer(bytes, static_cast<jlong>(response.size()));
}

}  // extern "C"
// NOLINTEND(readability-identifier-naming)
