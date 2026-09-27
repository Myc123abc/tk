option(BUILD_TK_UTIL_TEST "Build tk util test programs." OFF)

add_library(tk-util INTERFACE)
add_library(tk::util ALIAS tk-util)

target_include_directories(tk-util INTERFACE
  "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

if (BUILD_TK_UTIL_TEST)
  enable_testing()
  add_subdirectory(test)
endif()
