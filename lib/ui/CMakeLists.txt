option(BUILD_TK_UI_TEST "Build tk ui test program." OFF)

set(FT_DISABLE_ZLIB     ON)
set(FT_DISABLE_BZIP2    ON)
set(FT_DISABLE_PNG      ON)
set(FT_DISABLE_HARFBUZZ ON)
set(FT_DISABLE_BROTLI   ON)

set(CLIPPER2_UTILS    OFF)
set(CLIPPER2_EXAMPLES OFF)
set(CLIPPER2_TESTS    OFF)

set(MSDFGEN_CORE_ONLY        ON)
set(MSDFGEN_BUILD_STANDALONE OFF)
set(MSDFGEN_USE_SKIA         OFF)
set(MSDFGEN_DISABLE_SVG      ON)
set(MSDFGEN_DISABLE_PNG      ON)
set(MSDFGEN_DYNAMIC_RUNTIME  ON)

set(ZSTD_BUILD_PROGRAMS      OFF CACHE BOOL "" FORCE)
set(ZSTD_BUILD_CONTRIB       OFF CACHE BOOL "" FORCE)
set(ZSTD_BUILD_TESTS         OFF CACHE BOOL "" FORCE)
set(ZSTD_BUILD_SHARED        OFF CACHE BOOL "" FORCE)
set(ZSTD_BUILD_STATIC        ON  CACHE BOOL "" FORCE)
set(ZSTD_BUILD_DICTBUILDER   OFF CACHE BOOL "" FORCE)
set(ZSTD_MULTITHREAD_SUPPORT OFF CACHE BOOL "" FORCE)

add_subdirectory(../../vendor/DirectX-Headers  ${CMAKE_BINARY_DIR}/vendor/DirectX-Headers)
add_subdirectory(../../vendor/freetype         ${CMAKE_BINARY_DIR}/vendor/freetype)
add_subdirectory(../../vendor/harfbuzz         ${CMAKE_BINARY_DIR}/vendor/harfbuzz)
add_subdirectory(../../vendor/Clipper2/CPP     ${CMAKE_BINARY_DIR}/vendor/Clipper2/CPP)
add_subdirectory(../../vendor/msdfgen          ${CMAKE_BINARY_DIR}/vendor/msdfgen)
add_subdirectory(../../vendor/zstd/build/cmake ${CMAKE_BINARY_DIR}/vendor/zstd)

# The FreeType version resource is not needed for the static library.
if (NOT "${CMAKE_GENERATOR}" MATCHES "Visual Studio")
  get_target_property(TK_FREETYPE_SOURCES freetype SOURCES)
  list(FILTER TK_FREETYPE_SOURCES EXCLUDE REGEX "ftver\\.rc$")
  set_property(TARGET freetype PROPERTY SOURCES "${TK_FREETYPE_SOURCES}")
endif()

file(GLOB_RECURSE TK_SRC CONFIGURE_DEPENDS src/*.cpp)

add_library(tk-ui ${TK_SRC})
add_library(tk::ui ALIAS tk-ui)

target_include_directories(tk-ui
  PUBLIC
    include
  PRIVATE
    include/tk
    ../../vendor/stb
    ../../vendor/freetype/include
    ../../vendor/Clipper2/CPP/Clipper2Lib/include
)
target_link_libraries(tk-ui
  PRIVATE
    d3d12 dxgi dcomp dxguid dxcompiler shcore
    windowsapp dwmapi d3d11 d2d1
    DirectX-Headers
    harfbuzz
    Clipper2
    msdfgen::msdfgen
    libzstd_static
  PUBLIC
    tk::util
)

function(tk_ui_copy_shader target)
  file(GLOB_RECURSE TK_UI_SHADER_FILES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/shader/*"
  )
  add_custom_target(${target}-shaders
    COMMAND ${CMAKE_COMMAND} -E copy_directory
      "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/shader"
      "$<TARGET_FILE_DIR:${target}>/shader"
    DEPENDS ${TK_UI_SHADER_FILES}
    VERBATIM
  )
  add_dependencies(${target} ${target}-shaders)
endfunction()

if (BUILD_TK_UI_TEST)
  enable_testing()
  add_subdirectory(test)
endif()
