#pragma once

#include <cstdint>
#include <expected>
#include <string>
#include <utility>

namespace otves {

enum class ErrorCode : std::uint8_t {
  kInvalidArgument,
  kOutOfRange,
  kNotFound,
  kCorruptData,
  kUnsupportedVersion,
};

struct Error {
  ErrorCode code;
  std::string message;
};

// Every fallible engine operation returns Result; the engine does not throw.
template <typename T>
using Result = std::expected<T, Error>;

[[nodiscard]] inline std::unexpected<Error> make_error(ErrorCode code, std::string message) {
  return std::unexpected<Error>{Error{.code = code, .message = std::move(message)}};
}

}  // namespace otves
