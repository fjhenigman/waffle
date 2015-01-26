# ----------------------------------------------------------------------------
# Target: wflinfo (executable)
# ----------------------------------------------------------------------------

if(waffle_has_nacl)
    return()
endif()

list(APPEND wflinfo_sources
    wflinfo.c
    )
list(APPEND wflinfo_libraries
    ${GETOPT_LIBRARIES}
    )

if(waffle_has_egl)
list(APPEND wflinfo_sources
    eglinfo.c
    )
list(APPEND wflinfo_libraries
    dl
    )
endif()

if(waffle_has_glx)
list(APPEND wflinfo_sources
    glxinfo.c
    )
list(APPEND wflinfo_libraries
    dl
    )
endif()

list(REMOVE_DUPLICATES wflinfo_libraries)

add_executable(wflinfo ${wflinfo_sources})
target_link_libraries(wflinfo ${waffle_libname} ${wflinfo_libraries})

if(waffle_on_mac)
    set_target_properties(wflinfo
        PROPERTIES
        COMPILE_FLAGS "-ObjC"
        )
endif()

install(
    TARGETS wflinfo
    DESTINATION ${CMAKE_INSTALL_BINDIR}
    COMPONENT utils
    )
