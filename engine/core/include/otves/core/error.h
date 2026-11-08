#pragma once

#include <cstdint>
#include <expected>
#include <string>
#include <utility>

namespace otves {

// One code per cause, so the app can show translated text for it. `message` is English context
// for logs and tests and never reaches the screen.
enum class ErrorCode : std::uint8_t {
  kNotFound,
  kOffPlan,  // a point lies more than kMaxCoordinate from the origin
  kZeroLength,
  kThicknessNotPositive,
  kHeightNotPositive,
  kWidthNotPositive,
  kOffsetNegative,  // an opening's distance from its wall's start
  kSillNegative,
  kUnitSizeNotPositive,  // a brick or block dimension
  kJointNegative,        // a mortar joint thickness
  kRuleSyntax,           // a content script does not compile or has a bad name
  kRuleFailed,           // a content script raised an error or ran out of steps or memory
  kRuleOutput,           // a content script returned values of the wrong shape
  kGestureOpen,          // undo or redo while a gesture is open
  kNothingToUndo,
  kNothingToRedo,
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
