cmake_minimum_required(VERSION 3.30)
project(tk)

set(CMAKE_CXX_STANDARD 23)

add_library(tk)
target_sources(tk PUBLIC FILE_SET CXX_MODULES FILES
  src/tk.cpp
  src/export/log.cpp
  src/internal/util/error_handling.cpp
  src/internal/util/object_pool.cpp
  src/internal/window.cpp
)

add_executable(test test/main.cpp)
target_link_libraries(test PRIVATE tk)
