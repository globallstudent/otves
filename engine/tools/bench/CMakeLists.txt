add_executable(otves_bench main.cpp)
target_link_libraries(otves_bench PRIVATE otves::drafting otves::build_options)
