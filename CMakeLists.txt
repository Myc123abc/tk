cmake_minimum_required(VERSION 3.30)

project(Game-Breakout)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_definitions(-DFMT_HEADER_ONLY)
add_definitions(-DSPDLOG_HEADER_ONLY)
add_definitions(-DGLM_FORCE_DEPTH_ZERO_TO_ONE)
add_definitions(-DGLM_FORCE_RADIANS)
# add_definitions(-DGLM_FORCE_LEFT_HANDED)

find_package(spdlog CONFIG REQUIRED)

include(FetchContent)
FetchContent_Declare(
  VMA
  GIT_REPOSITORY https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator.git
  GIT_TAG       master 
)
FetchContent_MakeAvailable(VMA)

file(GLOB_RECURSE SOURCE src/*.cpp)
set(LIBS
  glfw
  vulkan
  dl
  pthread
  X11
  Xxf86vm
  Xrandr
  Xi
)

add_executable(Breakout src/main.cpp ${SOURCE})

target_include_directories(Breakout PRIVATE include)

target_link_libraries(Breakout PRIVATE ${LIBS})
target_link_libraries(Breakout PRIVATE GPUOpen::VulkanMemoryAllocator)
