find_package(unofficial-luau CONFIG REQUIRED)

add_library(otves_takeoff
  src/masonry.cpp
  src/rules.cpp
  src/sandbox.cpp
  src/wall_quantities.cpp)
add_library(otves::takeoff ALIAS otves_takeoff)

target_include_directories(otves_takeoff PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)
target_compile_features(otves_takeoff PUBLIC cxx_std_23)
target_link_libraries(otves_takeoff
  PUBLIC otves::geom
  PRIVATE otves::build_options unofficial::luau::Luau.Compiler unofficial::luau::Luau.VM)

if(OTVES_BUILD_TESTS)
  otves_add_test(otves_takeoff_tests
    tests/content_test.cpp
    tests/masonry_test.cpp
    tests/rules_test.cpp
    tests/wall_quantities_test.cpp)
  target_link_libraries(otves_takeoff_tests PRIVATE otves::takeoff)
  target_compile_definitions(otves_takeoff_tests PRIVATE
    OTVES_CONTENT_DIR="${PROJECT_SOURCE_DIR}/../content")
endif()
