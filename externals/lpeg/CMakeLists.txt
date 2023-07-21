cmake_minimum_required(VERSION 3.19)

project(lpeg C)

#include_directories(
#  ../lua/
#  ../../build/external/lua/
#)

add_library("${PROJECT_NAME}" SHARED
  lpcap.c
  lpcode.c
  lpcset.c
  lpprint.c
  lptree.c
  lpvm.c
  lptypes.h
)

target_include_directories(
  ${PROJECT_NAME} PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/../lua/
#${CMAKE_CURRENT_SOURCE_DIR}/externals/lpeg/
)

#target_link_libraries(
#  ${PROJECT_NAME}
#  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../../build/externals/lua/
#)

target_link_libraries(
  ${PROJECT_NAME}
  LINK_PUBLIC lua
#LINK_PUBLIC lpeg_pd3v
)

#install(TARGETS ${PROJECT_NAME} DESTINATION lib)

#install(DIRECTORY "${CMAKE_SOURCE_DIR}/externals/lua"
#  DESTINATION include
#  FILES_MATCHING
#  PATTERN "*.h"
#)
