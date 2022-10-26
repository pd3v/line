cmake_minimum_required(VERSION 3.19)

project(libsoma_project)

add_library(somar STATIC
  somar.cpp
)

#[[
target_link_libraries(
  ${PROJECT_NAME} 
  lua
)
]]

#install(TARGETS lua DESTINATION lib)
#install(FILES lua.h lualib.h lauxlib.h DESTINATION include)

install(TARGETS somar DESTINATION lib)
install(FILES somar.h DESTINATION include)