cmake_minimum_required(VERSION 3.10)

set(PROJECT_NAME self-vm)
project(${PROJECT_NAME} VERSION 0.1.0 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)

add_library(${PROJECT_NAME} SHARED
    src/isolate/scheduler_node.cpp
    src/isolate/scheduler_node.h
    src/isolate/scheduler_self.cpp
    src/isolate/scheduler_self.h)

target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_JS_INC})
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)

target_sources(${PROJECT_NAME} PRIVATE
    src/addon.cpp

    src/base/raw_ptr.h
    src/base/logger.cpp
    src/base/logger.h

    src/utils/utils.h
    src/utils/utils.h

    src/isolate/platform_delegate.cpp
    src/isolate/platform_delegate.h
    src/isolate/isolate_holder.cpp
    src/isolate/isolate_holder.h
    src/isolate/scheduler.cpp
    src/isolate/scheduler.h
    src/isolate/script_wrappable.cpp
    src/isolate/script_wrappable.h
    src/isolate/per_isolate_data.cpp
    src/isolate/per_isolate_data.h
    src/isolate/wrapper_type_info.cpp
    src/isolate/wrapper_type_info.h
    src/isolate/external_data.cpp
    src/isolate/external_data.h
    src/isolate/task.cpp
    src/isolate/task.h
    src/isolate/inspector_agent.cpp
    src/isolate/inspector_agent.h

    src/utils/string.h
    src/module/isolate_handle.cpp
    src/module/isolate_handle.h
    src/module/context_handle.cpp
    src/module/context_handle.h
    src/module/session_handle.cpp
    src/module/session_handle.h
    src/module/script_handle.cpp
    src/module/script_handle.h
    src/module/snapshot_handle.cpp
    src/module/snapshot_handle.h

    src/web/local_dom_window.cpp
    src/web/local_dom_window.h

    src/net/request.cpp
    src/net/request.h
    src/net/tcp_stream.cpp
    src/net/tcp_stream.h

    src/native/timer_manager.cpp
    src/native/timer_manager.h

    src/tool/tools.cpp
    src/tool/tools.h
    src/tool/utils.cpp
    src/tool/utils.h
#    src/tool/interface/window.cc
#    src/tool/interface/window.h
#    src/tool/interface/event_target.cc
#    src/tool/interface/event_target.h
)
target_link_libraries(${PROJECT_NAME} PRIVATE ${CMAKE_JS_LIB})

if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
    target_compile_definitions(${PROJECT_NAME} PUBLIC OS_LINUX)
elseif (CMAKE_SYSTEM_NAME STREQUAL "Windows")
    target_compile_definitions(${PROJECT_NAME} PUBLIC OS_WIN)
elseif (CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    target_compile_definitions(${PROJECT_NAME} PUBLIC OS_MAC)
endif ()

if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    target_compile_options(${PROJECT_NAME} PRIVATE
        /GR-
        /EHsc-
        /wd4275
        /wd4251
        /wd4834
    )
    if (CMAKE_BUILD_TYPE MATCHES "Debug")
        target_compile_definitions(${PROJECT_NAME} PRIVATE DEBUG)
    elseif (CMAKE_BUILD_TYPE MATCHES "Release")
        target_compile_options(${PROJECT_NAME} PRIVATE /Ox)
    endif ()
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    target_compile_options(${PROJECT_NAME} PRIVATE
        -Wno-unused-result
    )
    if (CMAKE_BUILD_TYPE MATCHES "Debug")
        target_compile_definitions(${PROJECT_NAME} PRIVATE DEBUG)
    elseif (CMAKE_BUILD_TYPE MATCHES "Release")
        target_compile_options(${PROJECT_NAME} PRIVATE -Ox)
    endif ()
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(${PROJECT_NAME} PRIVATE
        -Wno-unused-result
    )
    if (CMAKE_BUILD_TYPE MATCHES "Debug")
        target_compile_definitions(${PROJECT_NAME} PRIVATE DEBUG)
    elseif (CMAKE_BUILD_TYPE MATCHES "Release")
        target_compile_options(${PROJECT_NAME} PRIVATE -O3)
    endif ()
endif ()

set_target_properties(${PROJECT_NAME} PROPERTIES
    PREFIX ""
    SUFFIX ".node"
)

set(TARGET_COPY_DIR ${PROJECT_SOURCE_DIR}/bin)
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
    $<TARGET_FILE:${PROJECT_NAME}>
    ${TARGET_COPY_DIR}
    COMMENT "Copying ${PROJECT_NAME} to ${TARGET_COPY_DIR}"
)