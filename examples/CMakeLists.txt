# Copyright (C) 2022 xent
# Project is distributed under the terms of the GNU General Public License v3.0

project(examples C)
cmake_minimum_required(VERSION 3.13)
cmake_policy(SET CMP0069 NEW)

option(USE_DFU "Use memory layout for a bootloader." OFF)
option(USE_LTO "Enable Link Time Optimization." OFF)

set(PATH_XCORE "${PROJECT_SOURCE_DIR}/../xcore" CACHE STRING "Path to the XCORE library.")
set(PATH_HALM "${PROJECT_SOURCE_DIR}/../halm" CACHE STRING "Path to the HALM library.")

# Default compiler flags
set(FLAGS_DEFAULT "-fdata-sections -ffunction-sections -Wall -Wextra -Wshadow -pedantic")
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${FLAGS_DEFAULT} ${FLAGS_CPU}")
set(CMAKE_SHARED_LIBRARY_LINK_C_FLAGS "")

if(USE_LTO)
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
    if (CMAKE_C_COMPILER_ID STREQUAL "GNU")
        set(CMAKE_C_COMPILE_OPTIONS_IPO -flto -ffat-lto-objects)
    endif()
endif()

# Set platform type
set(BUNDLE_PLATFORM_FILE ${PROJECT_SOURCE_DIR}/${BUNDLE}/platform)
if(EXISTS ${BUNDLE_PLATFORM_FILE})
    file(STRINGS ${BUNDLE_PLATFORM_FILE} CONFIG_PLATFORM)
    list(GET CONFIG_PLATFORM 0 PLATFORM)
endif()

# Include platform-specific configuration
include("${PROJECT_SOURCE_DIR}/${BUNDLE}/Config.cmake" OPTIONAL)

# Generate linker script
if(EXISTS "${PROJECT_SOURCE_DIR}/${BUNDLE}/memory.ld")
    configure_file("${PROJECT_SOURCE_DIR}/${BUNDLE}/memory.ld" "${PROJECT_BINARY_DIR}/memory.ld")
    set(FLAGS_LINKER "--specs=nosys.specs --specs=nano.specs -Xlinker --gc-sections -T \"${PROJECT_BINARY_DIR}/memory.ld\"")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${FLAGS_LINKER}")
endif()

# Configure XCORE library
add_subdirectory(${PATH_XCORE} xcore)

# Configure HALM library, HALM_CONFIG_FILE should be defined
set(HALM_CONFIG_FILE "${PROJECT_SOURCE_DIR}/${BUNDLE}/halm.config" CACHE INTERNAL "Path to the library configuration file" FORCE)
add_subdirectory(${PATH_HALM} halm)

# Build examples
list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/../xcore/cmake")
include(ListDirectories)

add_custom_target(size)

if(${CMAKE_SYSTEM_NAME} STREQUAL "Generic")
    set(CMAKE_EXECUTABLE_SUFFIX ".elf")
endif()

list_directories(EXAMPLES_LIST "${PROJECT_SOURCE_DIR}/${BUNDLE}")
foreach(EXAMPLE_NAME ${EXAMPLES_LIST})
    file(GLOB_RECURSE EXAMPLE_SOURCES "${BUNDLE}/${EXAMPLE_NAME}/*.c")
    add_executable("${EXAMPLE_NAME}" ${EXAMPLE_SOURCES})
    target_link_libraries("${EXAMPLE_NAME}" PRIVATE halm)

    if(${CMAKE_SYSTEM_NAME} STREQUAL "Generic")
        if(USE_DFU)
            add_custom_command(TARGET "${EXAMPLE_NAME}"
                    POST_BUILD
                    COMMAND "${CMAKE_OBJCOPY}" "${EXAMPLE_NAME}${CMAKE_EXECUTABLE_SUFFIX}" -Obinary "${EXAMPLE_NAME}.bin"
            )
        else()
            add_custom_command(TARGET "${EXAMPLE_NAME}"
                    POST_BUILD
                    COMMAND "${CMAKE_OBJCOPY}" "${EXAMPLE_NAME}${CMAKE_EXECUTABLE_SUFFIX}" -Oihex "${EXAMPLE_NAME}.hex"
            )
        endif()
    endif()

    add_dependencies(size "${EXAMPLE_NAME}")
    add_custom_command(TARGET size POST_BUILD COMMAND "${CMAKE_SIZE}" "${EXAMPLE_NAME}")
endforeach()
