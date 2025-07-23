cmake_minimum_required(VERSION 3.10)

set(PROJECT_NAME self-vm)
project(${PROJECT_NAME} VERSION 0.1.0 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)

add_library(${PROJECT_NAME} SHARED)

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
        target_compile_options(${PROJECT_NAME} PRIVATE /O2)
    endif ()
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    target_compile_options(${PROJECT_NAME} PRIVATE
        -Wno-unused-result
    )
    if (CMAKE_BUILD_TYPE MATCHES "Debug")
        target_compile_definitions(${PROJECT_NAME} PRIVATE DEBUG)
    elseif (CMAKE_BUILD_TYPE MATCHES "Release")
        target_compile_options(${PROJECT_NAME} PRIVATE -O2)
    endif ()
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "GUN")
    target_compile_options(${PROJECT_NAME} PRIVATE
        -Wno-unused-result
    )
endif ()

target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_JS_INC})

target_sources(${PROJECT_NAME} PRIVATE
    src/addon.cpp

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

    src/module/string.h
    src/module/isolate_handle.cpp
    src/module/isolate_handle.h
    src/module/context_handle.cpp
    src/module/context_handle.h
    src/module/session_handle.cpp
    src/module/session_handle.h
    src/module/script_handle.cpp
    src/module/script_handle.h

    src/web/local_dom_window.cpp
    src/web/local_dom_window.h
)
target_link_libraries(${PROJECT_NAME} PRIVATE ${CMAKE_JS_LIB})

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