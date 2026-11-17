file(GLOB_RECURSE TK_BENCHMARK_MODULE_FILE CONFIGURE_DEPENDS module/*.cppm)

add_library(tk-benchmark src/benchmark.cpp)

add_library(tk::benchmark ALIAS tk-benchmark)
target_sources(tk-benchmark
  PUBLIC
    FILE_SET tk_benchmark_pub_module
    TYPE     CXX_MODULES
    FILES    ${TK_BENCHMARK_MODULE_FILE}
)
target_link_libraries(tk-benchmark PRIVATE tk::ui)
target_link_libraries(tk-benchmark PUBLIC tk::util)

add_executable(benchmark-test test/test.cpp)
target_link_libraries(benchmark-test PRIVATE tk::benchmark)
tk_ui_copy_shader(benchmark-test)
tk_copy_assets(benchmark-test)

add_executable(benchmark-object-pool test/object_pool.cpp)
target_link_libraries(benchmark-object-pool PRIVATE tk::benchmark tk::ui)
set_target_properties(benchmark-object-pool PROPERTIES
	RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/object_pool"
)
tk_ui_copy_shader(benchmark-object-pool)
tk_copy_assets(benchmark-object-pool)
