# Copyright (C) 2017 xent
# Project is distributed under the terms of the GNU General Public License v3.0

project(examples C)
cmake_minimum_required(VERSION 3.6)

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
set(FLAGS_DEFAULT "-fdata-sections -ffunction-sections -Wall -Wextra -Wshadow -pedantic")
set(FLAGS_LINKER "--specs=nosys.specs --specs=nano.specs -Xlinker --gc-sections -T \"${PROJECT_SOURCE_DIR}/memory.ld\"")
add_definitions(-D${PLATFORM})
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${FLAGS_DEFAULT} ${FLAGS_CPU} ${FLAGS_PLATFORM}")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${FLAGS_LINKER}")
set(CMAKE_SHARED_LIBRARY_LINK_C_FLAGS "")

if(${USE_LTO})
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -flto -ffat-lto-objects")
endif()

# Configure XCORE library
add_subdirectory(${PATH_XCORE} xcore)

# Configure HALM library, CONFIG_FILE should be defined
set(CONFIG_FILE "${PROJECT_SOURCE_DIR}/halm_config" CACHE INTERNAL "Path to the library configuration file" FORCE)
add_subdirectory(${PATH_HALM} halm)

# Build examples
list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/../../cmake")
include(ListDirectories)

add_custom_target(size)

list_directories(EXAMPLES_LIST "${PROJECT_SOURCE_DIR}")
foreach(EXAMPLE_NAME ${EXAMPLES_LIST})
    file(GLOB_RECURSE EXAMPLE_SOURCES "${EXAMPLE_NAME}/*.c")
    add_executable("${EXAMPLE_NAME}.elf" ${EXAMPLE_SOURCES})

    target_link_libraries("${EXAMPLE_NAME}.elf" PRIVATE halm)
    target_link_libraries("${EXAMPLE_NAME}.elf" PRIVATE xcore)

    add_custom_command(TARGET "${EXAMPLE_NAME}.elf"
            POST_BUILD
            COMMAND "${CMAKE_OBJCOPY}" "${EXAMPLE_NAME}.elf" -Oihex "${EXAMPLE_NAME}.hex"
    )

    add_dependencies(size "${EXAMPLE_NAME}.elf")
    add_custom_command(TARGET size POST_BUILD COMMAND "${CMAKE_SIZE}" "${EXAMPLE_NAME}.elf")
endforeach()
