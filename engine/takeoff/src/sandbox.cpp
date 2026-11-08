#include "sandbox.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <Luau/Compiler.h>
#include <lua.h>
#include <lualib.h>

#include <otves/core/assert.h>
#include <otves/core/error.h>
#include <otves/takeoff/rules.h>

namespace otves::takeoff {

struct SandboxLimits {
  std::size_t used = 0;
  std::int64_t steps_left = Sandbox::kStepBudget;
};

namespace {

// Globals a script may use; everything else the opened libraries define is removed.
constexpr std::array<std::string_view, 21> kGlobals{
    "_G",           "_VERSION", "assert",   "error",    "getmetatable", "ipairs", "math",
    "next",         "pairs",    "rawequal", "rawget",   "rawlen",       "rawset", "select",
    "setmetatable", "string",   "table",    "tonumber", "tostring",     "type",   "typeof"};

// math members whose results are exact or correctly rounded on every platform. The others call
// libm, whose last bit differs between arm64 and x86-64, or the clock-seeded generator.
constexpr std::array<std::string_view, 16> kMath{
    "abs",   "ceil", "clamp", "floor", "fmod", "huge",  "isfinite", "isinf",
    "isnan", "max",  "min",   "modf",  "pi",   "round", "sign",     "sqrt"};

constexpr int kMaxDepth = 32;

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters): Luau's lua_Alloc signature
void* allocate(void* data, void* block, std::size_t old_size, std::size_t new_size) {
  auto& limits = *static_cast<SandboxLimits*>(data);
  // Luau's allocator contract is realloc and free.
  if (new_size == 0) {
    std::free(block);  // NOLINT(cppcoreguidelines-no-malloc,cppcoreguidelines-owning-memory)
    limits.used -= old_size;
    return nullptr;
  }
  if (new_size > old_size && new_size - old_size > Sandbox::kMemoryLimit - limits.used) {
    return nullptr;
  }
  // NOLINTNEXTLINE(cppcoreguidelines-no-malloc,cppcoreguidelines-owning-memory)
  void* moved = std::realloc(block, new_size);
  if (moved != nullptr) {
    limits.used = limits.used - old_size + new_size;
  }
  return moved;
}

void interrupt(lua_State* state, int gc) {
  if (gc >= 0) {
    return;  // a collector step, not script progress
  }
  auto& limits = *static_cast<SandboxLimits*>(lua_callbacks(state)->userdata);
  if (--limits.steps_left < 0) {
    luaL_where(state, 0);  // the script function the interrupt stopped
    lua_pushliteral(state, "ran out of steps");
    lua_concat(state, 2);
    lua_error(state);
  }
}

template <std::size_t N>
void keep_only(lua_State* state, int table, const std::array<std::string_view, N>& keep) {
  std::vector<std::string> removed;
  lua_pushnil(state);
  while (lua_next(state, table) != 0) {
    lua_pop(state, 1);
    std::size_t length = 0;
    const char* key = lua_type(state, -1) == LUA_TSTRING ? lua_tolstring(state, -1, &length) : "";
    const std::string_view name{key, length};
    if (std::ranges::find(keep, name) == keep.end()) {
      removed.emplace_back(name);
    }
  }
  for (const std::string& name : removed) {
    lua_pushnil(state);
    lua_setfield(state, table, name.c_str());
  }
}

void open_libraries(lua_State* state) {
  const std::array<std::pair<const char*, lua_CFunction>, 4> libraries{{
      {"", luaopen_base},
      {LUA_TABLIBNAME, luaopen_table},
      {LUA_STRLIBNAME, luaopen_string},
      {LUA_MATHLIBNAME, luaopen_math},
  }};
  for (const auto& [name, open] : libraries) {
    lua_pushcfunction(state, open, nullptr);
    lua_pushstring(state, name);
    lua_call(state, 1, 0);
  }
  keep_only(state, LUA_GLOBALSINDEX, kGlobals);
  lua_getglobal(state, LUA_MATHLIBNAME);
  keep_only(state, lua_gettop(state), kMath);
  lua_pop(state, 1);
}

// The message on top of the stack, starting with the script name.
std::string error_text(lua_State* state, std::string_view name) {
  std::string text;
  if (lua_type(state, -1) == LUA_TSTRING) {
    std::size_t length = 0;
    const char* message = lua_tolstring(state, -1, &length);
    text.assign(message, length);
  } else {
    text = std::string{"error object is a "} + luaL_typename(state, -1);
  }
  const std::string prefix = std::string{name} + ":";
  return text.starts_with(prefix) ? text : prefix + " " + text;
}

// Recursion stops at kMaxDepth.
// NOLINTNEXTLINE(misc-no-recursion)
Result<void> freeze(lua_State* state, std::string_view name, int depth) {
  const int type = lua_type(state, -1);
  if (type == LUA_TNUMBER || type == LUA_TSTRING || type == LUA_TBOOLEAN) {
    return {};
  }
  const auto fail = [name](std::string_view what) {
    return make_error(ErrorCode::kRuleOutput, std::string{name} + ": " + std::string{what});
  };
  if (type != LUA_TTABLE) {
    return fail(std::string{"data holds a "} + lua_typename(state, type));
  }
  if (lua_getreadonly(state, -1) != 0) {
    return {};  // already checked: a shared or cyclic table
  }
  if (depth >= kMaxDepth || lua_checkstack(state, 2) == 0) {
    return fail("data nests too deep");
  }
  if (lua_getmetatable(state, -1) != 0) {
    lua_pop(state, 1);
    return fail("data has a metatable");
  }
  lua_setreadonly(state, -1, 1);
  lua_pushnil(state);
  while (lua_next(state, -2) != 0) {
    const int key = lua_type(state, -2);
    Result<void> frozen =
        key == LUA_TSTRING || key == LUA_TNUMBER
            ? freeze(state, name, depth + 1)
            : fail(std::string{"data has a "} + lua_typename(state, key) + " key");
    if (!frozen) {
      lua_pop(state, 2);
      return frozen;
    }
    lua_pop(state, 1);
  }
  return {};
}

}  // namespace

Result<std::string> compile_luau(const Script& script) {
  Luau::CompileOptions options;
  // Level 0 emits no builtin fast paths. At level 1, math.sin(x) would still run in a safe
  // environment even though the sandbox removed math.sin.
  options.optimizationLevel = 0;
  options.debugLevel = 1;
  std::string bytecode = Luau::compile(script.source, options);
  // A failed compile is a zero byte followed by ":line: message".
  if (!bytecode.empty() && bytecode.front() == '\0') {
    return make_error(ErrorCode::kRuleSyntax, script.name + bytecode.substr(1));
  }
  return bytecode;
}

Sandbox::Sandbox()
    : limits_(std::make_unique<SandboxLimits>()), state_(lua_newstate(allocate, limits_.get())) {
  OTVES_ASSERT(state_ != nullptr);
  lua_Callbacks* callbacks = lua_callbacks(state_);
  callbacks->userdata = limits_.get();
  callbacks->interrupt = interrupt;
  open_libraries(state_);
  luaL_sandbox(state_);
}

Sandbox::~Sandbox() {
  if (state_ != nullptr) {
    lua_close(state_);
  }
}

Result<void> Sandbox::protect(std::function<void()>& body) {
  const lua_CFunction trampoline = [](lua_State* state) -> int {
    const auto& run = *static_cast<std::function<void()>*>(lua_touserdata(state, 1));
    lua_remove(state, 1);
    run();
    return 0;
  };
  lua_pushcfunction(state_, trampoline, "engine");
  lua_pushlightuserdata(state_, &body);
  if (lua_pcall(state_, 1, 0, 0) != LUA_OK) {
    auto error = make_error(ErrorCode::kRuleFailed, error_text(state_, "engine"));
    lua_pop(state_, 1);
    return error;
  }
  return {};
}

Result<void> Sandbox::load(std::string_view name, const std::string& bytecode) {
  lua_State* thread = lua_newthread(state_);
  luaL_sandboxthread(thread);
  const std::string chunk = "=" + std::string{name};
  if (luau_load(thread, chunk.c_str(), bytecode.data(), bytecode.size(), 0) != 0) {
    auto error = make_error(ErrorCode::kRuleSyntax, error_text(thread, name));
    lua_pop(state_, 1);
    return error;
  }
  limits_->steps_left = kStepBudget;
  const int status = lua_pcall(thread, 0, 1, 0);
  limits_->steps_left = kStepBudget;  // a spent budget must not fail the caller's frames
  if (status != LUA_OK) {
    auto error = make_error(ErrorCode::kRuleFailed, error_text(thread, name));
    lua_pop(state_, 1);
    return error;
  }
  lua_xmove(thread, state_, 1);
  lua_remove(state_, -2);
  return {};
}

Result<void> Sandbox::call(std::string_view name, int args) {
  limits_->steps_left = kStepBudget;
  const int status = lua_pcall(state_, args, 1, 0);
  limits_->steps_left = kStepBudget;
  if (status != LUA_OK) {
    auto error = make_error(ErrorCode::kRuleFailed, error_text(state_, name));
    lua_pop(state_, 1);
    return error;
  }
  return {};
}

Result<void> Sandbox::freeze_data(std::string_view name) { return freeze(state_, name, 0); }

}  // namespace otves::takeoff
