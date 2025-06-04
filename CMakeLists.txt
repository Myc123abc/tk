cmake_minimum_required(VERSION 3.30)

project(tk)

################################################################################
#                               CMake Setting
################################################################################

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

################################################################################
#                                Library
################################################################################

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

if(WIN32)
find_package(Vulkan REQUIRED)
endif()

include(FetchContent)
FetchContent_Declare(
  FreeType
  GIT_REPOSITORY https://gitlab.freedesktop.org/freetype/freetype.git
  GIT_TAG        master 
)
FetchContent_MakeAvailable(FreeType)

################################################################################
#                               Build Static Library 
################################################################################

set(INCLUDE
  include
  ${freetype_SOURCE_DIR}/include
)

file(GLOB_RECURSE SOURCE src/*.cpp)

if(WIN32)
  set(LIBS
    SDL3::SDL3-static
    GPUOpen::VulkanMemoryAllocator
    freetyped
  )
elseif(UNIX)
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
    freetype
  )
endif()

add_library(tk STATIC ${SOURCE})

target_include_directories(tk PUBLIC ${INCLUDE})

target_link_libraries(tk PUBLIC ${LIBS})

target_compile_definitions(tk PUBLIC 
  GLM_FORCE_DEPTH_ZERO_TO_ONE
  GLM_FORCE_RADIANS
)

if(WIN32)

target_include_directories(tk PUBLIC ${Vulkan_INCLUDE_DIRS})
target_link_directories(tk PUBLIC
  ${freetype_BINARY_DIR}
)
target_link_libraries(tk PUBLIC ${Vulkan_LIBRARIES})

endif()

################################################################################
#                              Example 
################################################################################

add_subdirectory(example)