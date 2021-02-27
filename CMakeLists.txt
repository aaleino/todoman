cmake_minimum_required (VERSION 3.10)

set(CMAKE_CXX_FLAGS "-O3")

project(todoman
  LANGUAGES CXX
  VERSION 1.0.0
)

add_executable(todoman main.cpp)
add_subdirectory(FTXUI)

target_link_libraries(todoman
  PRIVATE ftxui::screen
  PRIVATE ftxui::dom
  PRIVATE ftxui::component)

set_target_properties(todoman PROPERTIES CXX_STANDARD 17)

install(TARGETS todoman RUNTIME DESTINATION "bin")

execute_process(COMMAND ${CMAKE_COMMAND} -E cmake_echo_color --red Remember to update FTXUI aswell )
