#pragma once

#include <source_location>

namespace otves::detail {

// Reports a broken invariant. Debug builds abort; release builds log and return so the
// engine can recover at the next command boundary. Never use it to validate user input.
void assert_failed(const char* expression, std::source_location location) noexcept;

}  // namespace otves::detail

// A macro, because the check needs the expression text and must vanish into one branch.
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define OTVES_ASSERT(condition)       \
  ((condition) ? static_cast<void>(0) \
               : ::otves::detail::assert_failed(#condition, std::source_location::current()))
