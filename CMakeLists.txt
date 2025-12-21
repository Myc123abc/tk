cmake_minimum_required(VERSION 3.30)
project(tk)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_subdirectory(vendor/DirectX-Headers)
add_subdirectory(vendor/glm)
add_subdirectory(vendor/SPSCQueue)

file(GLOB_RECURSE SRC src/*.cpp)
add_library(tk ${SRC})
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
)
target_link_libraries(tk
  PRIVATE
    d3d12 dxgi dcomp dxguid dxcompiler
    DirectX-Headers
    glm::glm
    SPSCQueue
)

add_executable(test test/main.cpp)
target_link_libraries(test PRIVATE tk)

add_custom_target(copy_assets ALL
  COMMAND ${CMAKE_COMMAND} -E copy_directory
          ${CMAKE_SOURCE_DIR}/assets
          $<TARGET_FILE_DIR:tk>/assets
)
