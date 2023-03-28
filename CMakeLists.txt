cmake_minimum_required(VERSION 3.19.2)

set (CMAKE_CXX_STANDARD 17)

project(line VERSION 0.4.21)
    
add_executable("${PROJECT_NAME}" line.cpp)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/externals/rtmidi)

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
)

# target_link_libraries(
#   ${PROJECT_NAME}
#   PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/externals/rtmidi/
# )

target_link_libraries(
  ${PROJECT_NAME} 
  LINK_PRIVATE rtmidi
  LINK_PUBLIC ${LUA_LIB_FILE}
  LINK_PRIVATE readline
)