add_library(otves_drafting
  src/plan.cpp)
add_library(otves::drafting ALIAS otves_drafting)

target_include_directories(otves_drafting PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)
target_compile_features(otves_drafting PUBLIC cxx_std_23)
target_link_libraries(otves_drafting
  PUBLIC otves::geom
  PRIVATE otves::build_options)

if(OTVES_BUILD_TESTS)
  otves_add_test(otves_drafting_tests
    tests/plan_test.cpp)
  target_link_libraries(otves_drafting_tests PRIVATE otves::drafting)
endif()
