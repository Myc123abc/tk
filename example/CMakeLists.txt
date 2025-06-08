add_executable(example main.cpp)

target_link_libraries(example PRIVATE tk)
target_include_directories(example PRIVATE ${CMAKE_SOURCE_DIR}/vendor/glm)