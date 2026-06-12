add_subdirectory(lk)
add_subdirectory(wex)

if(SOLTRACE_BUILD_OPTIX_SUPPORT)
    set(_soltrace_optixcsp_root "${CMAKE_CURRENT_SOURCE_DIR}/OptixCSP/CMakeLists.txt")
    set(OptiX_INSTALL_DIR "$ENV{ProgramData}/NVIDIA Corporation/OptiX SDK 9.1.0" CACHE PATH "Path to the installed location of the OptiX SDK.")

    if(EXISTS "${_soltrace_optixcsp_root}")
        if(NOT TARGET OptixCSP_core)
            add_subdirectory(OptixCSP)
        endif()

        if(TARGET OptixCSP_core AND NOT TARGET OptixCSP::OptixCSP_core)
            add_library(OptixCSP::OptixCSP_core ALIAS OptixCSP_core)
        endif()
    else()
        message(FATAL_ERROR "OptixCSP submodule is missing. Please run 'git submodule update --init --recursive'.")
    endif()
endif()
