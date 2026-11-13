add_library(otves_geom
  src/editing.cpp
  src/openings.cpp
  src/polygon.cpp
  src/sketch.cpp
  src/wall_joins.cpp)
add_library(otves::geom ALIAS otves_geom)

target_include_directories(otves_geom PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)
target_compile_features(otves_geom PUBLIC cxx_std_23)
target_link_libraries(otves_geom
  PUBLIC otves::core
  PRIVATE otves::build_options)

if(OTVES_BUILD_TESTS)
  otves_add_test(otves_geom_tests
    tests/editing_test.cpp
    tests/openings_test.cpp
    tests/polygon_test.cpp
    tests/sketch_test.cpp
    tests/wall_joins_test.cpp)
  target_link_libraries(otves_geom_tests PRIVATE otves::geom)
endif()
