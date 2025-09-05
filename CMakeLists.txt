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
endif()

if (MSVC)
  add_compile_options(/permissive-)
endif()
set(FT_DISABLE_HARFBUZZ ON)

add_subdirectory(vendor/glm)
add_subdirectory(vendor/vma)
add_subdirectory(vendor/freetype)
add_subdirectory(vendor/harfbuzz)

# when generator not vs, rc files will be built fail
if (NOT "${CMAKE_GENERATOR}" MATCHES "Visual Studio")
  file(WRITE vendor/freetype/src/base/ftver.rc "")
endif()

################################################################################
#                               Build Static Library 
################################################################################

file(GLOB_RECURSE SOURCE src/*.cpp)
add_library(tk SHARED ${SOURCE})

target_compile_definitions(tk PRIVATE UTF_CPP_CPLUSPLUS=202002L)

target_include_directories(tk
  PUBLIC
    interface
    vendor/glm
  PRIVATE
    include
    ${Vulkan_INCLUDE_DIRS}
    vendor/freetype/include
    vendor/utfcpp/source
)

target_link_libraries(tk
  PRIVATE
    ${Vulkan_LIBRARIES}
    GPUOpen::VulkanMemoryAllocator
    harfbuzz
)

target_link_directories(tk
  PRIVATE
    ${Vulkan_LIBRARIES}
)

target_compile_definitions(tk
  PRIVATE
    SHARED_TK
    GLM_FORCE_DEPTH_ZERO_TO_ONE
    GLM_FORCE_RADIANS
)

################################################################################
#                              Example 
################################################################################

add_subdirectory(example)