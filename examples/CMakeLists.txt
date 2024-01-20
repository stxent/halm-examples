# Copyright (C) 2022 xent
# Project is distributed under the terms of the GNU General Public License v3.0

cmake_minimum_required(VERSION 3.21)
project(examples C)

# Project configuration
option(USE_BIN "Convert executables to binary format." OFF)
option(USE_HEX "Convert executables to Intel HEX format." OFF)
option(USE_DFU "Enable memory layout compatible with a bootloader." OFF)
option(USE_LTO "Enable Link Time Optimization." OFF)

option(TARGET_NOR "Use external NOR Flash for an artifact if available." OFF)
option(TARGET_SDRAM "Use external SDRAM for an artifact if available." OFF)
option(TARGET_SRAM "Use embedded SRAM for an artifact if available." OFF)

set(PATH_XCORE "${PROJECT_SOURCE_DIR}/../xcore" CACHE STRING "Path to the XCORE library.")
set(PATH_HALM "${PROJECT_SOURCE_DIR}/../halm" CACHE STRING "Path to the HALM library.")

# Default compiler flags
set(CMAKE_COLOR_DIAGNOSTICS ON)
set(FLAGS_PROJECT "-fdata-sections -ffunction-sections -pedantic -Wall -Wextra -Wshadow")
set(CMAKE_C_STANDARD 23)
set(CMAKE_C_EXTENSIONS OFF)

if(USE_LTO)
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
    if(CMAKE_C_COMPILER_ID STREQUAL "GNU")
        set(CMAKE_C_COMPILE_OPTIONS_IPO -flto=auto -ffat-lto-objects)
    endif()
endif()

# Include platform-specific configuration
include("${PROJECT_SOURCE_DIR}/${BUNDLE}/Config.cmake" OPTIONAL)

# Generate linker script
if(EXISTS "${PROJECT_SOURCE_DIR}/${BUNDLE}/memory.ld")
    configure_file("${PROJECT_SOURCE_DIR}/${BUNDLE}/memory.ld" "${PROJECT_BINARY_DIR}/memory.ld")
    set(FLAGS_LINKER "--specs=nosys.specs --specs=nano.specs -Xlinker --gc-sections -T \"${PROJECT_BINARY_DIR}/memory.ld\"")
endif()

# Configure XCORE library
add_subdirectory(${PATH_XCORE} xcore)

# Configure HALM library, HALM_CONFIG_FILE should be defined
set(HALM_CONFIG_FILE "${PROJECT_BINARY_DIR}/halm.config" CACHE INTERNAL "Path to the library configuration file" FORCE)
add_subdirectory(${PATH_HALM} halm)

# Build examples
list(APPEND CMAKE_MODULE_PATH "${PATH_XCORE}/cmake")
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
    string(FIND ${TEMPLATE_NAME} ":" _template_config_used)

    if(NOT _template_config_used EQUAL -1)
        string(REGEX REPLACE "([^:]+):\"?([^\"]+)\"?" "\\1;\\2" _splitted_name ${TEMPLATE_NAME})
        list(GET _splitted_name 0 _template_name_raw)
        list(GET _splitted_name 1 _template_config)
    else()
        set(_template_name_raw ${TEMPLATE_NAME})
        set(_template_config "")
    endif()

    string(FIND ${_template_name_raw} "=" _template_rename_used)

    if(NOT _template_rename_used EQUAL -1)
        string(REGEX REPLACE "([^:]+)=(.+)" "\\1;\\2" _splitted_name ${_template_name_raw})
        list(GET _splitted_name 0 _template_alias)
        list(GET _splitted_name 1 _template_name)
    else()
        set(_template_alias ${_template_name_raw})
        set(_template_name ${_template_name_raw})
    endif()

    string(TOLOWER ${PLATFORM} _platform_name)
    string(TOLOWER ${FAMILY} _family_name)
    set(TEMPLATE_SOURCE "${PROJECT_BINARY_DIR}/${_template_alias}/main.c")

    add_custom_command(
            OUTPUT ${TEMPLATE_SOURCE}
            COMMAND "${PROJECT_SOURCE_DIR}/../tools/make_example.py"
                    --alias=${_template_alias}
                    --bundle=${BUNDLE}
                    --config="${_template_config}"
                    --family=${_family_name}
                    --output=${PROJECT_BINARY_DIR}
                    --platform=${_platform_name}
                    ${_template_name}
            DEPENDS "${PROJECT_SOURCE_DIR}/../templates/${_template_name}.jinja2"
    )

    add_executable("${_template_alias}" ${TEMPLATE_SOURCE})
    target_link_options("${_template_alias}" PRIVATE SHELL:${FLAGS_CPU} SHELL:${FLAGS_LINKER})
    target_link_libraries("${_template_alias}" PRIVATE halm shared)

    if(${CMAKE_SYSTEM_NAME} STREQUAL "Generic")
        if(USE_BIN)
            add_custom_command(TARGET "${_template_alias}"
                    POST_BUILD
                    COMMAND "${CMAKE_OBJCOPY}" "${_template_alias}${CMAKE_EXECUTABLE_SUFFIX}" -Obinary "${_template_alias}.bin"
            )
        endif()
        if(USE_HEX)
            add_custom_command(TARGET "${_template_alias}"
                    POST_BUILD
                    COMMAND "${CMAKE_OBJCOPY}" "${_template_alias}${CMAKE_EXECUTABLE_SUFFIX}" -Oihex "${_template_alias}.hex"
            )
        endif()
    endif()

    add_dependencies(size "${_template_alias}")
    add_custom_command(TARGET size POST_BUILD COMMAND "${CMAKE_SIZE}" "${_template_alias}${CMAKE_EXECUTABLE_SUFFIX}")
endforeach()

# Build platform-specific examples

foreach(EXAMPLE_NAME ${EXAMPLES_LIST})
    if(${EXAMPLE_NAME} STREQUAL "shared")
        continue()
    endif()

    file(GLOB_RECURSE EXAMPLE_SOURCES "${BUNDLE}/${EXAMPLE_NAME}/*.c")
    add_executable("${EXAMPLE_NAME}" ${EXAMPLE_SOURCES})
    target_link_options("${EXAMPLE_NAME}" PRIVATE SHELL:${FLAGS_CPU} SHELL:${FLAGS_LINKER})
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
