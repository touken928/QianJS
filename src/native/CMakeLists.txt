# Built-in native modules: sources for `qianjs` (options from native_modules.cmake in this directory, included by root).

if(QIANJS_MODULE_FS)
    target_sources(qianjs_impl PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/fs/fs_uv.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/fs/fs_ops.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/fs/fs_stat_js.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/fs/fs_sync.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/fs/fs_module.cc
    )
endif()

if(QIANJS_MODULE_CONSOLE)
    target_sources(qianjs_impl PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/console/console_module.cc
    )
endif()

if(QIANJS_MODULE_PROCESS)
    target_sources(qianjs_impl PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/process/process_module.cc
    )
endif()

if(QIANJS_MODULE_TIMERS)
    target_sources(qianjs_impl PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/timers/timers_module.cc
    )
endif()

if(QIANJS_MODULE_UI)
    target_sources(qianjs_impl PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/ui/ui_module.cc
    )
endif()

file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/generated")
qianjs_write_native_glue("${CMAKE_BINARY_DIR}/generated")
