cmake_minimum_required(VERSION 3.19.2)

set (CMAKE_CXX_STANDARD 17)

project(line VERSION 0.5.25)
    
add_executable("${PROJECT_NAME}" line.cpp)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/externals/rtmidi)
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
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/externals/link/include/ableton/
)

target_link_directories(
  ${PROJECT_NAME}
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/externals/rtmidi/
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/externals/link/src/ableton/link/
)

target_compile_definitions(${PROJECT_NAME} 
  PRIVATE -DLINKHUT_AUDIO_PLATFORM_DUMMY=1
)

target_link_libraries(
  ${PROJECT_NAME} 
  LINK_PRIVATE rtmidi
  LINK_PUBLIC ${LUA_LIB_FILE}
  LINK_PRIVATE readline
  Ableton::Link
)

set(CMAKE_MACOSX_RPATH 1)
install(TARGETS ${PROJECT_NAME} DESTINATION bin)
install(FILES "/usr/local/include/Rtaudio.h" DESTINATION include)
install(FILES "/usr/local/include/Rtmidi.h" DESTINATION include)
install(FILES "/usr/local/lib/librtaudio.dylib" DESTINATION lib)
install(FILES "/usr/local/lib/librtmidi.dylib" DESTINATION lib)

set_target_properties(${PROJECT_NAME} PROPERTIES INSTALL_RPATH "@loader_path/../lib")

include(InstallRequiredSystemLibraries)
#set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/License.txt")
#set(CPACK_PACKAGE_VERSION_MAJOR "${${PROJECT_NAME}_VERSION_MAJOR}")
#set(CPACK_PACKAGE_VERSION_MINOR "${${PROJECT_NAME}_VERSION_MINOR}")
#set(CPACK_PACKAGE_VERSION_PATCH "${coolstuff_VERSION_PATCH}")
include(CPack)
