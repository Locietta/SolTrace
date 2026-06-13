project(fluxdriver)

cmake_minimum_required(VERSION 3.19)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

include_directories(
    .
    ../simulation_data
    ../simulation_results
    ../simulation_runner/optix_runner
)

add_executable(fluxdriver main.cpp)

target_link_libraries(fluxdriver PRIVATE
    simdata
    simresult
    optix_runner
)

install(TARGETS fluxdriver
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
