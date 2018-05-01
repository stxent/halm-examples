# Copyright (C) 2017 xent
# Project is distributed under the terms of the GNU General Public License v3.0

function(list_directories _search_results _directory_path)
    file(GLOB _entry_list RELATIVE ${_directory_path} ${_directory_path}/*)
    set(_directory_list "")
    foreach(_entry ${_entry_list})
        if(IS_DIRECTORY ${_directory_path}/${_entry})
            string(FIND "${_entry}" "." _entry_is_invisible)
            if(NOT ${_entry_is_invisible} EQUAL 0)
                message(STATUS "Added target ${_entry}")
                list(APPEND _directory_list ${_entry})
            else()
                message(STATUS "Skipped directory ${_entry}")
            endif()
        endif()
    endforeach()
    set(${_search_results} ${_directory_list} PARENT_SCOPE)
endfunction()
