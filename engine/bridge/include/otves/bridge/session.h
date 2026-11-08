#pragma once

#include <cstdint>
#include <memory>
#include <span>

#include <otves/core/uuid.h>

namespace otves::bridge {

// One open job for the app: its document and the buffer the engine answers with. Single-threaded:
// the app calls it from its engine thread only (docs/adr/0006).
class Session {
 public:
  explicit Session(DeviceId device);
  ~Session();
  Session(const Session&) = delete;
  Session& operator=(const Session&) = delete;
  Session(Session&&) = delete;
  Session& operator=(Session&&) = delete;

  // Runs a schema::Request buffer and returns a schema::Response buffer, valid until the next
  // call. Empty only if the engine could not even build an error response.
  [[nodiscard]] std::span<const std::uint8_t> handle(
      std::span<const std::uint8_t> request) noexcept;

 private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

}  // namespace otves::bridge
