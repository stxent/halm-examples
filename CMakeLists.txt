# Copyright (C) 2017 xent
# Project is distributed under the terms of the GNU General Public License v3.0

project(halm-examples NONE)
cmake_minimum_required(VERSION 3.6)
include(ExternalProject)

# Build options
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build: Debug Release." FORCE)
endif()

option(USE_LTO "Enable Link Time Optimization." OFF)

set(PATH_XCORE "${PROJECT_SOURCE_DIR}/xcore")
set(PATH_HALM "${PROJECT_SOURCE_DIR}/halm")

macro(list_directories SEARCH_RESULT DIRECTORY_PATH)
    file(GLOB ENTRY_LIST RELATIVE ${DIRECTORY_PATH} ${DIRECTORY_PATH}/*)
    set(DIRECTORY_LIST "")
    foreach(ENTRY ${ENTRY_LIST})
        if(IS_DIRECTORY ${DIRECTORY_PATH}/${ENTRY})
            string(FIND "${ENTRY}" "." ENTRY_IS_INVISIBLE)
            if(NOT ${ENTRY_IS_INVISIBLE} EQUAL 0)
                message(STATUS "Added target ${ENTRY}")
                list(APPEND DIRECTORY_LIST ${ENTRY})
            else()
                message(STATUS "Skipped directory ${ENTRY}")
            endif()
        endif()
    endforeach()
    set(${SEARCH_RESULT} ${DIRECTORY_LIST})
endmacro()

list_directories(BUNDLE_LIST "${PROJECT_SOURCE_DIR}/examples")

add_custom_target(size)

foreach(BUNDLE_NAME ${BUNDLE_LIST})
    set(FLAGS_BUNDLE "")
    list(APPEND FLAGS_BUNDLE "-DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}")
    list(APPEND FLAGS_BUNDLE "-DUSE_LTO=${USE_LTO}")

    set(BUNDLE_TOOLCHAIN_FILE ${PROJECT_SOURCE_DIR}/examples/${BUNDLE_NAME}/toolchain)
    if(EXISTS ${BUNDLE_TOOLCHAIN_FILE})
        file(STRINGS ${BUNDLE_TOOLCHAIN_FILE} CONFIG_TOOLCHAIN)
        list(GET CONFIG_TOOLCHAIN 0 CORE_TYPE)
        list(APPEND FLAGS_BUNDLE "-DCMAKE_TOOLCHAIN_FILE=${PATH_XCORE}/toolchains/${CORE_TYPE}.cmake")
    endif()

    ExternalProject_Add(${BUNDLE_NAME}
            SOURCE_DIR ${PROJECT_SOURCE_DIR}/examples/${BUNDLE_NAME}
            CMAKE_COMMAND ${CMAKE_COMMAND}
            CMAKE_ARGS ${FLAGS_BUNDLE}
            BINARY_DIR ${CMAKE_BINARY_DIR}/${BUNDLE_NAME}
            INSTALL_COMMAND ""
            BUILD_ALWAYS 1
    )

    add_custom_target(${BUNDLE_NAME}_bundle ALL DEPENDS ${BUNDLE_NAME})
    add_custom_command(TARGET size POST_BUILD
            COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR}/${BUNDLE_NAME} --target size)
endforeach()
