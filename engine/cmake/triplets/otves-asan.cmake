# Dependencies built with the same sanitizers as the engine. Mixing instrumented and
# uninstrumented copies of libc++ templates gives false container-overflow reports.
include(${CMAKE_CURRENT_LIST_DIR}/otves-host.cmake)

set(VCPKG_C_FLAGS "-fsanitize=address,undefined -fno-omit-frame-pointer")
set(VCPKG_CXX_FLAGS "-fsanitize=address,undefined -fno-omit-frame-pointer")
set(VCPKG_LINKER_FLAGS "-fsanitize=address,undefined")
