cmake_minimum_required(VERSION 3.30)
project(tk)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(FT_DISABLE_ZLIB ON)
set(FT_DISABLE_BZIP2 ON)
set(FT_DISABLE_PNG ON)
set(FT_DISABLE_HARFBUZZ ON)
set(FT_DISABLE_BROTLI ON)

set(CLIPPER2_UTILS OFF)
set(CLIPPER2_EXAMPLES OFF)
set(CLIPPER2_TESTS OFF)

set(MSDFGEN_CORE_ONLY ON)
set(MSDFGEN_BUILD_STANDALONE OFF)
set(MSDFGEN_USE_SKIA OFF)
set(MSDFGEN_DISABLE_SVG ON)
set(MSDFGEN_DISABLE_PNG ON)
set(MSDFGEN_DYNAMIC_RUNTIME ON)

add_subdirectory(vendor/DirectX-Headers)
add_subdirectory(vendor/freetype)
add_subdirectory(vendor/harfbuzz)
add_subdirectory(vendor/Clipper2/CPP)
add_subdirectory(vendor/msdfgen)

# when generator not vs, rc files will be built fail
if (NOT "${CMAKE_GENERATOR}" MATCHES "Visual Studio")
  file(WRITE vendor/freetype/src/base/ftver.rc "")
endif()

# build libunibreak
file(GLOB_RECURSE LIBUNIBREAK_SRC vendor/libunibreak/src/*.c)
add_library(libunibreak STATIC ${LIBUNIBREAK_SRC})
target_include_directories(libunibreak PUBLIC vendor/libunibreak/src)

file(GLOB_RECURSE TK_SRC src/*.cpp)
add_library(tk ${TK_SRC})
target_compile_definitions(tk
  PUBLIC
    WIN32_LEAN_AND_MEAN
    NOMINMAX
  PRIVATE
    UTF_CPP_CPLUSPLUS=202002L
)
target_include_directories(tk
  PUBLIC
    include
  PRIVATE
    include/tk
    vendor/stb
    vendor/utfcpp/source
    vendor/freetype/include
    vendor/Clipper2/CPP/Clipper2Lib/include
)
target_link_libraries(tk
  PRIVATE
    d3d12 dxgi dcomp dxguid dxcompiler shcore
    windowsapp dwmapi d3d11 d2d1
    DirectX-Headers
    harfbuzz
    libunibreak
    Clipper2
    msdfgen::msdfgen
)

add_executable(test test/main.cpp)
target_link_libraries(test PRIVATE tk)

add_custom_target(copy_assets ALL
  COMMAND ${CMAKE_COMMAND} -E copy_directory
          ${CMAKE_SOURCE_DIR}/assets
          $<TARGET_FILE_DIR:tk>/assets
)
