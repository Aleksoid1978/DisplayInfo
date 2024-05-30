cmake_minimum_required(VERSION 3.14)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

cmake_policy(SET CMP0091 NEW)

project("DisplayInfo" VERSION "0.0.1" LANGUAGES CXX)

set(CMAKE_CONFIGURATION_TYPES
    "Debug"
    "Release"
    CACHE STRING "" FORCE
)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING
        "Choose the type of build, options are: Debug Release."
        FORCE)
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
    option(STATIC_MSVC_CRT "Static MSVC runtimes" ON)
endif()

add_executable(${PROJECT_NAME} DisplayInfo.cpp "${CMAKE_CURRENT_SOURCE_DIR}/DisplayConfig/DisplayConfig.cpp")
target_link_libraries(${PROJECT_NAME} PRIVATE Shcore)

if(MSVC)
    if(STATIC_MSVC_CRT)
        set_target_properties(${PROJECT_NAME} PROPERTIES
            MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>"
        )
    endif()

    set(list_of_delay_dlls
        api-ms-win-shcore-scaling-l1-1-1.dll
        user32.dll
    )
    foreach(dll_name ${list_of_delay_dlls})
        set(delayload_dlls_list "${delayload_dlls_list} /DELAYLOAD:${dll_name}")
    endforeach()

    set_target_properties(${PROJECT_NAME} PROPERTIES LINK_FLAGS "${delayload_dlls_list}")
    set_target_properties(${PROJECT_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY_RELEASE ${PROJECT_BINARY_DIR}/$<0:>)
    set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME})
endif()