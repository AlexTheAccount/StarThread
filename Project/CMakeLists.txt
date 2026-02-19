cmake_minimum_required(VERSION 4.2.1)
include(FetchContent)

project(StarThread LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 26)

# Force static linking to avoid runtime DLL requirements
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

# Fetches entt library
FetchContent_Declare(
  entt
  GIT_REPOSITORY https://github.com/skypjack/entt.git
  GIT_TAG v3.16.0
)
FetchContent_MakeAvailable(entt)

# Fetches ImGui library
FetchContent_Declare(
  imgui
  GIT_REPOSITORY https://github.com/ocornut/imgui.git
  GIT_TAG v1.92.5
)
FetchContent_MakeAvailable(imgui)

# Fetches Assimp library
FetchContent_Declare(
  assimp
  GIT_REPOSITORY https://github.com/assimp/assimp.git
  GIT_TAG v6.0.4
)
FetchContent_MakeAvailable(assimp)

# Fetches GLFW library
FetchContent_Declare(
  glfw
  GIT_REPOSITORY https://github.com/glfw/glfw.git
  GIT_TAG 3.4
)
FetchContent_MakeAvailable(glfw)

# Fetches Vulkan install
find_package(Vulkan REQUIRED)

# gather game sources
file(GLOB_RECURSE C_GAME_FILES CONFIGURE_DEPENDS
    ./Game/*.h
    ./Game/*.cpp
    ./gateware-26.33.16/Gateware.h
)

# gather engine sources
file(GLOB_RECURSE C_ENGINE_FILES CONFIGURE_DEPENDS
    ./Engine/*.h
    ./Engine/*.cpp
    ./gateware-26.33.16/Gateware.h
)

# gather shaders
file(GLOB_RECURSE SHADER_FILES CONFIGURE_DEPENDS
    ./Shaders/*.hlsl
)

add_executable(StarThread)
target_sources(StarThread PRIVATE ${C_GAME_FILES})
target_sources(StarThread PRIVATE ${C_ENGINE_FILES})
target_sources(StarThread PRIVATE ${SHADER_FILES})

set(IMGUI_SOURCES
  "${imgui_SOURCE_DIR}/imgui.cpp"
  "${imgui_SOURCE_DIR}/imgui_draw.cpp"
  "${imgui_SOURCE_DIR}/imgui_tables.cpp"
  "${imgui_SOURCE_DIR}/imgui_widgets.cpp"
  "${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp"
  "${imgui_SOURCE_DIR}/backends/imgui_impl_vulkan.cpp"
)
target_sources(StarThread PRIVATE ${IMGUI_SOURCES})

# Shader Compilation
find_program(DXC_EXECUTABLE dxc)
if(NOT DXC_EXECUTABLE)
  message(FATAL_ERROR "DXC not found")
endif()

set(SHADER_COMPILER "${DXC_EXECUTABLE}")
set(COMPILED_SHADERS "")

foreach(SH ${SHADER_FILES})
  get_filename_component(SH_NAME ${SH} NAME_WE)
  string(TOLOWER ${SH_NAME} SH_NAME_LOW)

  # decide profile by filename convention
  if(SH_NAME_LOW MATCHES "vertex")
    set(SH_PROFILE_DXC "vs_6_0")
  else()
    set(SH_PROFILE_DXC "ps_6_0")
  endif()

  # Place compiled SPIR-V into per-config binary dir
  set(OUT_DIR "${CMAKE_BINARY_DIR}/${CMAKE_CFG_INTDIR}/Shaders")
  file(MAKE_DIRECTORY ${OUT_DIR})
  set(OUT_FILE "${OUT_DIR}/${SH_NAME}.spv")
  set(COMPILE_CMD ${DXC_EXECUTABLE} -T ${SH_PROFILE_DXC} -E main -spirv -Fo ${OUT_FILE} ${SH})

  add_custom_command(
    OUTPUT ${OUT_FILE}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${OUT_DIR}
    COMMAND ${COMPILE_CMD}
    DEPENDS ${SH}
    COMMENT "Compiling shader ${SH_NAME} -> ${OUT_FILE}"
    VERBATIM
  )

  list(APPEND COMPILED_SHADERS ${OUT_FILE})
endforeach()

if(COMPILED_SHADERS)
  add_custom_target(Shaders ALL DEPENDS ${COMPILED_SHADERS})
  add_dependencies(StarThread Shaders)
endif()

# make shaders visible in VS solution explorer but not compiled by VS default rules
source_group(TREE ${CMAKE_SOURCE_DIR} FILES ${C_GAME_FILES} ${C_ENGINE_FILES} ${SHADER_FILES})
set_source_files_properties(${SHADER_FILES} PROPERTIES VS_TOOL_OVERRIDE "None")

# include directories and link libraries
target_include_directories(StarThread PRIVATE "${entt_SOURCE_DIR}/single_include")
target_include_directories(StarThread PRIVATE ${CMAKE_SOURCE_DIR}/Source)
target_include_directories(StarThread PRIVATE ${Vulkan_INCLUDE_DIRS})
target_include_directories(StarThread PRIVATE "${imgui_SOURCE_DIR}")
target_include_directories(StarThread PRIVATE "${assimp_SOURCE_DIR}/include")
target_link_libraries(StarThread PRIVATE assimp glfw ${Vulkan_LIBRARIES} comctl32)

# Precompiled header setup (uses Gateware include)
set(PRE_COMPILED
	./Source/precompiled.h 
)

# Organize files into filter in Visual Studio solution explorer
source_group(TREE ${CMAKE_SOURCE_DIR} FILES ${C_GAME_FILES} ${C_ENGINE_FILES} ${SHADER_FILES})

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