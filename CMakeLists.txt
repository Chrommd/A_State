cmake_minimum_required(VERSION 3.10)
project(A_State CXX C)

set(CMAKE_CXX_STANDARD 17)

if(MSVC)
    foreach(flag_var CMAKE_CXX_FLAGS CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_RELEASE CMAKE_C_FLAGS CMAKE_C_FLAGS_DEBUG CMAKE_C_FLAGS_RELEASE)
        string(REPLACE "/MD" "/MT" ${flag_var} "${${flag_var}}")
        string(REPLACE "/MDd" "/MTd" ${flag_var} "${${flag_var}}")
    endforeach()
endif()

add_subdirectory(MinHook)

add_library(A_State SHARED
    dll/dllmain.cpp
    dll/init.cpp
    dll/logger.cpp
    dll/hooks.cpp
    dll/lua_interface.cpp
    dll/console.cpp
)

target_include_directories(A_State PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/MinHook/include
    ${CMAKE_CURRENT_SOURCE_DIR}/dll
)

target_link_libraries(A_State PRIVATE kernel32 user32 minhook)

if(MSVC)
    set_target_properties(A_State PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY_DEBUG   "${CMAKE_BINARY_DIR}"
        RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}"
    )
endif()

add_executable(A_State_Injector
    injector/main.cpp
    injector/injector.cpp
    injector/process_utils.cpp
)

target_include_directories(A_State_Injector PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/injector
)

if(MSVC)
    set_target_properties(A_State_Injector PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY_DEBUG   "${CMAKE_BINARY_DIR}"
        RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}"
        LINK_FLAGS "/MANIFESTUAC:\"level='requireAdministrator' uiAccess='false'\""
    )
endif()
