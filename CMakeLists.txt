cmake_minimum_required(VERSION 3.19.2)

set (CMAKE_CXX_STANDARD 17)

project(line VERSION 0.3.3)

set(LIBRARY_OUTPUT_NAME ${PROJECT_NAME})

add_executable("${PROJECT_NAME}" line.cpp)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/externals/rtmidi)

target_link_libraries(
  ${PROJECT_NAME}
  LINK_PRIVATE rtmidi
  LINK_PUBLIC lua
  LINK_PRIVATE readline
)
