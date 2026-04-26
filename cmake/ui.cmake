# third_party/sdl2 — SDL2 静态库，供 native `ui` 模块（与 libuv 同构的 submodule + add_subdirectory）。

if(TARGET qianjs::ui_deps)
    return()
endif()

get_filename_component(_qianjs_root "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE)
set(_root "${_qianjs_root}/third_party/sdl2")
if(NOT EXISTS "${_root}/CMakeLists.txt")
    message(FATAL_ERROR "SDL2 missing at ${_root}.\n  git submodule update --init third_party/sdl2")
endif()

set(SDL_SHARED OFF CACHE BOOL "" FORCE)
set(SDL_STATIC ON CACHE BOOL "" FORCE)
set(SDL_TESTS OFF CACHE BOOL "" FORCE)
set(SDL2_DISABLE_INSTALL ON CACHE BOOL "" FORCE)

add_subdirectory("${_root}" "${CMAKE_BINARY_DIR}/third_party/sdl2" EXCLUDE_FROM_ALL)

add_library(qianjs_ui_deps INTERFACE)
if(TARGET SDL2::SDL2-static)
    target_link_libraries(qianjs_ui_deps INTERFACE SDL2::SDL2-static)
elseif(TARGET SDL2::SDL2)
    target_link_libraries(qianjs_ui_deps INTERFACE SDL2::SDL2)
else()
    message(FATAL_ERROR "SDL2 did not define SDL2::SDL2 or SDL2::SDL2-static (check third_party/sdl2 checkout)")
endif()
if(TARGET SDL2::SDL2main)
    target_link_libraries(qianjs_ui_deps INTERFACE SDL2::SDL2main)
endif()
add_library(qianjs::ui_deps ALIAS qianjs_ui_deps)
