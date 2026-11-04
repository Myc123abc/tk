file(GLOB_RECURSE UI_TEST_SRC CONFIGURE_DEPENDS
  main.cpp
  window/*.cpp  
)

add_executable(test-tk-ui ${UI_TEST_SRC})
target_link_libraries(test-tk-ui PRIVATE tk::ui)
tk_ui_copy_shader(test-tk-ui)
tk_copy_assets(test-tk-ui)

add_executable(test-tk-ui-viewport viewport.cpp)
target_link_libraries(test-tk-ui-viewport PRIVATE tk::ui)
add_dependencies(test-tk-ui-viewport test-tk-ui-shaders)
tk_copy_assets(test-tk-ui-viewport)

add_test(
  NAME test-tk-ui
  COMMAND test-tk-ui
)
