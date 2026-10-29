add_library(tk-benchmark src/benchmark.cpp)

add_library(tk::benchmark ALIAS tk-benchmark)
target_include_directories(tk-benchmark PUBLIC include)
target_link_libraries(tk-benchmark PRIVATE tk::ui)
target_link_libraries(tk-benchmark PUBLIC tk::util)

add_executable(benchmark-test test/test.cpp)
target_link_libraries(benchmark-test PRIVATE tk::benchmark)
tk_ui_copy_shader(benchmark-test)
tk_copy_assets(benchmark-test)

add_executable(benchmark-object-pool test/object_pool.cpp)
target_include_directories(benchmark-object-pool PRIVATE ../ui/src/util)
target_link_libraries(benchmark-object-pool PRIVATE tk::benchmark)
set_target_properties(benchmark-object-pool PROPERTIES
	RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/object_pool"
)
tk_ui_copy_shader(benchmark-object-pool)
tk_copy_assets(benchmark-object-pool)
