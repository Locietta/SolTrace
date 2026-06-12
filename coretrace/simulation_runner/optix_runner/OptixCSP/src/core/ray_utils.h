#pragma once

#include <cuda_runtime.h>
#include <cstdint>
#include <vector>

namespace OptixCSP
{

    struct HitRecord;

    /// Device scratch buffers required by gpu_compact_hit_buffer.
    /// Allocated once via allocate_compaction_scratch and reused across calls
    /// as long as num_rays and max_depth stay the same.
    struct CompactionScratch
    {
        uint8_t  *d_count = nullptr;   // per-ray output record count (bounded by max_depth ≤ 255)
        uint64_t *d_offsets = nullptr; // exclusive prefix-sum of d_count (pass 1); reused as global ray IDs (pass 2)
        uint8_t  *d_has_hit = nullptr; // 1 if ray contributes records, else 0
        uint32_t *d_n_hit = nullptr;   // scalar: total hit rays
        void *d_scan_tmp = nullptr;    // CUB DeviceScan temp storage
        size_t scan_bytes = 0;
        void *d_red_tmp = nullptr; // CUB DeviceReduce temp storage
        size_t red_bytes = 0;
        HitRecord *d_compacted = nullptr; // worst-case compacted output (num_rays * max_depth)

        // CUDA events for GPU-phase timing (non-null after allocate_compaction_scratch).
        cudaEvent_t e_gpu1_start = nullptr; // before count/scan/reduce kernels
        cudaEvent_t e_gpu1_stop  = nullptr; // after  count/scan/reduce kernels
        cudaEvent_t e_gpu2_start = nullptr; // before compact/select kernels
        cudaEvent_t e_gpu2_stop  = nullptr; // after  compact/select kernels
    };

    /// Per-call timing breakdown populated by gpu_compact_hit_buffer.
    /// All times are accumulated (ms) across calls.  Reset to {} at the start of each run().
    struct CompactionTimings
    {
        float    gpu_phase1_ms = 0.f; // count + scan + reduce kernels  (GPU time via CUDA events)
        float    scalar_dth_ms = 0.f; // 3x scalar D->H cudaMemcpy  (CPU wall-clock)
        float    gpu_phase2_ms = 0.f; // compact + select kernels       (GPU time via CUDA events)
        float    bulk_dth_ms   = 0.f; // HitRecord + ray-ID bulk D\u2192H (CPU wall-clock)
        uint32_t n_calls       = 0;   // total gpu_compact_hit_buffer invocations
    };

    /// Allocate all device scratch buffers for the given ray-buffer dimensions.
    /// Frees any previous allocation before reallocating.
    void allocate_compaction_scratch(CompactionScratch &scratch, uint64_t num_rays, uint64_t max_depth);

    /// Free all device scratch buffers and reset the struct to its default state.
    void free_compaction_scratch(CompactionScratch &scratch);

    /// GPU-side stream compaction of the raw hit buffer.
    /// Uses pre-allocated scratch buffers — no device allocations occur inside this call.
    /// Appends compacted HitRecords to @p host_out and the corresponding global ray indices
    /// (ray_offset + local_ray_index) to @p host_ray_ids (one entry per logical hit ray).
    /// Pass a non-null @p timings to accumulate per-phase timing (GPU events + CPU wall-clock).
    /// @returns Number of rays that produced at least one non-CREATE hit.
    uint32_t gpu_compact_hit_buffer(
        const HitRecord *d_hit_buffer,
        uint32_t num_rays,
        uint32_t max_depth,
        uint64_t ray_offset,
        std::vector<HitRecord> &host_out,
        std::vector<uint64_t> &host_ray_ids,
        cudaStream_t stream,
        CompactionScratch &scratch,
        CompactionTimings *timings = nullptr);

} // namespace OptixCSP
