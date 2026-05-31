cmake_minimum_required(VERSION 3.16)

project(qianjs LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(BUILD_SHARED_LIBS OFF)

# 输出目录
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# 平台检测
if(WIN32)
    set(PLATFORM_WINDOWS TRUE)
elseif(APPLE)
    set(PLATFORM_MACOS TRUE)
elseif(UNIX)
    set(PLATFORM_LINUX TRUE)
endif()

option(QIANJS_BUILD_TESTS "Build QianJS tests (GoogleTest via FetchContent)" ON)

if(QIANJS_BUILD_TESTS)
    enable_testing()
    include(FetchContent)
    set(gtest_force_shared_crt OFF CACHE BOOL "" FORCE)
    set(BUILD_GMOCK OFF CACHE BOOL "" FORCE)
    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG v1.14.0
        GIT_SHALLOW TRUE
    )
    FetchContent_MakeAvailable(googletest)
    add_library(qianjs_gtest_main INTERFACE)
    target_link_libraries(qianjs_gtest_main INTERFACE GTest::gtest_main)
    add_library(qianjs::gtest_main ALIAS qianjs_gtest_main)
endif()

include(cmake/qjs.cmake)

option(QIANJS_BUILD_CLI "Build the qianjs command-line executable" ON)

if(QIANJS_BUILD_CLI)
    include(${CMAKE_CURRENT_SOURCE_DIR}/src/native/native_modules.cmake)
    include(cmake/libuv.cmake)
    include(cmake/uvw.cmake)

    find_package(Threads REQUIRED)

    add_library(qianjs_impl STATIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src/app/application.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/src/app/cli.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/src/app/cli_usage.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/src/app/commands/build_command.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/src/app/commands/embed_command.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/src/app/commands/run_command.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/src/runtime/event_loop/event_loop.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/src/runtime/plugin_lifecycle.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/src/runtime/promise_registry.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/src/runtime/scheduler.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/src/runtime/instance.cc
    )

    target_include_directories(qianjs_impl PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_BINARY_DIR}/generated
    )

    target_link_libraries(qianjs_impl PUBLIC
        qjs::qjs
        Threads::Threads
    )

    # Native catalog, generated glue, platform/ui stack (see src/native/native_modules.cmake).
    add_subdirectory(src/native)

    target_compile_definitions(qianjs_impl PUBLIC
        "QIANJS_HAVE_LIBUV=$<BOOL:${QIANJS_NATIVE_NEED_LIBUV}>"
    )
    if(QIANJS_NATIVE_NEED_LIBUV)
        target_link_libraries(qianjs_impl PUBLIC qianjs::libuv qianjs::uvw)
    endif()
    if(QIANJS_NATIVE_NEED_UI)
        include(cmake/ui.cmake)
        include(cmake/nanovg.cmake)
        target_link_libraries(qianjs_impl PRIVATE qianjs::ui_deps qianjs::nanovg)
    endif()

    add_executable(qianjs
        ${CMAKE_CURRENT_SOURCE_DIR}/src/app/main.cc
    )
    target_link_libraries(qianjs PRIVATE qianjs_impl)

    if(NOT APPLE)
        target_link_options(qianjs PRIVATE -static-libgcc -static-libstdc++)
        if(WIN32)
            target_link_options(qianjs PRIVATE -static)
        endif()
    endif()

    # 性能优化和二进制压缩
    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        target_compile_options(qianjs_impl PRIVATE -O3 -DNDEBUG -ffunction-sections -fdata-sections)
        if(APPLE)
            target_link_options(qianjs PRIVATE -Wl,-dead_strip)
        else()
            target_link_options(qianjs PRIVATE -Wl,--gc-sections -s)
        endif()
    endif()

    if(WIN32)
        if(MSVC)
            message(FATAL_ERROR "MSVC is not supported. Please use MinGW-w64 or Clang on Windows.")
        endif()
        target_compile_definitions(qianjs_impl PUBLIC _CRT_SECURE_NO_WARNINGS)
    endif()
endif()

if(QIANJS_BUILD_TESTS)
    add_subdirectory(tests)
endif()
