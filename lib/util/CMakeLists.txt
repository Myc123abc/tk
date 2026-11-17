option(BUILD_TK_UTIL_TEST "Build tk util test programs." OFF)

set(XXHASH_BUILD_XXHSUM OFF)
set(BUILD_SHARED_LIBS OFF)

add_subdirectory(../../vendor/xxHash/build/cmake ${CMAKE_BINARY_DIR}/vendor/xxHash/build/cmake EXCLUDE_FROM_ALL)

file(GLOB_RECURSE TK_UTIL_PUB_MODULE_FILE CONFIGURE_DEPENDS module/*.cppm)

add_library(tk-util)
add_library(tk::util ALIAS tk-util)

target_sources(tk-util
  PUBLIC
    FILE_SET tk_util_pub_module
    TYPE     CXX_MODULES
    FILES    ${TK_UTIL_PUB_MODULE_FILE}
)

target_include_directories(tk-util PRIVATE
  "../../vendor/constexpr-xxh3"
)

target_link_libraries(tk-util PRIVATE xxHash::xxhash)

if (BUILD_TK_UTIL_TEST)
  enable_testing()
  add_subdirectory(test)
endif()
