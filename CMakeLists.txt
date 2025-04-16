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

add_definitions(-DSPDLOG_USE_STD_FORMAT)
find_package(spdlog CONFIG REQUIRED)

set(SDL_SHARED OFF)
set(SDL_STATIC ON)
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
  GIT_TAG        master 
)
FetchContent_MakeAvailable(VMA)

################################################################################
#                               Build Binrary 
################################################################################

set(INCLUDE
  include/tk
  include/tk/GraphicsEngine
  include/tk/UI
)
file(GLOB_RECURSE SOURCE src/*.cpp)
set(LIBS
  vulkan
  dl
  pthread
  X11
  Xxf86vm
  Xrandr
  Xi
  SDL3::SDL3-static
  GPUOpen::VulkanMemoryAllocator
)

add_executable(Breakout ${SOURCE})

target_include_directories(Breakout PRIVATE ${INCLUDE})

target_link_libraries(Breakout PRIVATE ${LIBS})

target_compile_definitions(Breakout PRIVATE
  GLM_FORCE_DEPTH_ZERO_TO_ONE
  GLM_FORCE_RADIANS
)
