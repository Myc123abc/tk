option(BUILD_TK_UTIL_TEST "Build tk util test programs." OFF)

set(XXHASH_BUILD_XXHSUM OFF)
set(BUILD_SHARED_LIBS OFF)

add_subdirectory(../../vendor/xxHash/build/cmake ${CMAKE_BINARY_DIR}/vendor/xxHash/build/cmake EXCLUDE_FROM_ALL)

add_library(tk-util INTERFACE)
add_library(tk::util ALIAS tk-util)

target_include_directories(tk-util INTERFACE
  "${CMAKE_CURRENT_SOURCE_DIR}/include"
  "../../vendor/constexpr-xxh3"
)

target_link_libraries(tk-util INTERFACE xxHash::xxhash)

if (BUILD_TK_UTIL_TEST)
  enable_testing()
  add_subdirectory(test)
endif()
