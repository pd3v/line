cmake_minimum_required(VERSION 3.19.2)

set (CMAKE_CXX_STANDARD 17)

project(line VERSION 0.3.3)

set(LIBRARY_OUTPUT_NAME ${PROJECT_NAME})

add_executable("${PROJECT_NAME}" line.cpp)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/externals/rtmidi)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/externals/lua)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/externals/lpeg)

target_link_libraries(
  ${PROJECT_NAME}
  LINK_PRIVATE rtmidi
  LINK_PUBLIC lua4line
  LINK_PUBLIC lpeg4line
  LINK_PRIVATE readline
)

install(TARGETS ${PROJECT_NAME} DESTINATION bin)

set(CMAKE_MACOSX_RPATH 1)
set_target_properties(${PROJECT_NAME} PROPERTIES INSTALL_RPATH "@loader_path/../lib")

include(InstallRequiredSystemLibraries)
#set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/License.txt")
include(CPack)