include(CMakeParseArguments)

macro(set_shared_lib_flags LIB_NAME)
    set_target_properties("${LIB_NAME}_shared" PROPERTIES
        COMPILE_FLAGS ${ARGN})
    set_target_properties("${LIB_NAME}_shared" PROPERTIES
        LINK_FLAGS ${ARGN})
endmacro()

macro(export_install_target TYPE LIB_NAME)
    string(REPLACE "hare_" "" PURE_NAME ${LIB_NAME})
    string(TOUPPER ${TYPE} UPPER_TYPE)
    set(HARE_${UPPER_TYPE}_LIBRARIES "${PURE_NAME};${HARE_${UPPER_TYPE}_LIBRARIES}" PARENT_SCOPE)
    set(OUTER_INCS)

    if(NOT "${OUTER_INCLUDES}" STREQUAL "NONE")
        set(OUTER_INCS ${OUTER_INCLUDES})
    endif()

    target_include_directories("${LIB_NAME}_${TYPE}"
        PUBLIC "$<INSTALL_INTERFACE:include>"
        "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>"
        "$<BUILD_INTERFACE:${PROJECT_BINARY_DIR}/include>"
    )
    set_target_properties("${LIB_NAME}_${TYPE}" PROPERTIES EXPORT_NAME ${PURE_NAME})
    export(TARGETS "${LIB_NAME}_${TYPE}"
        NAMESPACE ${PROJECT_NAME}::
        FILE "${PROJECT_BINARY_DIR}/HareTargets-${TYPE}.cmake"
        APPEND
    )
    install(TARGETS "${LIB_NAME}_${TYPE}"
        EXPORT HareTargets-${TYPE}
        LIBRARY DESTINATION "lib" COMPONENT lib
        ARCHIVE DESTINATION "lib" COMPONENT lib
        RUNTIME DESTINATION "lib" COMPONENT lib
        COMPONENT dev
    )
endmacro()

macro(add_hare_library LIB_NAME)
    cmake_parse_arguments(LIB
        "" # Options
        "VERSION" # One val
        "SOURCES;LIBRARIES;INNER_LIBRARIES" # Multi val
        ${ARGN}
    )

    set(ADD_HARE_LIBRARY_INTERFACE)
    set(INNER_LIBRARIES)

    if(${HARE_LIBRARY_STATIC})
        add_library("${LIB_NAME}_static" STATIC ${LIB_SOURCES})
        set_target_properties("${LIB_NAME}_static" PROPERTIES
            OUTPUT_NAME "${LIB_NAME}"
            CLEAN_DIRECT_OUTPUT 1)

        if(LIB_INNER_LIBRARIES)
            set(INNER_LIBRARIES "${LIB_INNER_LIBRARIES}_static")
        endif()

        target_link_libraries("${LIB_NAME}_static"
            ${LIB_PLATFORM}
            ${INNER_LIBRARIES}
            ${LIB_LIBRARIES})

        export_install_target(static "${LIB_NAME}")

        set(ADD_HARE_LIBRARY_INTERFACE "${LIB_NAME}_static")
    endif()

    if(${HARE_LIBRARY_SHARED})
        add_library("${LIB_NAME}_shared" SHARED ${LIB_SOURCES})

        if(LIB_INNER_LIBRARIES)
            set(INNER_LIBRARIES "${LIB_INNER_LIBRARIES}_shared")
        endif()

        target_link_libraries("${LIB_NAME}_shared"
            ${LIB_PLATFORM}
            ${INNER_LIBRARIES}
            ${LIB_LIBRARIES})

        if(HARE_SHARED_FLAGS)
            set_shared_lib_flags("${LIB_NAME}" "${HARE_SHARED_FLAGS}")
        endif()

        if(WIN32)
            set_target_properties(
                "${LIB_NAME}_shared" PROPERTIES
                OUTPUT_NAME "${LIB_NAME}"
                SOVERSION ${HARE_ABI_LIBVERSION})
        else()
            set_target_properties(
                "${LIB_NAME}_shared" PROPERTIES
                OUTPUT_NAME "${LIB_NAME}"
                VERSION "${HARE_ABI_LIBVERSION}"
                SOVERSION "${HARE_ABI_MAJOR}")
        endif()

        if(NOT WIN32)
            set(LIB_LINK_NAME
                "${CMAKE_SHARED_LIBRARY_PREFIX}${LIB_NAME}${CMAKE_SHARED_LIBRARY_SUFFIX}")

            add_custom_command(TARGET ${LIB_NAME}_shared
                POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E create_symlink
                "$<TARGET_FILE_NAME:${LIB_NAME}_shared>"
                "${LIB_LINK_NAME}"
                WORKING_DIRECTORY "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}")
        endif()

        export_install_target(shared "${LIB_NAME}")

        set(ADD_HARE_LIBRARY_INTERFACE "${LIB_NAME}_shared")

        if(NOT WIN32)
            install(FILES
                "$<TARGET_FILE_DIR:${LIB_NAME}_shared>/${LIB_LINK_NAME}"
                DESTINATION "lib"
                COMPONENT lib)
        endif()
    endif()

    add_library(${LIB_NAME} INTERFACE)
    target_link_libraries(${LIB_NAME} INTERFACE ${ADD_HARE_LIBRARY_INTERFACE})
endmacro()