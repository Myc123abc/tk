cmake_minimum_required(VERSION 3.30)

project(Game-Breakout)

################################################################################
#                               CMake Setting
################################################################################

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

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
  SDL3 
  GIT_REPOSITORY https://github.com/libsdl-org/SDL
  GIT_TAG        main 
)
FetchContent_MakeAvailable(SDL3)

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

add_executable(Breakout ${SOURCE})

target_include_directories(Breakout PRIVATE
  include
  include/GraphicsEngine
  include/GraphicsEngine/imgui
)

target_link_libraries(Breakout PRIVATE ${LIBS})
target_link_libraries(Breakout PRIVATE
  fmt::fmt
  SDL3::SDL3
  GPUOpen::VulkanMemoryAllocator
)

target_compile_definitions(Breakout PRIVATE
  GLM_FORCE_DEPTH_ZERO_TO_ONE
  GLM_FORCE_RADIANS
  # GLM_FORCE_LEFT_HANDED
)
