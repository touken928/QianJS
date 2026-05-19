# QianJS native module catalog — single source of truth for options, sources, glue, and deps.
# Include from root CMakeLists.txt before add_subdirectory(src/native).

# -----------------------------------------------------------------------------
# Profile (SKU): sets which modules are ON. Empty / "custom" = per-module options.
# -----------------------------------------------------------------------------
set(QIANJS_PROFILE "" CACHE STRING "Module bundle: minimal, io, desktop, or empty for custom")
set_property(CACHE QIANJS_PROFILE PROPERTY STRINGS "" minimal io desktop custom)

# -----------------------------------------------------------------------------
# Module catalog (register once at configure time)
# -----------------------------------------------------------------------------
macro(qianjs_module NAME)
    set(_qm_SOURCES)
    set(_qm_DEPS)
    set(_qm_REQUIRES)
    cmake_parse_arguments(_qm "" "CLASS;HEADER" "SOURCES;DEPS;REQUIRES" ${ARGN})
    if(NOT _qm_CLASS OR NOT _qm_HEADER)
        message(FATAL_ERROR "qianjs_module(${NAME}): CLASS and HEADER are required")
    endif()
    set_property(GLOBAL APPEND PROPERTY QIANJS_MODULE_NAMES "${NAME}")
    set_property(GLOBAL PROPERTY QIANJS_MOD_${NAME}_CLASS "${_qm_CLASS}")
    set_property(GLOBAL PROPERTY QIANJS_MOD_${NAME}_HEADER "${_qm_HEADER}")
    set_property(GLOBAL PROPERTY QIANJS_MOD_${NAME}_SOURCES "${_qm_SOURCES}")
    set_property(GLOBAL PROPERTY QIANJS_MOD_${NAME}_DEPS "${_qm_DEPS}")
    set_property(GLOBAL PROPERTY QIANJS_MOD_${NAME}_REQUIRES "${_qm_REQUIRES}")
    string(TOUPPER "${NAME}" _up)
    set(_default OFF)
    if(NAME STREQUAL "console" OR NAME STREQUAL "process" OR NAME STREQUAL "timers" OR NAME STREQUAL "fs")
        set(_default ON)
    endif()
    option(QIANJS_MODULE_${_up} "QianJS module: ${NAME}" ${_default})
endmacro()

function(_qianjs_profile_default_modules PROFILE OUT_VAR)
    if(PROFILE STREQUAL "minimal")
        set(${OUT_VAR} console process PARENT_SCOPE)
    elseif(PROFILE STREQUAL "io")
        set(${OUT_VAR} console process timers fs PARENT_SCOPE)
    elseif(PROFILE STREQUAL "desktop")
        set(${OUT_VAR} console process timers fs ui app PARENT_SCOPE)
    else()
        set(${OUT_VAR} "" PARENT_SCOPE)
    endif()
endfunction()

function(_qianjs_apply_profile_to_options)
    if(NOT QIANJS_PROFILE OR QIANJS_PROFILE STREQUAL "custom")
        return()
    endif()
    _qianjs_profile_default_modules("${QIANJS_PROFILE}" _profile_mods)
    if(NOT _profile_mods)
        message(FATAL_ERROR "Unknown QIANJS_PROFILE='${QIANJS_PROFILE}' (use minimal, io, desktop, or custom)")
    endif()
    get_property(_all GLOBAL PROPERTY QIANJS_MODULE_NAMES)
    foreach(_name ${_all})
        string(TOUPPER "${_name}" _up)
        if(_name IN_LIST _profile_mods)
            set(QIANJS_MODULE_${_up} ON CACHE BOOL "" FORCE)
        else()
            set(QIANJS_MODULE_${_up} OFF CACHE BOOL "" FORCE)
        endif()
    endforeach()
endfunction()

function(_qianjs_module_enabled NAME OUT_VAR)
    string(TOUPPER "${NAME}" _up)
    if(QIANJS_MODULE_${_up})
        set(${OUT_VAR} ON PARENT_SCOPE)
    else()
        set(${OUT_VAR} OFF PARENT_SCOPE)
    endif()
endfunction()

# Enabled = module options (or profile) + transitive DEPS closure (e.g. app enables ui).
function(_qianjs_resolve_enabled_modules OUT_ENABLED)
    get_property(_all GLOBAL PROPERTY QIANJS_MODULE_NAMES)
    set(_enabled "")
    foreach(_name ${_all})
        _qianjs_module_enabled("${_name}" _on)
        if(_on)
            list(APPEND _enabled ${_name})
        endif()
    endforeach()
    set(_changed TRUE)
    while(_changed)
        set(_changed FALSE)
        foreach(_name ${_enabled})
            get_property(_deps GLOBAL PROPERTY QIANJS_MOD_${_name}_DEPS)
            foreach(_dep ${_deps})
                if(NOT _dep IN_LIST _enabled)
                    list(APPEND _enabled ${_dep})
                    string(TOUPPER "${_dep}" _dup)
                    set(QIANJS_MODULE_${_dup} ON CACHE BOOL "" FORCE)
                    set(_changed TRUE)
                    message(STATUS "qianjs: enabled '${_dep}' (required by '${_name}')")
                endif()
            endforeach()
        endforeach()
    endwhile()
    set(${OUT_ENABLED} ${_enabled} PARENT_SCOPE)
endfunction()

function(_qianjs_topo_sort_modules ENABLED_IN OUT_SORTED)
    set(_sorted "")
    set(_remaining ${ENABLED_IN})
    set(_guard 0)
    while(_remaining)
        math(EXPR _guard "${_guard} + 1")
        if(_guard GREATER 32)
            message(FATAL_ERROR "qianjs: module dependency cycle detected")
        endif()
        set(_ready "")
        foreach(_name ${_remaining})
            get_property(_deps GLOBAL PROPERTY QIANJS_MOD_${_name}_DEPS)
            set(_ok TRUE)
            foreach(_dep ${_deps})
                if(_dep IN_LIST _remaining)
                    set(_ok FALSE)
                endif()
            endforeach()
            if(_ok)
                list(APPEND _ready ${_name})
            endif()
        endforeach()
        if(NOT _ready)
            message(FATAL_ERROR "qianjs: module dependency cycle among: ${_remaining}")
        endif()
        list(APPEND _sorted ${_ready})
        foreach(_r ${_ready})
            list(REMOVE_ITEM _remaining ${_r})
        endforeach()
    endwhile()
    set(${OUT_SORTED} ${_sorted} PARENT_SCOPE)
endfunction()

function(_qianjs_write_generated_headers ENABLED_SORTED GEN_DIR PROFILE)
    set(_defs "#pragma once\n/* Generated by native_modules.cmake — do not edit. */\n\n")
    string(APPEND _defs "#define QIANJS_BUILD_PROFILE \"${PROFILE}\"\n\n")

    set(_mods_list "")
    set(_glue "#pragma once\n/* Generated by native_modules.cmake — do not edit. */\n#include <qianjs_modules.h>\n#include <js_plugin.h>\n\n")

    get_property(_all GLOBAL PROPERTY QIANJS_MODULE_NAMES)
    foreach(_name ${_all})
        string(TOUPPER "${_name}" _up)
        if(_name IN_LIST ENABLED_SORTED)
            string(APPEND _defs "#define QIANJS_MODULE_${_up} 1\n")
            if(_mods_list)
                string(APPEND _mods_list ",")
            endif()
            string(APPEND _mods_list "${_name}")
        else()
            string(APPEND _defs "#define QIANJS_MODULE_${_up} 0\n")
        endif()
    endforeach()

    string(APPEND _defs "\n#define QIANJS_BUILD_MODULES \"${_mods_list}\"\n")

    set(_glue_h "${_glue}void qianjs_populate_default_plugins(qjs::PluginRegistry& r);\n")
    set(_glue_cc "/* Generated by native_modules.cmake — do not edit. */\n#include <qianjs_default_plugins.g.h>\n\n")

    foreach(_name ${ENABLED_SORTED})
        string(TOUPPER "${_name}" _up)
        get_property(_hdr GLOBAL PROPERTY QIANJS_MOD_${_name}_HEADER)
        string(APPEND _glue_cc "#if QIANJS_MODULE_${_up}\n#include \"${_hdr}\"\n#endif\n")
    endforeach()

    string(APPEND _glue_cc "\nvoid qianjs_populate_default_plugins(qjs::PluginRegistry& r) {\n")
    foreach(_name ${ENABLED_SORTED})
        string(TOUPPER "${_name}" _up)
        get_property(_class GLOBAL PROPERTY QIANJS_MOD_${_name}_CLASS)
        string(APPEND _glue_cc "#if QIANJS_MODULE_${_up}\n  r.emplace<${_class}>();\n#endif\n")
    endforeach()
    string(APPEND _glue_cc "}\n")

    file(MAKE_DIRECTORY "${GEN_DIR}")
    file(WRITE "${GEN_DIR}/qianjs_modules.h" "${_defs}")
    file(WRITE "${GEN_DIR}/qianjs_default_plugins.g.h" "${_glue_h}")
    file(WRITE "${GEN_DIR}/qianjs_default_plugins.g.cc" "${_glue_cc}")
    set(QIANJS_GENERATED_PLUGINS_CC "${GEN_DIR}/qianjs_default_plugins.g.cc" PARENT_SCOPE)
endfunction()

# Attach cataloged modules to TARGET; sets QIANJS_NATIVE_NEED_LIBUV / NEED_UI in parent scope.
function(qianjs_native_attach TARGET)
    if(NOT TARGET ${TARGET})
        message(FATAL_ERROR "qianjs_native_attach: target '${TARGET}' does not exist")
    endif()

    _qianjs_apply_profile_to_options()
    _qianjs_resolve_enabled_modules(_enabled)
    _qianjs_topo_sort_modules("${_enabled}" _sorted)

    set(_need_libuv OFF)
    set(_need_ui OFF)
    set(_src_root "${CMAKE_SOURCE_DIR}/src/native")

    foreach(_name ${_enabled})
        get_property(_sources GLOBAL PROPERTY QIANJS_MOD_${_name}_SOURCES)
        get_property(_requires GLOBAL PROPERTY QIANJS_MOD_${_name}_REQUIRES)
        if(_sources)
            set(_abs_sources "")
            foreach(_rel ${_sources})
                list(APPEND _abs_sources "${_src_root}/${_rel}")
            endforeach()
            target_sources(${TARGET} PRIVATE ${_abs_sources})
        endif()
        if("LIBUV" IN_LIST _requires)
            set(_need_libuv ON)
        endif()
        if("UI_STACK" IN_LIST _requires)
            set(_need_ui ON)
        endif()
    endforeach()

    if(_need_ui)
        _qianjs_attach_ui_stack(${TARGET})
    endif()

    _qianjs_write_generated_headers("${_sorted}" "${CMAKE_BINARY_DIR}/generated" "${QIANJS_PROFILE}")
    target_sources(${TARGET} PRIVATE "${QIANJS_GENERATED_PLUGINS_CC}")

    set(QIANJS_NATIVE_NEED_LIBUV ${_need_libuv} PARENT_SCOPE)
    set(QIANJS_NATIVE_NEED_UI ${_need_ui} PARENT_SCOPE)
    set(QIANJS_NATIVE_ENABLED_MODULES "${_sorted}" PARENT_SCOPE)

    message(STATUS "qianjs profile='${QIANJS_PROFILE}' modules='${_sorted}' libuv=${_need_libuv} ui=${_need_ui}")
endfunction()

# --- catalog (defaults OFF; profile or -DQIANJS_MODULE_* turns modules on) ---
set_property(GLOBAL PROPERTY QIANJS_MODULE_NAMES "")

qianjs_module(console
    CLASS ConsolePlugin
    HEADER native/console/console_module.h
    SOURCES console/console_module.cc
)

qianjs_module(process
    CLASS ProcessPlugin
    HEADER native/process/process_module.h
    SOURCES process/process_module.cc
)

qianjs_module(timers
    CLASS TimersPlugin
    HEADER native/timers/timers_module.h
    SOURCES timers/timers_module.cc
    REQUIRES LIBUV
)

qianjs_module(fs
    CLASS FsPlugin
    HEADER native/fs/fs_module.h
    SOURCES
        fs/fs_module.cc
        fs/fs_uv.cc
        fs/fs_ops.cc
        fs/fs_stat_js.cc
        fs/fs_sync.cc
    REQUIRES LIBUV
)

qianjs_module(ui
    CLASS UiPlugin
    HEADER native/ui/ui_module.h
    SOURCES ui/ui_module.cc
    REQUIRES UI_STACK
)

qianjs_module(app
    CLASS AppPlugin
    HEADER native/app/app_module.h
    SOURCES app/app_module.cc
    DEPS ui
    REQUIRES UI_STACK
)

# Platform / frame loop (part of UI_STACK, not a JS plugin)
set(QIANJS_UI_STACK_SOURCES
    platform/draw_list.cc
    platform/platform_window.cc
    runtime/app_host.cc
    runtime/clock.cc
    runtime/frame_loop.cc
    systems/input_system.cc
    systems/render_system.cc
)

function(_qianjs_attach_ui_stack TARGET)
    set(_root "${CMAKE_SOURCE_DIR}/src")
    set(_paths "")
    foreach(_rel ${QIANJS_UI_STACK_SOURCES})
        list(APPEND _paths "${_root}/${_rel}")
    endforeach()
    target_sources(${TARGET} PRIVATE ${_paths})
endfunction()

# Hook called from qianjs_native_attach when UI stack needed
