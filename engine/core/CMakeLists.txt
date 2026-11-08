add_library(otves_core
  src/assert.cpp
  src/command.cpp
  src/document.cpp
  src/hlc.cpp
  src/level.cpp
  src/op.cpp
  src/opening.cpp
  src/uuid.cpp
  src/wall.cpp)
add_library(otves::core ALIAS otves_core)

target_include_directories(otves_core PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)
target_compile_features(otves_core PUBLIC cxx_std_23)
target_link_libraries(otves_core PRIVATE otves::build_options)

if(ANDROID)
  target_link_libraries(otves_core PRIVATE log)
endif()

if(OTVES_BUILD_TESTS)
  otves_add_test(otves_core_tests
    tests/document_test.cpp
    tests/error_test.cpp
    tests/hlc_test.cpp
    tests/level_test.cpp
    tests/opening_test.cpp
    tests/slot_map_test.cpp
    tests/units_test.cpp
    tests/uuid_test.cpp)
  target_link_libraries(otves_core_tests PRIVATE otves::core)
endif()
