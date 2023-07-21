cmake_minimum_required(VERSION 3.19)

project(lpeg C)

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
)

target_link_libraries(
  ${PROJECT_NAME}
  LINK_PUBLIC lua
)
