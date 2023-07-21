cmake_minimum_required(VERSION 3.19.2)

set (CMAKE_CXX_STANDARD 17)

project(line VERSION 0.4.24)
    
add_executable("${PROJECT_NAME}" line.cpp)

#add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/externals/readline)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/externals/lua)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/externals/lpeg)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/externals/rtmidi)

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
  PRIVATE ${READLINE_HEADER_PATH}/
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/externals/lua/
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/externals/lpeg/
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/externals/rtmidi/
)

target_link_directories(
  ${PROJECT_NAME}
  PRIVATE ${READLINE_LIB_PATH}/
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/build/externals/lua/
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/build/externals/lpeg/
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/build/externals/rtmidi/
)

target_link_libraries(
  ${PROJECT_NAME}
  LINK_PRIVATE readline
  LINK_PRIVATE lua
  LINK_PRIVATE lpeg
  LINK_PRIVATE rtmidi
)
