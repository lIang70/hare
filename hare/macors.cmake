include(CheckSymbolExists)
include(CheckIncludeFiles)

macro(CHECK_SYMBOLS_EXIST SYMLIST HEADERS PREFIX)
    foreach(SYMNAME ${SYMLIST})
        string(TOUPPER "${SYMNAME}" SYMNAME_UPPER)

        if("${PREFIX}" STREQUAL "")
            set(HAVE_SYM_DEF "HAVE_${SYMNAME_UPPER}")
        else()
            set(HAVE_SYM_DEF "${PREFIX}__HAVE_${SYMNAME_UPPER}")
        endif()

        CHECK_SYMBOL_EXISTS(${SYMNAME} "${HEADERS}" ${HAVE_SYM_DEF})
    endforeach()
endmacro()

macro(CHECK_INCLUDE_FILE_CONCAT FILE PREFIX)
    string(REGEX REPLACE "[./]" "_" FILE_UL ${FILE})
    string(TOUPPER "${FILE_UL}" FILE_UL_UPPER)

    if("${PREFIX}" STREQUAL "")
        set(HAVE_FILE_DEF "HAVE_${FILE_UL_UPPER}")
    else()
        set(HAVE_FILE_DEF "${PREFIX}__HAVE_${FILE_UL_UPPER}")
    endif()

    CHECK_INCLUDE_FILES("${HARE_INCLUDES};${FILE}" ${HAVE_FILE_DEF})

    if(${HAVE_FILE_DEF})
        set(HARE_INCLUDES ${HARE_INCLUDES} ${FILE})
    endif()
endmacro()