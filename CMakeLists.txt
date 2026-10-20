cmake_minimum_required(VERSION 3.30)
project(tk)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(BUILD_TK_UI        "Build tk ui library."        ON)
option(BUILD_TK_UTIL      "Build tk util library."      ON)
option(BUILD_TK_BENCHMARK "Build tk benchmark library." ON)

add_compile_definitions(
  WIN32_LEAN_AND_MEAN
  NOMINMAX
)

if(MSVC)
  add_compile_options(/utf-8)
endif()

function(tk_copy_assets target)
  if (CMAKE_GENERATOR MATCHES "Ninja")
    set_property(GLOBAL PROPERTY JOB_POOLS tk_asset_copy=1)
    set_property(TARGET ${target} PROPERTY JOB_POOL_LINK tk_asset_copy)
  endif()

  add_custom_command(TARGET ${target} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
      "${CMAKE_SOURCE_DIR}/assets"
      "$<TARGET_FILE_DIR:${target}>/assets"
    VERBATIM
  )
endfunction()

if (BUILD_TK_UTIL OR BUILD_TK_UI OR BUILD_TK_BENCHMARK)
  add_subdirectory(lib/util)
endif()

if (BUILD_TK_UI OR BUILD_TK_BENCHMARK)
  add_subdirectory(lib/ui)
endif()

if (BUILD_TK_BENCHMARK)
  add_subdirectory(lib/benchmark)
endif()
