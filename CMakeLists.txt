# Copyright (C) 2017 xent
# Project is distributed under the terms of the GNU General Public License v3.0

project(halm-examples NONE)
cmake_minimum_required(VERSION 3.13)
cmake_policy(SET CMP0054 NEW)
include(ExternalProject)

# Build options
option(USE_LTO "Enable Link Time Optimization." OFF)

set(PATH_XCORE "${PROJECT_SOURCE_DIR}/xcore")
set(PATH_HALM "${PROJECT_SOURCE_DIR}/halm")

list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")
include(ListDirectories)

add_custom_target(size)

list_directories(BUNDLE_LIST "${PROJECT_SOURCE_DIR}/examples")
foreach(BUNDLE_NAME ${BUNDLE_LIST})
    set(FLAGS_BUNDLE "")
    list(APPEND FLAGS_BUNDLE -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE})
    list(APPEND FLAGS_BUNDLE -DUSE_LTO=${USE_LTO})

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
