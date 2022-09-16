cmake_minimum_required(VERSION 3.19.2)

set (CMAKE_CXX_STANDARD 17)

project(line VERSION 0.4.0)

set(LIBRARY_OUTPUT_NAME ${PROJECT_NAME})

add_executable("${PROJECT_NAME}" line.cpp)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/externals/rtmidi)

SET(LUA_LIB_NAME liblua5.4-0.a liblua5.4-0.so)

# FIND_PATH(LUA_INCLUDE_DIR lua.h
#   /usr/include/
#   /opt/
# )
# message(LUA_INCLUDE_DIR=${LUA_INCLUDE_DIR})

# target_include_directories(
#   ${PROJECT_NAME} PUBLIC
#   # ${LUA_INCLUDE_DIR}
#   /usr/include/lua5.4/
#   /usr/include/
# )

FIND_LIBRARY(LUA_LIB_FILE
  NAMES ${LUA_LIB_NAME}
  PATHS /usr/lib/lua.5.4/ /usr/lib/
)
message(LUA_LIB_FILE=${LUA_LIB_FILE})

target_link_libraries(
  ${PROJECT_NAME}
  LINK_PRIVATE rtmidi
  LINK_PUBLIC ${LUA_LIB_FILE}
  LINK_PRIVATE readline
)