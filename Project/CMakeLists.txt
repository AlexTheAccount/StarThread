cmake_minimum_required(VERSION 4.2.1)
include(FetchContent)

project(StarThread LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 26)

# Fetches entt library from it's github
FetchContent_Declare(
  entt
  GIT_REPOSITORY https://github.com/skypjack/entt.git
  GIT_TAG v3.16.0
)
FetchContent_MakeAvailable(entt)

# Fetches ImGui library from it's github
FetchContent_Declare(
  imgui
  GIT_REPOSITORY https://github.com/ocornut/imgui.git
  GIT_TAG v1.92.5
)
FetchContent_MakeAvailable(imgui)

# Fetches Vulkan install
find_package(Vulkan REQUIRED)

file(GLOB_RECURSE SOURCE_FILES CONFIGURE_DEPENDS
	./Source/*.h
	./Source/*.cpp
)

add_executable(StarThread)
target_sources(StarThread PRIVATE ${SOURCE_FILES})
target_include_directories(StarThread PRIVATE "${entt_SOURCE_DIR}/single_include")
target_include_directories(StarThread PRIVATE ${CMAKE_SOURCE_DIR}/Source)
target_include_directories(StarThread PRIVATE ${Vulkan_INCLUDE_DIRS})
target_include_directories(StarThread PRIVATE "${CMAKE_SOURCE_DIR}/gateware-26.33.16/include")
target_include_directories(StarThread PRIVATE "${imgui_SOURCE_DIR}")

# Organize files into filter in Visual Studio solution explorer
source_group(TREE ${CMAKE_SOURCE_DIR} FILES ${SOURCE_FILES})

# swaps "ALL_BUILD" with project as startup project
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT StarThread)

# add a preprocessor definition so target architecture can be detected
target_compile_definitions(StarThread PRIVATE _AMD64_)

# tells visual studio to launch from the binary output directory
set_target_properties(StarThread PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIG>"
  VS_DEBUGGER_COMMAND "$<TARGET_FILE:StarThread>"
  VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:StarThread>"
)