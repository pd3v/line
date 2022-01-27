cmake_minimum_required(VERSION 3.19.2)

# Default as c++17 but also possible c++14
set (CMAKE_CXX_STANDARD 17)

project(line)

set(LIBRARY_OUTPUT_NAME ${PROJECT_NAME})

add_executable("${PROJECT_NAME}" line.cpp)
#target_link_libraries("${PROJECT_NAME}_demo" PUBLIC ${LIBRARY_OUTPUT_NAME})

add_subdirectory(externals/rtmidi)

target_include_directories(
  ${PROJECT_NAME} 
  # PUBLIC external/diatonic/include/
  PRIVATE externals/rtmidi/
)

target_link_directories(
  ${PROJECT_NAME}
  # PUBLIC externals/diatonic/src/
  PRIVATE externals/rtmidi/
)

target_link_libraries(
  ${PROJECT_NAME}
  # LINK_PUBLIC diatonic 
  LINK_PRIVATE rtmidi
)