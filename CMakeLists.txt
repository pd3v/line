cmake_minimum_required(VERSION 3.19.2)

set (CMAKE_CXX_STANDARD 17)

project(line)

set(LIBRARY_OUTPUT_NAME ${PROJECT_NAME})

add_executable("${PROJECT_NAME}" line.cpp)

add_subdirectory(externals/rtmidi)

target_include_directories(
  ${PROJECT_NAME} 
  PRIVATE externals/rtmidi/
)

target_link_directories(
  ${PROJECT_NAME}
  PRIVATE externals/rtmidi/
)

target_link_libraries(
  ${PROJECT_NAME}
  LINK_PRIVATE rtmidi
)