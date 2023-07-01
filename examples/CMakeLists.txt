# Copyright (C) 2022 xent
# Project is distributed under the terms of the GNU General Public License v3.0

cmake_minimum_required(VERSION 3.21)
cmake_policy(SET CMP0069 NEW)
project(examples C)

option(USE_BIN "Convert executables to binary format." OFF)
option(USE_HEX "Convert executables to Intel HEX format." ON)
option(USE_LTO "Enable Link Time Optimization." OFF)

set(PATH_XCORE "${PROJECT_SOURCE_DIR}/../xcore" CACHE STRING "Path to the XCORE library.")
set(PATH_HALM "${PROJECT_SOURCE_DIR}/../halm" CACHE STRING "Path to the HALM library.")

# Default compiler flags
set(FLAGS_DEFAULT "-pedantic -Wall -Wextra -Wshadow")
set(FLAGS_PROJECT "-fdata-sections -ffunction-sections")
set(CMAKE_C_STANDARD 23)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_C_FLAGS_INIT "${CMAKE_C_FLAGS_INIT} ${FLAGS_DEFAULT}")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS_INIT} ${FLAGS_PROJECT} ${FLAGS_CPU}")
set(CMAKE_SHARED_LIBRARY_LINK_C_FLAGS "")

if(USE_LTO)
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
    if(CMAKE_C_COMPILER_ID STREQUAL "GNU")
        set(CMAKE_C_COMPILE_OPTIONS_IPO -flto -ffat-lto-objects)
    endif()
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

# Gather shared objects
if(EXISTS "${PROJECT_SOURCE_DIR}/${BUNDLE}/shared")
    file(GLOB_RECURSE SHARED_SOURCES "${BUNDLE}/shared/*.c")
    add_library(shared OBJECT ${SHARED_SOURCES})
    target_include_directories(shared PUBLIC "${BUNDLE}/shared")
    target_link_libraries(shared PRIVATE halm)
endif()

list_directories(EXAMPLES_LIST "${PROJECT_SOURCE_DIR}/${BUNDLE}")

# Generate and build templated examples

foreach(TEMPLATE_NAME ${TEMPLATES_LIST})
    string(TOLOWER ${PLATFORM} _platform_name)
    string(TOLOWER ${FAMILY} _family_name)
    set(TEMPLATE_SOURCE "${PROJECT_BINARY_DIR}/${TEMPLATE_NAME}/main.c")

    add_custom_command(
        OUTPUT ${TEMPLATE_SOURCE}
        COMMAND "${PROJECT_SOURCE_DIR}/../tools/make_example.py"
                --bundle=${BUNDLE}
                --config="${TEMPLATES_CONFIG}"
                --family=${_family_name}
                --output=${PROJECT_BINARY_DIR}
                --platform=${_platform_name}
                ${TEMPLATE_NAME}
        DEPENDS "${PROJECT_SOURCE_DIR}/../templates/${TEMPLATE_NAME}.jinja2"
    )

    add_executable("${TEMPLATE_NAME}" ${TEMPLATE_SOURCE})
    target_link_libraries("${TEMPLATE_NAME}" PRIVATE halm shared)

    if(${CMAKE_SYSTEM_NAME} STREQUAL "Generic")
        if(USE_BIN)
            add_custom_command(TARGET "${TEMPLATE_NAME}"
                    POST_BUILD
                    COMMAND "${CMAKE_OBJCOPY}" "${TEMPLATE_NAME}${CMAKE_EXECUTABLE_SUFFIX}" -Obinary "${TEMPLATE_NAME}.bin"
            )
        endif()
        if(USE_HEX)
            add_custom_command(TARGET "${TEMPLATE_NAME}"
                    POST_BUILD
                    COMMAND "${CMAKE_OBJCOPY}" "${TEMPLATE_NAME}${CMAKE_EXECUTABLE_SUFFIX}" -Oihex "${TEMPLATE_NAME}.hex"
            )
        endif()
    endif()

    add_dependencies(size "${TEMPLATE_NAME}")
    add_custom_command(TARGET size POST_BUILD COMMAND "${CMAKE_SIZE}" "${TEMPLATE_NAME}${CMAKE_EXECUTABLE_SUFFIX}")
endforeach()

# Generate and build custom examples

foreach(EXAMPLE_NAME ${EXAMPLES_LIST})
    if(${EXAMPLE_NAME} STREQUAL "shared")
        continue()
    endif()

    file(GLOB_RECURSE EXAMPLE_SOURCES "${BUNDLE}/${EXAMPLE_NAME}/*.c")
    add_executable("${EXAMPLE_NAME}" ${EXAMPLE_SOURCES})
    target_link_libraries("${EXAMPLE_NAME}" PRIVATE halm)

    if(TARGET shared)
        target_link_libraries("${EXAMPLE_NAME}" PRIVATE shared)
    endif()

    if(${CMAKE_SYSTEM_NAME} STREQUAL "Generic")
        if(USE_BIN)
            add_custom_command(TARGET "${EXAMPLE_NAME}"
                    POST_BUILD
                    COMMAND "${CMAKE_OBJCOPY}" "${EXAMPLE_NAME}${CMAKE_EXECUTABLE_SUFFIX}" -Obinary "${EXAMPLE_NAME}.bin"
            )
        endif()
        if(USE_HEX)
            add_custom_command(TARGET "${EXAMPLE_NAME}"
                    POST_BUILD
                    COMMAND "${CMAKE_OBJCOPY}" "${EXAMPLE_NAME}${CMAKE_EXECUTABLE_SUFFIX}" -Oihex "${EXAMPLE_NAME}.hex"
            )
        endif()
    endif()

    add_dependencies(size "${EXAMPLE_NAME}")
    add_custom_command(TARGET size POST_BUILD COMMAND "${CMAKE_SIZE}" "${EXAMPLE_NAME}${CMAKE_EXECUTABLE_SUFFIX}")
endforeach()
