# Copyright (C) 2017 xent
# Project is distributed under the terms of the GNU General Public License v3.0

project(examples C)
cmake_minimum_required(VERSION 3.6)
include(ExternalProject)

# Project platform must be set correctly
set(PLATFORM "LPC13XX")

# Build options
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build: Debug Release." FORCE)
endif()

option(USE_LTO "Enable Link Time Optimization." OFF)

set(PATH_XCORE "${PROJECT_SOURCE_DIR}/../../xcore" CACHE STRING "Path to the XCORE library.")
set(PATH_HALM "${PROJECT_SOURCE_DIR}/../../halm" CACHE STRING "Path to the HALM library.")

# Default compiler flags
set(FLAGS_DEFAULT "-fdata-sections -ffunction-sections -fmessage-length=0 -Wall -Wextra -Wshadow -pedantic")
SET(FLAGS_LINKER "--specs=nosys.specs --specs=nano.specs -Xlinker --gc-sections -T ${PROJECT_SOURCE_DIR}/memory.ld")
set(FLAGS_PLATFORM "-D${PLATFORM}")
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${FLAGS_DEFAULT} ${FLAGS_CPU} ${FLAGS_PLATFORM}")
SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${FLAGS_LINKER}")
set(CMAKE_SHARED_LIBRARY_LINK_C_FLAGS "")

if(${USE_LTO})
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -flto -ffat-lto-objects")
endif()

include_directories("${PATH_XCORE}/include")
include_directories("${PATH_HALM}/include")

# Subproject settings
set(FLAGS_SUBPROJECT "")
list(APPEND FLAGS_SUBPROJECT "-DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}")
list(APPEND FLAGS_SUBPROJECT "-DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}")
list(APPEND FLAGS_SUBPROJECT "-DUSE_LTO=${USE_LTO}")

# Configure external libraries
ExternalProject_Add(xcore
        SOURCE_DIR ${PATH_XCORE}
        CMAKE_COMMAND ${CMAKE_COMMAND}
        CMAKE_ARGS ${FLAGS_SUBPROJECT}
        BINARY_DIR ${CMAKE_BINARY_DIR}/xcore
        INSTALL_COMMAND ""
        BUILD_ALWAYS 1
)

ExternalProject_Add(halm
        SOURCE_DIR ${PATH_HALM}
        CMAKE_COMMAND ${CMAKE_COMMAND}
        CMAKE_ARGS
                ${FLAGS_SUBPROJECT}
                -DPLATFORM=${PLATFORM}
                -DPATH_XCORE=${PATH_XCORE}
                -DCONFIG_FILE=${PROJECT_SOURCE_DIR}/halm_config
        BINARY_DIR ${CMAKE_BINARY_DIR}/halm
        INSTALL_COMMAND ""
        BUILD_ALWAYS 1
)

# Build examples
list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/../../cmake")
message(${CMAKE_MODULE_PATH})
include(ListDirectories)

add_custom_target(size)

list_directories(EXAMPLES_LIST "${PROJECT_SOURCE_DIR}")
foreach(EXAMPLE_NAME ${EXAMPLES_LIST})
    file(GLOB_RECURSE EXAMPLE_SOURCES "${EXAMPLE_NAME}/*.c")
    add_executable("${EXAMPLE_NAME}.elf" ${EXAMPLE_SOURCES})

    target_link_libraries("${EXAMPLE_NAME}.elf" "${CMAKE_BINARY_DIR}/halm/libhalm.a")
    add_dependencies("${EXAMPLE_NAME}.elf" halm)
    target_link_libraries("${EXAMPLE_NAME}.elf" "${CMAKE_BINARY_DIR}/xcore/libxcore.a")
    add_dependencies("${EXAMPLE_NAME}.elf" xcore)

    add_custom_command(TARGET "${EXAMPLE_NAME}.elf"
            POST_BUILD
            COMMAND "${CMAKE_OBJCOPY}" "${EXAMPLE_NAME}.elf" -Oihex "${EXAMPLE_NAME}.hex"
    )

    add_dependencies(size "${EXAMPLE_NAME}.elf")
    add_custom_command(TARGET size POST_BUILD COMMAND "${CMAKE_SIZE}" "${EXAMPLE_NAME}.elf")
endforeach()
