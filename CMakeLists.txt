cmake_minimum_required(VERSION 3.19.2)

set(CMAKE_CXX_STANDARD 17)
if(APPLE)
  #set(CMAKE_C_COMPILER /usr/bin/clang)
  #set(CMAKE_CXX_COMPILER /usr/bin/clang++)
  #set(CMAKE_C_COMPILER /opt/local/libexec/llvm-10/bin/clang)
  #set(CMAKE_CXX_COMPILER /opt/local/libexec/llvm-10/bin/clang++)
endif(APPLE)

if(LINUX)
  set(CMAKE_C_COMPILER /usr/bin/gcc)
  set(CMAKE_CXX_COMPILER /usr/bin/g++)
endif(LINUX)

project(line VERSION 0.5.36)

add_executable("${PROJECT_NAME}" line.cpp)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/externals/lua)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/externals/lpeg)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/externals/rtmidi)

include(${CMAKE_CURRENT_SOURCE_DIR}/externals/link/AbletonLinkConfig.cmake)

FIND_PATH(READLINE_INCLUDE_PATH
  NAMES readline
  PATHS /opt/ /usr/
)

FIND_LIBRARY(READLINE_LIB_FILE
  NAMES readline
  PATHS /opt/ /usr/
)

get_filename_component(READLINE_LIB_PATH ${READLINE_LIB_FILE} PATH)

target_include_directories(
  ${PROJECT_NAME} PUBLIC
  PRIVATE ${READLINE_INCLUDE_PATH}
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/externals/lua/
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/externals/lpeg/
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/externals/rtmidi/
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/externals/link/include/ableton/
)

target_link_directories(
  ${PROJECT_NAME}
  PRIVATE ${READLINE_LIB_PATH}/
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/build/externals/lua/
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/build/externals/lpeg/
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/build/externals/rtmidi/
  #PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/build/externals/link/src/ableton/link/
)

target_compile_definitions(${PROJECT_NAME}
  PRIVATE -DLINKHUT_AUDIO_PLATFORM_DUMMY=1
)

target_link_libraries(
  ${PROJECT_NAME}
  LINK_PRIVATE readline
  LINK_PRIVATE lua
  LINK_PRIVATE lpeg
  LINK_PRIVATE rtmidi
  Ableton::Link
)