cmake_minimum_required(VERSION 4.0...4.4)

# Gradle passes the NDK as ANDROID_NDK, while vcpkg's Android triplets read ANDROID_NDK_HOME from
# the environment and the vcpkg toolchain needs the NDK's toolchain to chain to.
if(DEFINED ANDROID_NDK)
  if(NOT DEFINED ENV{ANDROID_NDK_HOME})
    set(ENV{ANDROID_NDK_HOME} "${ANDROID_NDK}")
  endif()
  if(NOT DEFINED VCPKG_CHAINLOAD_TOOLCHAIN_FILE)
    set(VCPKG_CHAINLOAD_TOOLCHAIN_FILE "${ANDROID_NDK}/build/cmake/android.toolchain.cmake")
  endif()
endif()

project(otves_engine VERSION 0.1.0 DESCRIPTION "Otves design engine" LANGUAGES CXX)

set(CMAKE_CXX_EXTENSIONS OFF)

option(OTVES_BUILD_TESTS "Build engine tests" ${PROJECT_IS_TOP_LEVEL})
set(OTVES_SANITIZERS "" CACHE STRING "Sanitizers: address;undefined | thread | hwaddress")

include(cmake/OtvesBuildOptions.cmake)

if(OTVES_BUILD_TESTS)
  enable_testing()
  find_package(GTest CONFIG REQUIRED)
  include(GoogleTest)
endif()

add_subdirectory(core)
add_subdirectory(geom)
add_subdirectory(takeoff)
add_subdirectory(bridge)
