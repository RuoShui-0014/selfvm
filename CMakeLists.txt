cmake_minimum_required(VERSION 3.1)

project(self-vm)

set(CMAKE_CXX_STANDARD 20)

if (CMAKE_BUILD MATCHES "Debug")
    add_definitions(-DDEBUG)
endif ()

if (MSVC)
    add_compile_options(/MD)
    add_compile_options(/wd4275 /wd4251 /GR-)
    if (CMAKE_BUILD MATCHES "Release")
        add_compile_options(/O2)
    endif ()
    add_compile_definitions(NOMINMAX)
endif ()

include_directories(${CMAKE_JS_INC})
#include_directories(C:/Users/ruosh/.cmake-js/node-x64/v22.17.0/include/node)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_JS_BIN})

add_library(${PROJECT_NAME} SHARED
        src/addon.cpp

        src/utils/utils.h
        src/utils/utils.h

        src/lib/thread_pool.cpp
        src/lib/thread_pool.h
        #        src/lib/logger.cpp
        #        src/lib/logger.h

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

        src/module/isolate_handle.cpp
        src/module/isolate_handle.h
        src/module/context_handle.cpp
        src/module/context_handle.h
        src/web/local_dom_window.cpp
        src/web/local_dom_window.h
)
target_compile_definitions(${PROJECT_NAME} PRIVATE _UCRT)
target_link_libraries(${PROJECT_NAME} ${CMAKE_JS_LIB})
#target_link_libraries(${PROJECT_NAME} E:/src/node-v22.17.0/Release/lib/v8_base_without_compiler.lib)
#target_link_libraries(${PROJECT_NAME} E:/src/node-v22.17.0/Release/node.lib)

set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "" SUFFIX ".node")

set(TARGET_COPY_DIR ${PROJECT_SOURCE_DIR}/out)  # 修改为你想要复制到的目录

# 添加自定义命令在构建后复制文件
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        $<TARGET_FILE:${PROJECT_NAME}>
        ${TARGET_COPY_DIR}
        COMMENT "Copying ${PROJECT_NAME} to ${TARGET_COPY_DIR}"
)