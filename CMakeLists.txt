cmake_minimum_required(VERSION 3.20)
project(GravityBoost C)
enable_language(CXX)

set(CMAKE_C_STANDARD 17)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Build SDL3 as a static library from submodule
set(SDL_SHARED OFF CACHE BOOL "" FORCE)
set(SDL_STATIC ON  CACHE BOOL "" FORCE)
set(SDL_TEST_LIBRARY OFF CACHE BOOL "" FORCE)
set(SDL_TESTS OFF CACHE BOOL "" FORCE)
set(SDL_EXAMPLES OFF CACHE BOOL "" FORCE)
add_subdirectory(lib/SDL)

# Build cimgui as a static library
set(CIMGUI_DIR ${CMAKE_SOURCE_DIR}/lib/cimgui)
set(IMGUI_DIR  ${CIMGUI_DIR}/imgui)

add_library(cimgui STATIC
    ${CIMGUI_DIR}/cimgui.cpp
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_demo.cpp
    ${IMGUI_DIR}/backends/imgui_impl_sdl3.cpp
    ${IMGUI_DIR}/backends/imgui_impl_sdlrenderer3.cpp
)
target_include_directories(cimgui PUBLIC
    ${CIMGUI_DIR}
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
)
target_link_libraries(cimgui PUBLIC SDL3::SDL3-static)

add_executable(qsimplesim
    src/main.c
    src/imgui_sdl3.cpp
)

target_include_directories(GravityBoost PRIVATE src)
target_link_libraries(GravityBoost PRIVATE SDL3::SDL3-static)
target_link_libraries(GravityBoost PRIVATE cimgui)
target_link_libraries(GravityBoost PRIVATE m)

if(EMSCRIPTEN)
    set_target_properties(GravityBoost PROPERTIES SUFFIX ".html")
    target_link_options(GravityBoost PRIVATE
        -sALLOW_MEMORY_GROWTH=1
        -sUSE_SDL=0
    )
endif()
