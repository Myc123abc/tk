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
set(SDL_AUDIO OFF)
set(SDL_VIDEO ON)
set(SDL_GPU OFF)
set(SDL_RENDER OFF)
set(SDL_CAMERA OFF)
set(SDL_JOYSTICK OFF)
set(SDL_HAPTIC OFF)
set(SDL_HIDAPI OFF)
set(SDL_POWER OFF)
set(SDL_SENSOR OFF)
set(SDL_DIALOG OFF)

set(MSDF_ATLAS_BUILD_STANDALONE OFF)
set(MSDF_ATLAS_USE_VCPKG OFF)
set(MSDF_ATLAS_USE_SKIA OFF)
set(MSDFGEN_DISABLE_PNG ON)

add_subdirectory(vendor/glm)
add_subdirectory(vendor/SDL)
add_subdirectory(vendor/vma)
add_subdirectory(vendor/freetype)

# when generator not vs, ftver.rc which in freetype will be built fail
if (NOT "${CMAKE_GENERATOR}" MATCHES "Visual Studio")
  file(WRITE vendor/freetype/src/base/ftver.rc "")
endif()

# msdfgen only have find_package(Freetype::Freetype)
# we need alias freetype to Freetype::Freetype
if(TARGET freetype AND NOT TARGET Freetype::Freetype)
  add_library(Freetype::Freetype ALIAS freetype)
  message(STATUS "Created ALIAS Freetype::Freetype pointing to actual 'freetype' library.")
endif()

add_subdirectory(vendor/msdf-atlas-gen)

################################################################################
#                               Build Static Library 
################################################################################

file(GLOB_RECURSE SOURCE src/*.cpp)
add_library(tk STATIC ${SOURCE})

target_include_directories(tk
  PUBLIC
    interface
    ${glm_SOURCE_DIR}
  PRIVATE
    include
    ${Vulkan_INCLUDE_DIRS}
)

target_link_libraries(tk
  PUBLIC
    SDL3::SDL3-static
    ${Vulkan_LIBRARIES}
    GPUOpen::VulkanMemoryAllocator
    msdf-atlas-gen
)

target_link_directories(tk
  PRIVATE
    ${Vulkan_LIBRARIES}
)

target_compile_definitions(tk PUBLIC 
  GLM_FORCE_DEPTH_ZERO_TO_ONE
  GLM_FORCE_RADIANS
)

################################################################################
#                              Example 
################################################################################

add_subdirectory(example)