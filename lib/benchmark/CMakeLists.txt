add_library(tk-benchmark src/benchmark.cpp)

add_library(tk::benchmark ALIAS tk-benchmark)
target_include_directories(tk-benchmark PUBLIC include)
target_link_libraries(tk-benchmark PRIVATE tk::ui)

add_executable(benchmark-test test/test.cpp)
target_link_libraries(benchmark-test PRIVATE tk::benchmark)
tk_ui_copy_shader(benchmark-test)
tk_copy_assets(benchmark-test)
