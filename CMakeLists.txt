cmake_minimum_required(VERSION 3.30)

project(Game-Breakout)

################################################################################
#                               CMake Setting
################################################################################

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

################################################################################
#                               Macro Definition
################################################################################

add_definitions(-DGLM_FORCE_DEPTH_ZERO_TO_ONE)
add_definitions(-DGLM_FORCE_RADIANS)
# add_definitions(-DGLM_FORCE_LEFT_HANDED)

################################################################################
#                                Library
################################################################################

include(FetchContent)
FetchContent_Declare(
  fmt
  GIT_REPOSITORY https://github.com/fmtlib/fmt
  GIT_TAG        master
)
FetchContent_MakeAvailable(fmt)

include(FetchContent)
FetchContent_Declare(
  VMA
  GIT_REPOSITORY https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator.git
  GIT_TAG       master 
)
FetchContent_MakeAvailable(VMA)

################################################################################
#                               Build Binrary 
################################################################################

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

target_include_directories(Breakout PRIVATE
  include
  include/GraphicsEngine
)

target_link_libraries(Breakout PRIVATE ${LIBS})
target_link_libraries(Breakout PRIVATE
  fmt::fmt
  GPUOpen::VulkanMemoryAllocator
)
