# NanoVG (static) + OpenGL — used by canvas UI stack.

if(TARGET qianjs::nanovg)
    return()
endif()

get_filename_component(_qianjs_root "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE)
set(_nanovg_root "${_qianjs_root}/third_party/nanovg")
if(NOT EXISTS "${_nanovg_root}/src/nanovg.c")
    message(FATAL_ERROR "NanoVG missing at ${_nanovg_root}/src.\n  git clone https://github.com/memononen/nanovg.git third_party/nanovg")
endif()

add_library(qianjs_nanovg STATIC
    "${_nanovg_root}/src/nanovg.c"
    "${CMAKE_SOURCE_DIR}/src/platform/nanovg_backend.cc"
)
target_include_directories(qianjs_nanovg PUBLIC
    "${_nanovg_root}/src"
)
target_include_directories(qianjs_nanovg PRIVATE
    "${CMAKE_SOURCE_DIR}/src"
)
file(TO_CMAKE_PATH "${_nanovg_root}/example/Roboto-Regular.ttf" _font_path)
target_compile_definitions(qianjs_nanovg PUBLIC "QIANJS_DEFAULT_FONT=\"${_font_path}\"")

if(APPLE)
    find_library(QIANJS_OPENGL_FRAMEWORK OpenGL REQUIRED)
    target_link_libraries(qianjs_nanovg PUBLIC "${QIANJS_OPENGL_FRAMEWORK}")
elseif(WIN32)
    target_link_libraries(qianjs_nanovg PUBLIC opengl32)
else()
    find_package(OpenGL REQUIRED)
    target_link_libraries(qianjs_nanovg PUBLIC OpenGL::GL)
    # nanovg_backend.cc includes SDL_opengl.h (GL3); needs SDL2 include dirs.
    if(NOT TARGET qianjs::ui_deps)
        message(FATAL_ERROR "nanovg: include cmake/ui.cmake before nanovg.cmake on Linux")
    endif()
    target_link_libraries(qianjs_nanovg PRIVATE qianjs::ui_deps)
endif()

add_library(qianjs::nanovg ALIAS qianjs_nanovg)
