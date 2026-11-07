# Host description shared by the sanitizer triplets: static libraries for this machine.
# CMAKE_HOST_SYSTEM_PROCESSOR is empty when vcpkg evaluates a triplet, so ask the OS directly.
cmake_host_system_information(RESULT otves_host_arch QUERY OS_PLATFORM)
if(otves_host_arch MATCHES "^(arm64|aarch64|ARM64)$")
  set(VCPKG_TARGET_ARCHITECTURE arm64)
else()
  set(VCPKG_TARGET_ARCHITECTURE x64)
endif()

# Dependencies must use the engine's compiler: CI sets CC/CXX to clang, and mixing GCC and
# Clang sanitizer runtimes does not link.
set(VCPKG_ENV_PASSTHROUGH CC CXX)

set(VCPKG_CRT_LINKAGE dynamic)
set(VCPKG_LIBRARY_LINKAGE static)

if(CMAKE_HOST_SYSTEM_NAME STREQUAL "Darwin")
  set(VCPKG_CMAKE_SYSTEM_NAME Darwin)
  set(VCPKG_OSX_ARCHITECTURES ${VCPKG_TARGET_ARCHITECTURE})
else()
  set(VCPKG_CMAKE_SYSTEM_NAME Linux)
endif()
