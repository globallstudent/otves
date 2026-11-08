#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <string_view>

#include <otves/core/error.h>
#include <otves/takeoff/rules.h>

struct lua_State;

namespace otves::takeoff {

// Luau bytecode for one script, or kRuleSyntax with the compiler's message.
[[nodiscard]] Result<std::string> compile_luau(const Script& script);

struct SandboxLimits;

// One Luau state holding only deterministic libraries, with read-only globals, a memory cap and
// a step budget per script call (docs/adr/0005). Values pass on the Lua stack of state().
class Sandbox {
 public:
  static constexpr std::size_t kMemoryLimit = std::size_t{32} << 20U;  // bytes
  static constexpr std::int64_t kStepBudget = 1'000'000;  // loop back-edges and calls per script

  Sandbox();
  ~Sandbox();
  Sandbox(const Sandbox&) = delete;
  Sandbox& operator=(const Sandbox&) = delete;
  Sandbox(Sandbox&&) = delete;
  Sandbox& operator=(Sandbox&&) = delete;

  [[nodiscard]] lua_State* state() const { return state_; }

  // Runs `body` as a protected call on a fresh stack frame. Any Luau error it raises through the
  // stack API, such as running out of memory, becomes kRuleFailed instead of aborting.
  [[nodiscard]] Result<void> protect(std::function<void()>& body);

  // Runs a script's top level in its own thread and pushes its single result.
  [[nodiscard]] Result<void> load(std::string_view name, const std::string& bytecode);

  // Calls the function below the top `args` values and replaces them all with its result.
  [[nodiscard]] Result<void> call(std::string_view name, int args);

  // Checks that the value on top holds only tables, strings, numbers and booleans, and makes every
  // table in it read-only.
  [[nodiscard]] Result<void> freeze_data(std::string_view name);

 private:
  std::unique_ptr<SandboxLimits> limits_;  // a stable address for the Luau callbacks
  lua_State* state_ = nullptr;
};

}  // namespace otves::takeoff
