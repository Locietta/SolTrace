#include "ray_utils.h"

#include "shaders/Soltrace.h"
#include "utils/util_check.hpp"

#include <cuda_runtime.h>
#include <cub/cub.cuh>
#include <thrust/iterator/counting_iterator.h>

#include <chrono>
#include <cstdint>
#include <vector>

namespace OptixCSP
{

    // ---------------------------------------------------------------------------
    // Pass 1 – count output records per ray.
    //
    // For each ray the loop reads up to max_depth HitRecord entries.
    // raw_count accumulates every valid record (HIT_CREATE through HIT_EXIT).
    // A ray with only a HIT_CREATE event (raw_count == 1) contributes 0 output
    // records (it missed all elements).  Any ray with raw_count > 1 contributes
    // all raw_count records (CREATE + one or more hits).
    // ---------------------------------------------------------------------------
    __global__ static void count_ray_outputs(
        const HitRecord *__restrict__ hit_buffer,
        uint32_t num_rays,
        uint32_t max_depth,
        uint8_t  *__restrict__ out_record_count,
        uint8_t  *__restrict__ out_has_hit)
    {
        const uint32_t ray = blockIdx.x * blockDim.x + threadIdx.x;
        if (ray >= num_rays)
            return;

        uint32_t raw_count = 0;
        for (uint32_t depth = 0; depth < max_depth; ++depth)
        {
            const uint8_t ht = hit_buffer[max_depth * ray + depth].hit_type;
            if (ht < HIT_CREATE || ht > HIT_EXIT)
                break;
            ++raw_count;
            if (ht == HIT_ABSORB || ht == HIT_EXIT)
                break;
        }

        const uint8_t has_hit = (raw_count > 1) ? 1u : 0u;
        out_record_count[ray] = has_hit ? static_cast<uint8_t>(raw_count) : 0u;
        out_has_hit[ray] = has_hit;

        return;
    }

    // ---------------------------------------------------------------------------
    // Pass 2 – write compacted records.
    //
    // Each thread handles one ray.  Rays whose entry in the exclusive prefix-sum
    // equals that of the next ray (i.e. record_count was 0) write nothing.
    // For qualifying rays the CREATE record is written first, followed by all
    // subsequent hit records, using the pre-computed offset as the base index.
    // ---------------------------------------------------------------------------
    __global__ static void compact_ray_outputs(
        const HitRecord *__restrict__ hit_buffer,
        uint32_t num_rays,
        uint32_t max_depth,
        const uint64_t *__restrict__ offsets,
        const uint8_t  *__restrict__ has_hit,
        HitRecord *__restrict__ out_buffer)
    {
        const uint32_t ray = blockIdx.x * blockDim.x + threadIdx.x;
        if (ray >= num_rays || !has_hit[ray])
            return;

        const HitRecord *ray_base = hit_buffer + max_depth * ray;
        uint64_t out_idx = offsets[ray];

        // Depth 0 is always HIT_CREATE for qualifying rays
        out_buffer[out_idx++] = ray_base[0];

        for (uint32_t depth = 1; depth < max_depth; ++depth)
        {
            const HitRecord &hr = ray_base[depth];
            const uint8_t ht = hr.hit_type;

            if (ht < HIT_CREATE || ht > HIT_EXIT)
                break;

            out_buffer[out_idx++] = hr;
            if (ht == HIT_ABSORB || ht == HIT_EXIT)
                break;
        }

        return;
    }

    // ---------------------------------------------------------------------------
    // Host-callable scratch management
    // ---------------------------------------------------------------------------
    void allocate_compaction_scratch(CompactionScratch &scratch, uint64_t num_rays, uint64_t max_depth)
    {
        free_compaction_scratch(scratch);

        CUDA_CHECK(cudaMalloc(&scratch.d_count, num_rays * sizeof(uint8_t)));
        CUDA_CHECK(cudaMalloc(&scratch.d_offsets, num_rays * sizeof(uint64_t)));
        CUDA_CHECK(cudaMalloc(&scratch.d_has_hit, num_rays * sizeof(uint8_t)));
        CUDA_CHECK(cudaMalloc(&scratch.d_n_hit, sizeof(uint32_t)));

        // Query CUB temp-storage sizes using typed null pointers (size query only).
        // scan_bytes must cover both ExclusiveSum and DeviceSelect::Flagged (d_scan_tmp is reused).
        uint32_t *null_u32 = nullptr;
        uint8_t  *null_u8  = nullptr;
        uint64_t *null_u64 = nullptr;
        cub::DeviceScan::ExclusiveSum(scratch.d_scan_tmp, scratch.scan_bytes, null_u8, null_u64, num_rays);
        cub::DeviceReduce::Sum(scratch.d_red_tmp, scratch.red_bytes, null_u8, null_u32, num_rays);

        size_t select_bytes = 0;
        thrust::counting_iterator<uint64_t> count_iter(0ull);
        cub::DeviceSelect::Flagged(nullptr, select_bytes, count_iter, null_u8, null_u64, null_u32, num_rays);
        if (select_bytes > scratch.scan_bytes)
            scratch.scan_bytes = select_bytes;

        CUDA_CHECK(cudaMalloc(&scratch.d_scan_tmp, scratch.scan_bytes > 0 ? scratch.scan_bytes : 1));
        CUDA_CHECK(cudaMalloc(&scratch.d_red_tmp, scratch.red_bytes > 0 ? scratch.red_bytes : 1));

        // Worst-case compacted output: every slot in the hit buffer could be kept
        CUDA_CHECK(cudaMalloc(&scratch.d_compacted, num_rays * max_depth * sizeof(HitRecord)));

        // CUDA events for GPU-phase timing
        CUDA_CHECK(cudaEventCreate(&scratch.e_gpu1_start));
        CUDA_CHECK(cudaEventCreate(&scratch.e_gpu1_stop));
        CUDA_CHECK(cudaEventCreate(&scratch.e_gpu2_start));
        CUDA_CHECK(cudaEventCreate(&scratch.e_gpu2_stop));
    }

    void free_compaction_scratch(CompactionScratch &scratch)
    {
        // cudaFree is nullptr-safe
        cudaFree(scratch.d_count);
        cudaFree(scratch.d_offsets);
        cudaFree(scratch.d_has_hit);
        cudaFree(scratch.d_n_hit);
        cudaFree(scratch.d_scan_tmp);
        cudaFree(scratch.d_red_tmp);
        cudaFree(scratch.d_compacted);
        // cudaEventDestroy is not nullptr-safe
        if (scratch.e_gpu1_start) cudaEventDestroy(scratch.e_gpu1_start);
        if (scratch.e_gpu1_stop)  cudaEventDestroy(scratch.e_gpu1_stop);
        if (scratch.e_gpu2_start) cudaEventDestroy(scratch.e_gpu2_start);
        if (scratch.e_gpu2_stop)  cudaEventDestroy(scratch.e_gpu2_stop);
        scratch = CompactionScratch{};
    }

    // ---------------------------------------------------------------------------
    // Host-callable orchestrator
    // ---------------------------------------------------------------------------
    uint32_t gpu_compact_hit_buffer(
        const HitRecord *d_hit_buffer,
        uint32_t num_rays,
        uint32_t max_depth,
        uint64_t ray_offset,
        std::vector<HitRecord> &host_out,
        std::vector<uint64_t> &host_ray_ids,
        cudaStream_t stream,
        CompactionScratch &scratch,
        CompactionTimings *timings)
    {
        if (num_rays == 0)
            return 0;

        // ---- Pass 1: count records per ray ----
        const uint32_t block_size = 256;
        const uint32_t grid_size = (num_rays + block_size - 1) / block_size;

        if (timings) CUDA_CHECK(cudaEventRecord(scratch.e_gpu1_start, stream));

        count_ray_outputs<<<grid_size, block_size, 0, stream>>>(
            d_hit_buffer, num_rays, max_depth, scratch.d_count, scratch.d_has_hit);

        // ---- Exclusive prefix-sum: d_count → d_offsets ----
        cub::DeviceScan::ExclusiveSum(scratch.d_scan_tmp, scratch.scan_bytes, scratch.d_count, scratch.d_offsets, num_rays, stream);

        // ---- Reduce: sum(d_has_hit) → d_n_hit ----
        cub::DeviceReduce::Sum(scratch.d_red_tmp, scratch.red_bytes, scratch.d_has_hit, scratch.d_n_hit, num_rays, stream);

        if (timings) CUDA_CHECK(cudaEventRecord(scratch.e_gpu1_stop, stream));

        // ---- Synchronize to read back scalar results ----
        CUDA_CHECK(cudaStreamSynchronize(stream));

        if (timings)
        {
            float ms = 0.f;
            CUDA_CHECK(cudaEventElapsedTime(&ms, scratch.e_gpu1_start, scratch.e_gpu1_stop));
            timings->gpu_phase1_ms += ms;
        }

        // ---- D→H scalar memcpy (CPU wall-clock) ----
        std::chrono::high_resolution_clock::time_point t_scalar;
        if (timings) t_scalar = std::chrono::high_resolution_clock::now();

        uint64_t last_offset = 0;
        uint8_t  last_count = 0;
        uint32_t n_hit_rays = 0;
        CUDA_CHECK(cudaMemcpy(&last_offset, scratch.d_offsets + (num_rays - 1), sizeof(uint64_t), cudaMemcpyDeviceToHost));
        CUDA_CHECK(cudaMemcpy(&last_count, scratch.d_count + (num_rays - 1), sizeof(uint8_t), cudaMemcpyDeviceToHost));
        CUDA_CHECK(cudaMemcpy(&n_hit_rays, scratch.d_n_hit, sizeof(uint32_t), cudaMemcpyDeviceToHost));

        if (timings)
            timings->scalar_dth_ms += std::chrono::duration<float, std::milli>(
                std::chrono::high_resolution_clock::now() - t_scalar).count();

        const uint64_t total_records = last_offset + last_count;

        if (total_records > 0)
        {
            // ---- Pass 2: write compacted HitRecords to pre-allocated device buffer ----
            if (timings) CUDA_CHECK(cudaEventRecord(scratch.e_gpu2_start, stream));

            compact_ray_outputs<<<grid_size, block_size, 0, stream>>>(
                d_hit_buffer, num_rays, max_depth, scratch.d_offsets, scratch.d_has_hit, scratch.d_compacted);

            // ---- Compact global ray IDs into d_offsets (reused after pass 2) ----
            // DeviceSelect::Flagged selects (ray_offset + i) for each i where d_has_hit[i] == 1.
            // d_scan_tmp and d_offsets are both free after compact_ray_outputs completes.
            thrust::counting_iterator<uint64_t> ray_id_iter(ray_offset);
            cub::DeviceSelect::Flagged(
                scratch.d_scan_tmp, scratch.scan_bytes,
                ray_id_iter, scratch.d_has_hit,
                scratch.d_offsets,   // output: global IDs of hit rays (uint64_t)
                scratch.d_n_hit,     // output count (already read; safe to overwrite)
                num_rays, stream);

            if (timings) CUDA_CHECK(cudaEventRecord(scratch.e_gpu2_stop, stream));

            CUDA_CHECK(cudaStreamSynchronize(stream));

            if (timings)
            {
                float ms = 0.f;
                CUDA_CHECK(cudaEventElapsedTime(&ms, scratch.e_gpu2_start, scratch.e_gpu2_stop));
                timings->gpu_phase2_ms += ms;
            }

            // ---- D→H bulk memcpy (CPU wall-clock) ----
            std::chrono::high_resolution_clock::time_point t_bulk;
            if (timings) t_bulk = std::chrono::high_resolution_clock::now();

            // ---- Copy compacted HitRecords to host ----
            const size_t prev_rec = host_out.size();
            host_out.resize(prev_rec + total_records);
            CUDA_CHECK(cudaMemcpy(
                host_out.data() + prev_rec,
                scratch.d_compacted,
                total_records * sizeof(HitRecord),
                cudaMemcpyDeviceToHost));

            // ---- Copy global ray IDs to host (one per logical hit ray) ----
            const size_t prev_ids = host_ray_ids.size();
            host_ray_ids.resize(prev_ids + n_hit_rays);
            CUDA_CHECK(cudaMemcpy(
                host_ray_ids.data() + prev_ids,
                scratch.d_offsets,
                n_hit_rays * sizeof(uint64_t),
                cudaMemcpyDeviceToHost));

            if (timings)
                timings->bulk_dth_ms += std::chrono::duration<float, std::milli>(
                    std::chrono::high_resolution_clock::now() - t_bulk).count();
        }

        if (timings) ++timings->n_calls;
        return n_hit_rays;
    }

} // namespace OptixCSP
