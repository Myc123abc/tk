add_executable(test-tk-ui main.cpp)
target_link_libraries(test-tk-ui PRIVATE tk::ui)
tk_ui_copy_shader(test-tk-ui)
tk_copy_assets(test-tk-ui)

add_executable(test-tk-ui-viewport viewport.cpp)
target_link_libraries(test-tk-ui-viewport PRIVATE tk::ui)
add_dependencies(test-tk-ui-viewport test-tk-ui-shaders)
tk_copy_assets(test-tk-ui-viewport)

add_executable(test-tk-ui-polyline-join polyline_join.cpp)
target_include_directories(test-tk-ui-polyline-join PRIVATE ../src/ui/frame_data)
target_link_libraries(test-tk-ui-polyline-join PRIVATE tk::ui)

add_test(
  NAME test-tk-ui
  COMMAND test-tk-ui
)

add_test(
  NAME test-tk-ui-polyline-join
  COMMAND test-tk-ui-polyline-join
)
