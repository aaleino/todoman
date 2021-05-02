cmake_minimum_required (VERSION 3.10)

# --- Fetch FTXUI --------------------------------------------------------------
include(FetchContent)

set(FETCHCONTENT_UPDATES_DISCONNECTED TRUE)
FetchContent_Declare(ftxui
  GIT_REPOSITORY https://github.com/aaleino/FTXUI
  GIT_TAG 40929423e0b3eb5e276661be91f183641440cd5c
)

FetchContent_GetProperties(ftxui)
if(NOT ftxui_POPULATED)
  FetchContent_Populate(ftxui)
  add_subdirectory(${ftxui_SOURCE_DIR} ${ftxui_BINARY_DIR} EXCLUDE_FROM_ALL)
endif()

# ------------------------------------------------------------------------------


set(CMAKE_CXX_FLAGS "-O3")

project(todoman
  LANGUAGES CXX
  VERSION 1.0.0
)

add_executable(todoman main.cpp)

target_link_libraries(todoman
  PRIVATE ftxui::screen
  PRIVATE ftxui::dom
  PRIVATE ftxui::component)

set_target_properties(todoman PROPERTIES CXX_STANDARD 17)

install(TARGETS todoman RUNTIME DESTINATION "bin")
