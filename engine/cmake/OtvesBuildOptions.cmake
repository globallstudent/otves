# Compile and link options shared by every engine target.
# Targets opt in with: target_link_libraries(<target> PRIVATE otves::build_options)

add_library(otves_build_options INTERFACE)
add_library(otves::build_options ALIAS otves_build_options)

target_compile_features(otves_build_options INTERFACE cxx_std_23)

if(NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  message(FATAL_ERROR "The engine is built with Clang (AppleClang, LLVM or the NDK); got ${CMAKE_CXX_COMPILER_ID}")
endif()

target_compile_options(otves_build_options INTERFACE
  -Wall -Wextra -Wpedantic
  -Wshadow -Wconversion -Wsign-conversion -Wold-style-cast
  -Wnon-virtual-dtor -Woverloaded-virtual -Wnull-dereference
  -Wimplicit-fallthrough -Wdouble-promotion)

# Standard library hardening: full checks in Debug, cheap bounds checks elsewhere.
target_compile_definitions(otves_build_options INTERFACE
  $<IF:$<CONFIG:Debug>,_LIBCPP_HARDENING_MODE=_LIBCPP_HARDENING_MODE_DEBUG,_LIBCPP_HARDENING_MODE=_LIBCPP_HARDENING_MODE_FAST>
  $<$<CONFIG:Debug>:_GLIBCXX_ASSERTIONS>)

if(OTVES_SANITIZERS)
  list(JOIN OTVES_SANITIZERS "," otves_sanitize)
  target_compile_options(otves_build_options INTERFACE
    -fsanitize=${otves_sanitize} -fno-omit-frame-pointer -fno-sanitize-recover=all)
  target_link_options(otves_build_options INTERFACE -fsanitize=${otves_sanitize})
endif()

# Adds a GoogleTest executable for a module and registers its tests with CTest.
function(otves_add_test name)
  add_executable(${name} ${ARGN})
  target_link_libraries(${name} PRIVATE otves::build_options GTest::gtest_main)
  gtest_discover_tests(${name} DISCOVERY_MODE PRE_TEST)
endfunction()
