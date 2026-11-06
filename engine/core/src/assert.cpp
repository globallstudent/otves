#include <algorithm>
#include <array>
#include <charconv>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <iterator>
#include <memory>
#include <source_location>
#include <span>
#include <string_view>

#include <otves/core/assert.h>

#ifdef __ANDROID__
#include <android/log.h>
#else
#include <cstdio>
#endif

namespace otves::detail {
namespace {

// Fixed-size, non-allocating message buffer: the assert path must work even when the
// heap is the thing that broke. Long messages are truncated. The buffer starts zeroed and
// is only ever filled forwards, so the text is always null-terminated.
class Message {
 public:
  void append(std::string_view text) noexcept {
    const std::span<char> free = std::span{buffer_}.subspan(size_, kCapacity - 1 - size_);
    const std::size_t count = std::min(text.size(), free.size());
    std::ranges::copy_n(text.begin(), static_cast<std::ptrdiff_t>(count), free.begin());
    size_ += count;
  }

  void append(std::uint_least32_t number) noexcept {
    std::array<char, kMaxDigits> digits{};
    const auto result =
        std::to_chars(std::to_address(digits.begin()), std::to_address(digits.end()), number);
    const auto length = std::distance(std::to_address(digits.begin()), result.ptr);
    append(std::string_view{digits.data(), static_cast<std::size_t>(length)});
  }

  [[nodiscard]] const char* c_str() const noexcept { return buffer_.data(); }

 private:
  static constexpr std::size_t kCapacity = 512;
  static constexpr std::size_t kMaxDigits = 16;

  std::array<char, kCapacity> buffer_{};
  std::size_t size_ = 0;
};

}  // namespace

void assert_failed(const char* expression, std::source_location location) noexcept {
  Message message;
  message.append("Assertion failed: ");
  message.append(expression);
  message.append(" at ");
  message.append(location.file_name());
  message.append(":");
  message.append(location.line());
#ifdef __ANDROID__
  __android_log_write(ANDROID_LOG_ERROR, "otves", message.c_str());
#else
  std::fputs(message.c_str(), stderr);
  std::fputc('\n', stderr);
#endif
#ifndef NDEBUG
  std::abort();
#endif
}

}  // namespace otves::detail
