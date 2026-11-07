# Dependencies built with ThreadSanitizer, matching the engine's desktop-tsan preset.
include(${CMAKE_CURRENT_LIST_DIR}/otves-host.cmake)

set(VCPKG_C_FLAGS "-fsanitize=thread")
set(VCPKG_CXX_FLAGS "-fsanitize=thread")
set(VCPKG_LINKER_FLAGS "-fsanitize=thread")
