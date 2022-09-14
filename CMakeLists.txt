cmake_minimum_required(VERSION 3.19.2)

set (CMAKE_CXX_STANDARD 17)

project(line VERSION 0.3.4)

set(LIBRARY_OUTPUT_NAME ${PROJECT_NAME})

add_executable("${PROJECT_NAME}" line.cpp)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/externals/rtmidi)

SET(LUA_LIB_NAME liblua.a liblua.so)

FIND_PATH(LUA_INCLUDE_DIR lua.h
  /usr/
  /opt/
)
message(LUA_INCLUDE_DIR=${LUA_INCLUDE_DIR})

FIND_LIBRARY(LUA_LIB_FILE
  NAMES ${LUA_LIB_NAME}
  PATHS /usr/ /opt/
)
message(LUA_LIB_FILE=${LUA_LIB_FILE})

target_include_directories(
  ${PROJECT_NAME} PUBLIC
  ${LUA_INCLUDE_DIR}
)

target_link_libraries(
  ${PROJECT_NAME}
  LINK_PRIVATE rtmidi
  LINK_PUBLIC ${LUA_LIB_FILE}
  LINK_PRIVATE readline
)