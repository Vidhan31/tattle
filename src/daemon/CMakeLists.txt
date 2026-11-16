qt_add_executable(tattled
    main.cpp
)

target_compile_definitions(tattled
    PRIVATE
        TATTLE_VERSION="${PROJECT_VERSION}"
)

target_include_directories(tattled
    PRIVATE
        ${CMAKE_SOURCE_DIR}/src
        ${CMAKE_SOURCE_DIR}/src/core
)

target_link_libraries(tattled
    PRIVATE
        tattle_core
        tattle::compiler_flags
)

install(TARGETS tattled
    DESTINATION ${CMAKE_INSTALL_BINDIR}
)
