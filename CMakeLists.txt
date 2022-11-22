cmake_minimum_required(VERSION 3.19.2)

set (CMAKE_CXX_STANDARD 17)

project(line VERSION 0.4.3)

set(LIBRARY_OUTPUT_NAME ${PROJECT_NAME})

add_executable("${PROJECT_NAME}" line.cpp
  AudioPlatform_rtaudio.hpp
  AudioPlatform_rtaudio.cpp
  AudioPlatform.hpp
  AudioEngine.hpp
  AudioEngine.cpp
)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/externals/rtmidi)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/externals/rtaudio)

include(${CMAKE_CURRENT_SOURCE_DIR}/externals/link/AbletonLinkConfig.cmake)

if (CMAKE_SYSTEM_NAME STREQUAL "Darwin")
  SET(LUA_LIB_NAME liblua.a liblua.dylib)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
  SET(LUA_LIB_NAME liblua.a liblua.so)
endif ()

FIND_PATH(LUA_INCLUDE_DIR lua.h
  /opt/
  /usr/
)

FIND_LIBRARY(LUA_LIB_FILE
  NAMES ${LUA_LIB_NAME}
  PATHS /opt/ /usr/
)

target_include_directories(
  ${PROJECT_NAME} PUBLIC
  ${LUA_INCLUDE_DIR}
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/externals/rtmidi/
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/externals/rtaudio/include/
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/externals/link/include/ableton/
)

target_link_directories(
  ${PROJECT_NAME}
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/externals/rtmidi/
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/externals/rtaudio/
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/externals/link/src/ableton/link/
)

target_link_libraries(
  ${PROJECT_NAME}
  LINK_PRIVATE rtmidi
  LINK_PUBLIC ${LUA_LIB_FILE}
  LINK_PRIVATE readline
  LINK_PUBLIC rtaudio
  Ableton::Link
)
