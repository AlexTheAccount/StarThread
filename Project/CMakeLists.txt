cmake_minimum_required(VERSION 4.2.1)

project(StarThread LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 26)

file(GLOB_RECURSE SOURCE_FILES CONFIGURE_DEPENDS
	./Source/*.h
	./Source/*.cpp
)

add_executable(StarThread)
target_sources(StarThread PRIVATE ${SOURCE_FILES})

target_include_directories(StarThread PRIVATE
    ${CMAKE_SOURCE_DIR}/Source
)

# tells visual studio to launch from the binary output directory
set_target_properties(StarThread PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIG>"
  VS_DEBUGGER_COMMAND "$<TARGET_FILE:StarThread>"
  VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:StarThread>"
)