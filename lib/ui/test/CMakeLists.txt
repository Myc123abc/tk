add_executable(test-tk-ui main.cpp)
target_link_libraries(test-tk-ui PRIVATE tk::ui)
tk_ui_copy_shader(test-tk-ui)
tk_copy_assets(test-tk-ui)

add_executable(test-tk-ui-viewport viewport.cpp)
target_link_libraries(test-tk-ui-viewport PRIVATE tk::ui)
tk_ui_copy_shader(test-tk-ui-viewport)
tk_copy_assets(test-tk-ui-viewport)

add_test(
  NAME test-tk-ui
  COMMAND test-tk-ui
)
