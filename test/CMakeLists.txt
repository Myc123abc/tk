add_executable(test_tk main.cpp)
target_link_libraries(test_tk PRIVATE tk)
tk_target(test_tk)

add_test(
  NAME test_tk
  COMMAND test_tk
)
