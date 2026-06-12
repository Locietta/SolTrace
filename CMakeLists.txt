cmake_minimum_required(VERSION 3.24)

project(soltrace_ui VERSION 3.4.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

if(MSVC)
	add_compile_options(/utf-8)
endif()

if(WIN32)
	add_compile_definitions(_WINSOCKAPI_)
	add_compile_definitions(UNICODE)
endif()

option(SOLTRACE_BUILD_GUI "Build the GUI parts of Soltrace" OFF)
option(SOLTRACE_BUILD_CORETRACE "Build the coretrace library (ray tracing core)" ON)
option(SOLTRACE_BUILD_OPTIX_SUPPORT "Build the OptiX support for ray tracing" OFF)
option(SOLTRACE_BUILD_PERF_TEST "Build performance tests (not built for debug)" OFF)

enable_testing()

add_subdirectory(external)
add_subdirectory(coretrace)
add_subdirectory(google-tests)

if(SOLTRACE_BUILD_GUI)
	add_subdirectory(app)
endif()
