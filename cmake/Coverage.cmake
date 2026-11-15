# cmake/Coverage.cmake
# Configures code coverage generation for first-party Tattle targets.

if(TARGET tattle_coverage)
    return()
endif()

add_library(tattle_coverage INTERFACE)
add_library(tattle::coverage ALIAS tattle_coverage)

option(ENABLE_COVERAGE "Enable code coverage instrumentation" OFF)

if(NOT ENABLE_COVERAGE)
    return()
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    message(STATUS "Enabling GCC code coverage (--coverage)")
    target_compile_options(tattle_coverage INTERFACE
        --coverage
        -fno-inline
        -fno-inline-small-functions
        -fno-default-inline
        -g
        -O0
    )
    target_link_options(tattle_coverage INTERFACE
        --coverage
    )
elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    message(STATUS "Enabling Clang code coverage (-fprofile-instr-generate -fcoverage-mapping)")
    target_compile_options(tattle_coverage INTERFACE
        -fprofile-instr-generate
        -fcoverage-mapping
        -fno-inline
        -g
        -O0
    )
    target_link_options(tattle_coverage INTERFACE
        -fprofile-instr-generate
        -fcoverage-mapping
    )
else()
    message(WARNING "Coverage is not configured for compiler: ${CMAKE_CXX_COMPILER_ID}")
endif()

if(TARGET tattle_compiler_flags)
    target_link_libraries(tattle_compiler_flags INTERFACE tattle_coverage)
endif()
