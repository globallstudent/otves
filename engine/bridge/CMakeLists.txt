find_package(flatbuffers CONFIG REQUIRED)
# flatc runs on the build machine, also when cross-compiling for Android. vcpkg installs it with
# the host triplet, which it picks itself unless VCPKG_HOST_TRIPLET is set.
file(GLOB otves_flatc_dirs "${VCPKG_INSTALLED_DIR}/*/tools/flatbuffers")
find_program(OTVES_FLATC flatc HINTS ${otves_flatc_dirs} NO_DEFAULT_PATH REQUIRED)

set(otves_schema_file "${PROJECT_SOURCE_DIR}/../schema/engine.fbs")
set(otves_schema_dir "${CMAKE_CURRENT_BINARY_DIR}/generated")
set(otves_schema_header "${otves_schema_dir}/engine_generated.h")
add_custom_command(
  OUTPUT "${otves_schema_header}"
  COMMAND "${OTVES_FLATC}" --cpp --scoped-enums -o "${otves_schema_dir}" "${otves_schema_file}"
  DEPENDS "${otves_schema_file}" "${OTVES_FLATC}"
  COMMENT "Generating C++ for schema/engine.fbs"
  VERBATIM)
add_custom_target(otves_schema_code DEPENDS "${otves_schema_header}")

# The generated code, for the bridge and its tests. SYSTEM keeps its warnings out of our build.
add_library(otves_schema INTERFACE)
add_library(otves::schema ALIAS otves_schema)
target_include_directories(otves_schema SYSTEM INTERFACE "${otves_schema_dir}")
target_link_libraries(otves_schema INTERFACE flatbuffers::flatbuffers)
add_dependencies(otves_schema otves_schema_code)

add_library(otves_bridge
  src/convert.cpp
  src/derived.cpp
  src/oplog.cpp
  src/placement.cpp
  src/session.cpp)
add_library(otves::bridge ALIAS otves_bridge)

target_include_directories(otves_bridge PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)
target_compile_features(otves_bridge PUBLIC cxx_std_23)
target_link_libraries(otves_bridge
  PUBLIC otves::core
  PRIVATE otves::build_options otves::drafting otves::geom otves::schema otves::takeoff)
add_dependencies(otves_bridge otves_schema_code)

# libdesign.so for the app. Desktop builds it too when a JDK is found, so JNI code compiles in CI.
if(NOT ANDROID)
  find_package(JNI COMPONENTS JVM)
endif()
if(ANDROID OR JNI_FOUND)
  add_library(otves_design SHARED src/jni.cpp)
  set_target_properties(otves_design PROPERTIES OUTPUT_NAME design)
  target_link_libraries(otves_design PRIVATE otves::bridge otves::build_options)
  if(NOT ANDROID)
    # The library is loaded into a running JVM, so it needs jni.h but not libjvm.
    target_include_directories(otves_design SYSTEM PRIVATE ${JNI_INCLUDE_DIRS})
  endif()
endif()

if(OTVES_BUILD_TESTS)
  otves_add_test(otves_bridge_tests
    tests/oplog_test.cpp
    tests/session_test.cpp)
  # The op log tests reach the private codecs.
  target_include_directories(otves_bridge_tests PRIVATE src)
  target_link_libraries(otves_bridge_tests PRIVATE otves::bridge otves::schema)
  target_compile_definitions(otves_bridge_tests PRIVATE
    OTVES_CONTENT_DIR="${PROJECT_SOURCE_DIR}/../content")
  add_dependencies(otves_bridge_tests otves_schema_code)
endif()
