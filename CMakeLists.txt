# Copyright (C) 2017 xent
# Project is distributed under the terms of the GNU General Public License v3.0

cmake_minimum_required(VERSION 3.21)
project(halm-examples NONE)
include(CheckLanguage)
include(ExternalProject)

# Build options
option(USE_BIN "Convert executables to Binary format." ON)
option(USE_HEX "Convert executables to Intel HEX format." OFF)
option(USE_DFU "Enable memory layout compatible with a bootloader." OFF)
option(USE_LTO "Enable Link Time Optimization." OFF)

option(TARGET_NOR "Use external NOR Flash for an artifact if available." OFF)
option(TARGET_SDRAM "Use external SDRAM for an artifact if available." OFF)
option(TARGET_SRAM "Use embedded SRAM for an artifact if available." OFF)

set(KCONFIG_DEFCONFIG "defconfig" CACHE STRING "Command that will be used as a Kconfig file generator.")
set(PATH_XCORE "${PROJECT_SOURCE_DIR}/xcore")
set(PATH_HALM "${PROJECT_SOURCE_DIR}/halm")

list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/xcore/cmake")
include(ListDirectories)

add_custom_target(size)

list_directories(BUNDLE_LIST "${PROJECT_SOURCE_DIR}/examples")
foreach(BUNDLE_NAME ${BUNDLE_LIST})
    if(${BUNDLE_NAME} STREQUAL "helpers")
        continue()
    endif()

    if(TARGET_NOR OR TARGET_SDRAM OR TARGET_SRAM)
        set(BUNDLE_TARGET_FILE ${PROJECT_SOURCE_DIR}/examples/${BUNDLE_NAME}/targets)
        if(EXISTS ${BUNDLE_TARGET_FILE})
            file(STRINGS ${BUNDLE_TARGET_FILE} CONFIG_TARGETS)
            list(FIND CONFIG_TARGETS "nor" TARGET_NOR_FOUND)
            list(FIND CONFIG_TARGETS "sdram" TARGET_SDRAM_FOUND)
            list(FIND CONFIG_TARGETS "sram" TARGET_SRAM_FOUND)

            if((TARGET_NOR AND TARGET_NOR_FOUND EQUAL -1)
                    OR (TARGET_SDRAM AND TARGET_SDRAM_FOUND EQUAL -1)
                    OR (TARGET_SRAM AND TARGET_SRAM_FOUND EQUAL -1))
                continue()
            endif()
        else()
            continue()
        endif()
    endif()

    set(BUNDLE_TOOLCHAIN_FILE ${PROJECT_SOURCE_DIR}/examples/${BUNDLE_NAME}/toolchain)
    set(FLAGS_BUNDLE "")

    if(EXISTS ${BUNDLE_TOOLCHAIN_FILE})
        file(STRINGS ${BUNDLE_TOOLCHAIN_FILE} CONFIG_TOOLCHAIN)
        list(GET CONFIG_TOOLCHAIN 0 CORE_TYPE)

        set(TOOLCHAIN_FILE_PATH "${PATH_XCORE}/toolchains/${CORE_TYPE}.cmake")
        file(STRINGS ${TOOLCHAIN_FILE_PATH} TOOLCHAIN_FILE_DATA)

        string(REGEX MATCH
                "CMAKE_C_COMPILER[\t ]+\"([a-zA-Z0-9_\-]+)\""
                TOOLCHAIN_COMPILER_UNUSED
                "${TOOLCHAIN_FILE_DATA}"
        )

        unset(TOOLCHAIN_EXISTS)
        find_program(TOOLCHAIN_EXISTS "${CMAKE_MATCH_1}" NO_CACHE)
        if("${TOOLCHAIN_EXISTS}" STREQUAL "TOOLCHAIN_EXISTS-NOTFOUND")
            message(STATUS "Toolchain ${CMAKE_MATCH_1} not found")
            continue()
        endif()

        list(APPEND FLAGS_BUNDLE "-DCMAKE_TOOLCHAIN_FILE=${TOOLCHAIN_FILE_PATH}")
    else()
        check_language(C)
        if("${CMAKE_C_COMPILER}" STREQUAL "NOTFOUND")
            message(STATUS "Default toolchain not found")
            continue()
        endif()
    endif()

    list(APPEND FLAGS_BUNDLE -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE})
    list(APPEND FLAGS_BUNDLE -DBUNDLE=${BUNDLE_NAME})
    list(APPEND FLAGS_BUNDLE -DUSE_BIN=${USE_BIN})
    list(APPEND FLAGS_BUNDLE -DUSE_HEX=${USE_HEX})
    list(APPEND FLAGS_BUNDLE -DUSE_DFU=${USE_DFU})
    list(APPEND FLAGS_BUNDLE -DUSE_LTO=${USE_LTO})
    list(APPEND FLAGS_BUNDLE -DTARGET_NOR=${TARGET_NOR})
    list(APPEND FLAGS_BUNDLE -DTARGET_SDRAM=${TARGET_SDRAM})
    list(APPEND FLAGS_BUNDLE -DTARGET_SRAM=${TARGET_SRAM})

    ExternalProject_Add(${BUNDLE_NAME}
            SOURCE_DIR ${PROJECT_SOURCE_DIR}/examples
            CMAKE_COMMAND ${CMAKE_COMMAND}
            CMAKE_ARGS ${FLAGS_BUNDLE}
            BINARY_DIR ${CMAKE_BINARY_DIR}/${BUNDLE_NAME}
            INSTALL_COMMAND ""
            BUILD_ALWAYS 1
    )

    set(BUNDLE_HALM_DEFCONFIG ${PROJECT_SOURCE_DIR}/examples/${BUNDLE_NAME}/halm.config)
    execute_process(
            COMMAND ${KCONFIG_DEFCONFIG} --kconfig ${PATH_HALM}/Kconfig ${BUNDLE_HALM_DEFCONFIG}
            WORKING_DIRECTORY ${PATH_HALM}
            OUTPUT_QUIET
    )
    file(RENAME ${PATH_HALM}/.config ${CMAKE_BINARY_DIR}/${BUNDLE_NAME}/halm.config)

    add_custom_target(${BUNDLE_NAME}_bundle ALL DEPENDS ${BUNDLE_NAME})
    add_custom_command(TARGET size
            POST_BUILD
            COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR}/${BUNDLE_NAME} --target size)
endforeach()
