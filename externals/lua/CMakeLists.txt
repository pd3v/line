cmake_minimum_required(VERSION 3.19)

project(lua C)

add_library(${PROJECT_NAME} SHARED
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

if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
  set(CURRENT_OS LUA_USE_MACOSX=1)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
  set(CURRENT_OS LUA_USE_LINUX=1)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Windows")
  set(CURRENT_OS DLUA_USE_WINDOWS=1)
endif()

target_compile_definitions(${PROJECT_NAME}
  PRIVATE -D${CURRENT_OS}
)

target_link_libraries(${PROJECT_NAME}
  LINK_PRIVATE readline
)
