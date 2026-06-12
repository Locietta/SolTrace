#pragma once
#include <optix.h>        // For OptixAabb
#include <vector_types.h> // For float3
#include <curand_kernel.h>
#include <cuda_runtime_api.h>

namespace OptixCSP {


    void compute_d_on_gpu(
        const OptixAabb* d_all_aabbs,
        int num_aabbs,
        float3 sun_dir_normalized,
        float* d_out_max_d_on_gpu
    );

    void compute_uv_bounds_on_gpu(
        const OptixAabb* d_all_aabbs,
        int num_aabbs,
        float d_plane_val,
        const float3& sun_vector_normalized,
        const float3& sun_u_basis,
        const float3& sun_v_basis,
        float tan_max_angle,
        float* d_out_uv_bounds
    );

    void initialize_curand_states_on_gpu(
        curandState* d_rng_states,
        unsigned int num_states,
        unsigned long long seed,
        unsigned int sequence_offset,
        cudaStream_t stream
    );
}