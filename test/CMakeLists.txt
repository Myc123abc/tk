add_executable(test main.cpp)
target_link_libraries(test PRIVATE tk)

add_custom_target(copy_assets ALL
  COMMAND ${CMAKE_COMMAND} -E copy_directory
          ${CMAKE_SOURCE_DIR}/assets
          $<TARGET_FILE_DIR:tk>/test/assets
)

add_executable(benchmark benchmark.cpp)
target_link_libraries(benchmark PRIVATE tk)
