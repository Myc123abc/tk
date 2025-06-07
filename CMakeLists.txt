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

if(WIN32)
find_package(Vulkan REQUIRED)
# avoid vma use MTd_StaticDebug but msdfgen use MDd_DynamicDebug
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

set(SDL_SHARED OFF)
set(SDL_STATIC ON)
set(SDL_TEST_LIBRARY OFF)
set(MSDF_ATLAS_BUILD_STANDALONE OFF)
set(MSDF_ATLAS_USE_VCPKG OFF)
set(MSDF_ATLAS_USE_SKIA OFF)
set(MSDFGEN_DISABLE_PNG ON)

include(FetchContent)

FetchContent_Declare(
  SDL3 
  GIT_REPOSITORY https://github.com/libsdl-org/SDL
  GIT_TAG        main 
)

FetchContent_Declare(
  VMA
  GIT_REPOSITORY https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator.git
  GIT_TAG        master 
)

# when build system is not visual studio, it maybe error because of rc file
# just clear the content of rc file and build again, it's ok
FetchContent_Declare(
  freetype
  GIT_REPOSITORY https://gitlab.freedesktop.org/freetype/freetype.git
  GIT_TAG        master
  OVERRIDE_FIND_PACKAGE
)

FetchContent_Declare(
  msdf-atlas-gen
  GIT_REPOSITORY https://github.com/Chlumsky/msdf-atlas-gen.git
  GIT_TAG        master
)

FetchContent_MakeAvailable(SDL3 VMA freetype msdf-atlas-gen)

if(TARGET freetype AND NOT TARGET Freetype::Freetype)
  add_library(Freetype::Freetype ALIAS freetype)
  message(STATUS "Created ALIAS Freetype::Freetype pointing to actual 'freetype' library.")
endif()

################################################################################
#                               Build Static Library 
################################################################################

set(INCLUDE
  include
)

file(GLOB_RECURSE SOURCE src/*.cpp)

if(WIN32)
  set(LIBS
    SDL3::SDL3-static
    GPUOpen::VulkanMemoryAllocator
    msdf-atlas-gen
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
    msdf-atlas-gen
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
target_link_libraries(tk PUBLIC ${Vulkan_LIBRARIES})

endif()

################################################################################
#                              Example 
################################################################################

add_subdirectory(example)