cmake_minimum_required(VERSION 3.5)

project(self-vm)

set(CMAKE_CXX_STANDARD 20)

if(MSVC)
    add_compile_options(/wd4275 /wd4251)
endif()

include_directories(${CMAKE_JS_INC})
#include_directories(C:/Users/ruosh/.cmake-js/node-x64/v22.17.0/include/node)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_JS_BIN})

add_library(${PROJECT_NAME} SHARED
        src/addon.cpp
        src/platform_delegate.cpp
        src/platform_delegate.h
        src/utils/script_wrappable.cpp
        src/utils/script_wrappable.h
        src/utils/utils.h
        src/utils/utils.h
        src/isolate_handle/isolate_handle.cpp
        src/isolate_handle/isolate_handle.h
        src/isolate_handle/context_handle.cpp
        src/isolate_handle/context_handle.h
)

target_link_libraries(${PROJECT_NAME} ${CMAKE_JS_LIB})
#target_link_libraries(${PROJECT_NAME} C:/Users/ruosh/.cmake-js/node-x64/v22.17.0/win-x64/node.lib)

set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "" SUFFIX ".node")