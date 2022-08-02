cmake_minimum_required(VERSION 3.19)

project(lualib_project C)

add_library(lua4line SHARED
  lapi.c
  lauxlib.c
  lbaselib.c
  lcode.c
  lcorolib.c
  lctype.c
  ldblib.c
  ldebug.c
  ldo.c
  ldump.c
  lfunc.c
  lgc.c
  linit.c
  liolib.c
  llex.c
  lmathlib.c
  lmem.c
  loadlib.c
  lobject.c
  lopcodes.c
  loslib.c
  lparser.c
  lstate.c
  lstring.c
  lstrlib.c
  ltable.c
  ltablib.c
  ltm.c
  lua.c
  lundump.c
  lutf8lib.c
  lvm.c
  lzio.c
)

install(TARGETS lua4line DESTINATION lib)

install(DIRECTORY "${CMAKE_SOURCE_DIR}/externals/lua"
  DESTINATION include
  FILES_MATCHING
  PATTERN "*.h" PATTERN "*.hpp"
)