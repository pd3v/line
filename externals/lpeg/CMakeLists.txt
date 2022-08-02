cmake_minimum_required(VERSION 3.19)

project(lpeglib_project C)

add_library(lpeg4line STATIC
  lpcap.c
  lpcode.c
  lpprint.c
  lptree.c
  # lptypes.h
  lpvm.c
)

install(TARGETS lpeg4line DESTINATION lib)

install(DIRECTORY "${CMAKE_SOURCE_DIR}/externals/lpeg"
  DESTINATION include
  FILES_MATCHING
  PATTERN "*.h"
)