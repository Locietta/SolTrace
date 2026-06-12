project(simdriver)

cmake_minimum_required(VERSION 3.19)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

include_directories(
    .
    ../coretrace/simulation_data
    ../coretrace/simulation_results
    ../coretrace/simulation_runner/native_runner
)

add_executable(simdriver main.cpp)

if(SOLTRACE_BUILD_EMBREE_SUPPORT)
    target_include_directories(simdriver PRIVATE
        ../coretrace/simulation_runner/embree_runner
    )
    target_compile_definitions(simdriver PRIVATE SOLTRACE_EMBREE_SUPPORT)
    target_link_libraries(simdriver PRIVATE embree_runner)
endif()

target_link_libraries(simdriver PRIVATE
    simdata
    simresult
    native_runner
)

if(UNIX)
    target_link_libraries(simdriver PRIVATE pthread)
endif()

install(TARGETS simdriver
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
