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

add_subdirectory(vendor/DirectX-Headers)
add_subdirectory(vendor/glm)
add_subdirectory(vendor/SPSCQueue)
add_subdirectory(vendor/freetype)
add_subdirectory(vendor/harfbuzz)

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
  PRIVATE
    WIN32_LEAN_AND_MEAN
    NOMINMAX
    UTF_CPP_CPLUSPLUS=202002L
)
target_include_directories(tk
  PUBLIC
    include
  PRAIVATE
    include/tk
    vendor/stb
    vendor/utfcpp/source
    vendor/freetype/include
)
target_link_libraries(tk
  PUBLIC
    glm::glm
  PRIVATE
    d3d12 dxgi dcomp dxguid dxcompiler
    DirectX-Headers
    SPSCQueue
    harfbuzz
    libunibreak
)

add_executable(test test/main.cpp)
target_link_libraries(test PRIVATE tk)

add_custom_target(copy_assets ALL
  COMMAND ${CMAKE_COMMAND} -E copy_directory
          ${CMAKE_SOURCE_DIR}/assets
          $<TARGET_FILE_DIR:tk>/assets
)
