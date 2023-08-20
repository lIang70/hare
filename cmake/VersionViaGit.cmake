# This module defines the following variables utilizing
# git to determine the parent tag. And if found the macro
# will attempt to parse them in the github tag fomat
#
# Useful for auto-versioning in our CMakeLists
#
# HARE_GIT__VERSION_MAJOR - Major version.
# HARE_GIT__VERSION_MINOR - Minor version
# HARE_GIT__VERSION_STAGE - Stage version
#
# Example usage:
#
# event_fuzzy_version_from_git()
#     message("Libvent major=${HARE_GIT__VERSION_MAJOR}")
#     message("        minor=${HARE_GIT__VERSION_MINOR}")
#     message("        patch=${HARE_GIT__VERSION_PATCH}")
#     message("        stage=${HARE_GIT__VERSION_STAGE}")
# endif()

include(FindGit)

macro(hare_fuzzy_version_from_git)
    # set our defaults.
    set(HARE_GIT__VERSION_MAJOR 0)
    set(HARE_GIT__VERSION_MINOR 2)
    set(HARE_GIT__VERSION_PATCH 1)
    set(HARE_GIT__VERSION_STAGE "beta")

    find_package(Git)

    if(GIT_FOUND)
        execute_process(
            COMMAND
            ${GIT_EXECUTABLE} describe --abbrev=0 --always
            WORKING_DIRECTORY
            ${PROJECT_SOURCE_DIR}
            RESULT_VARIABLE
            GITRET
            OUTPUT_VARIABLE
            GITVERSION
            OUTPUT_STRIP_TRAILING_WHITESPACE
        )

        string(REGEX REPLACE "[\\._-]" ";" VERSION_LIST "${GITVERSION}")

        if(VERSION_LIST)
            list(LENGTH VERSION_LIST VERSION_LIST_LENGTH)
        endif()

        if((GITRET EQUAL 0) AND(VERSION_LIST_LENGTH EQUAL 5))
            list(GET VERSION_LIST 1 _MAJOR)
            list(GET VERSION_LIST 2 _MINOR)
            list(GET VERSION_LIST 3 _PATCH)
            list(GET VERSION_LIST 4 _STAGE)

            set(_DEFAULT_VERSION "${HARE_GIT__VERSION_MAJOR}.${HARE_GIT__VERSION_MINOR}.${HARE_GIT__VERSION_PATCH}-${HARE_GIT__VERSION_STAGE}")
            set(_GIT_VERSION "${_MAJOR}.${_MINOR}.${_PATCH}-${_STAGE}")

            if(${_DEFAULT_VERSION} VERSION_LESS ${_GIT_VERSION})
                set(HARE_GIT__VERSION_MAJOR ${_MAJOR})
                set(HARE_GIT__VERSION_MINOR ${_MINOR})
                set(HARE_GIT__VERSION_PATCH ${_PATCH})
                set(HARE_GIT__VERSION_STAGE ${_STAGE})
            endif()
        endif()
    endif()
endmacro()
