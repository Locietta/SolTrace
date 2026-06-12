add_subdirectory(native_runner)

if (SOLTRACE_BUILD_EMBREE_SUPPORT)
    add_subdirectory(embree_runner)
endif()

# find cuda and optix libraries, if found then add optix_runner
if (SOLTRACE_BUILD_OPTIX_SUPPORT)

    list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../cmake/")

    find_package(CUDAToolkit)
    find_package(OptiX)
    if(NOT CUDAToolkit_FOUND)
        message(FATAL_ERROR "CUDAToolkit not found. Please install CUDA.")
    endif()
    if(NOT OptiX_FOUND)
        message(FATAL_ERROR "OptiX not found. Please install OptiX.")
    endif()

    if(CUDAToolkit_FOUND AND OptiX_FOUND)
        message(STATUS "OptiX and CUDA found, adding optix_runner")
        message(STATUS "OptiX_ROOT_DIR: ${OptiX_ROOT_DIR}")
        message(STATUS "  CUDA compiler:           ${CMAKE_CUDA_COMPILER}")
        message(STATUS "  CUDA toolkit version:    ${CUDAToolkit_VERSION}")
        message(STATUS "  CUDA toolkit root dir:   ${CUDAToolkit_LIBRARY_ROOT}")

        # Pass OptiX paths to child subdirectories so OptixCSP can find OptiX
        # The parent FindOptiX sets OptiX_ROOT_DIR; OptixCSP's FindOptiX expects OptiX_INSTALL_DIR
        set(OptiX_INSTALL_DIR "${OptiX_ROOT_DIR}" CACHE PATH "Path to OptiX installed location." FORCE)

        add_subdirectory(optix_runner)

    else()
        message(STATUS "OptiX or CUDA not found, skipping optix_runner")
    endif()

endif()
