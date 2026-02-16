cmake_minimum_required(VERSION 3.22)
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

# Build Box2D from submodule
add_subdirectory(lib/box2d)

# Build cJSON as a simple static library (just the one file)
add_library(cjson_lib STATIC lib/cJSON/cJSON.c)
target_include_directories(cjson_lib PUBLIC lib/cJSON)

# GravityBoost executable
add_executable(GravityBoost
    src/main.c
    src/imgui_sdl3.cpp
    src/app/app.c
    src/game/game.c
    src/physics/physics.c
    src/physics/phys_gravity.c
    src/render/render.c
    src/render/render_ship.c
    src/render/render_background.c
    src/render/render_bounds.c
    src/render/render_planets.c
    src/render/render_ui.c
    src/render/render_field.c
    src/render/planet_gen.c
    src/data/json.c
    src/data/fs.c
)

target_include_directories(GravityBoost PRIVATE src lib/stb)
target_link_libraries(GravityBoost PRIVATE
    SDL3::SDL3-static
    cimgui
    box2d
    cjson_lib
    m
)

# Level Editor executable
add_executable(GravityEditor
    src/editor/editor_main.c
    src/editor/editor_state.c
    src/editor/editor_save.c
    src/editor/editor_ui.c
    src/editor/editor_interact.c
    src/imgui_sdl3.cpp
    src/render/render_planets.c
    src/render/render_bounds.c
    src/render/render_field.c
    src/render/render_background.c
    src/render/planet_gen.c
    src/physics/phys_gravity.c
    src/data/json.c
    src/data/fs.c
)
target_include_directories(GravityEditor PRIVATE src lib/stb)
target_link_libraries(GravityEditor PRIVATE
    SDL3::SDL3-static
    cimgui
    box2d
    cjson_lib
    m
)

if(EMSCRIPTEN)
    set_target_properties(GravityBoost PROPERTIES SUFFIX ".html")
    target_link_options(GravityBoost PRIVATE
        -sALLOW_MEMORY_GROWTH=1
        -sUSE_SDL=0
    )
endif()
