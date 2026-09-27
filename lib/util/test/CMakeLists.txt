add_executable(test-memory-pool memory_pool.cpp)
add_executable(benchmark-memory-pool memory_pool_benchmark.cpp)

target_link_libraries(test-memory-pool
  PRIVATE
    tk::util
)

target_link_libraries(benchmark-memory-pool
  PRIVATE
    tk::util
)

add_test(
  NAME memory-pool
  COMMAND test_memory-pool
)
