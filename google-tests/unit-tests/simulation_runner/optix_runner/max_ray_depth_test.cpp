#include <gtest/gtest.h>

#include <optical_properties.hpp>
#include <optix_runner.hpp>
#include <simulation_data.hpp>
#include <simulation_data_export.hpp>
#include <simulation_result_export.hpp>
#include <shaders/Soltrace.h>        // OptixCSP::DEFAULT_MAX_TRACE_DEPTH
#include <core/soltrace_constants.h> // OptixCSP::HitType

using SolTrace::Runner::RunnerStatus;

// Reuse the two-plate scene defined in two_plate_test.cpp.
// Plate 1 is ideal-reflective; plate 2 is ideal-absorptive.
// Each ray that hits the scene makes exactly 2 element interactions:
//   1. reflect off plate 1
//   2. absorb on plate 2
void make_two_plate_sd(SimulationData &sd, element_ptr &plate1, element_ptr &plate2);

// ---------------------------------------------------------------------------
// set_max_ray_depth / get_max_ray_depth accessor tests (no GPU required)
// ---------------------------------------------------------------------------

TEST(OptixRunnerMaxRayDepth, DefaultIsDefaultMaxTraceDepth)
{
    OptixRunner runner;
    EXPECT_EQ(runner.get_max_ray_depth(), OptixCSP::DEFAULT_MAX_TRACE_DEPTH);
}

TEST(OptixRunnerMaxRayDepth, SetAndGet)
{
    OptixRunner runner;
    runner.set_max_ray_depth(10);
    EXPECT_EQ(runner.get_max_ray_depth(), 10u);
}

// Minimum valid depth is 2; setting exactly 2 must be accepted without clamping.
TEST(OptixRunnerMaxRayDepth, MinimumBoundaryAccepted)
{
    OptixRunner runner;
    runner.set_max_ray_depth(2);
    EXPECT_EQ(runner.get_max_ray_depth(), 2u);
}

// Maximum valid depth is 255; setting exactly 255 must be accepted without clamping.
TEST(OptixRunnerMaxRayDepth, MaximumBoundaryAccepted)
{
    OptixRunner runner;
    runner.set_max_ray_depth(255);
    EXPECT_EQ(runner.get_max_ray_depth(), 255u);
}

// Depth of 0 is below the minimum; must be clamped to 2 with a warning.
TEST(OptixRunnerMaxRayDepth, ZeroClampsToMinimum)
{
    OptixRunner runner;
    testing::internal::CaptureStderr();
    runner.set_max_ray_depth(0);
    const std::string output = testing::internal::GetCapturedStderr();
    EXPECT_EQ(runner.get_max_ray_depth(), 2u);
    EXPECT_NE(output.find("WARNING"), std::string::npos);
}

// Depth of 1 is below the minimum; must be clamped to 2 with a warning.
TEST(OptixRunnerMaxRayDepth, OneClampsToMinimum)
{
    OptixRunner runner;
    testing::internal::CaptureStderr();
    runner.set_max_ray_depth(1);
    const std::string output = testing::internal::GetCapturedStderr();
    EXPECT_EQ(runner.get_max_ray_depth(), 2u);
    EXPECT_NE(output.find("WARNING"), std::string::npos);
}

// Depth of 256 exceeds the maximum; must be clamped to 255 with a warning.
TEST(OptixRunnerMaxRayDepth, ExceedingMaxClampsTo255)
{
    OptixRunner runner;
    testing::internal::CaptureStderr();
    runner.set_max_ray_depth(256);
    const std::string output = testing::internal::GetCapturedStderr();
    EXPECT_EQ(runner.get_max_ray_depth(), 255u);
    EXPECT_NE(output.find("WARNING"), std::string::npos);
}

// Large values (e.g. passing a big int) must also clamp to 255 with a warning.
TEST(OptixRunnerMaxRayDepth, LargeValueClampsTo255)
{
    OptixRunner runner;
    testing::internal::CaptureStderr();
    runner.set_max_ray_depth(10000);
    const std::string output = testing::internal::GetCapturedStderr();
    EXPECT_EQ(runner.get_max_ray_depth(), 255u);
    EXPECT_NE(output.find("WARNING"), std::string::npos);
}

// An out-of-range value is always clamped and stored, replacing any previous value.
TEST(OptixRunnerMaxRayDepth, ClampedValueOverwritesPreviousValue)
{
    OptixRunner runner;
    runner.set_max_ray_depth(20);
    ASSERT_EQ(runner.get_max_ray_depth(), 20u);

    testing::internal::CaptureStderr();
    runner.set_max_ray_depth(0);
    const std::string output = testing::internal::GetCapturedStderr();
    EXPECT_EQ(runner.get_max_ray_depth(), 2u);
    EXPECT_NE(output.find("WARNING"), std::string::npos);
}

// Setting a valid depth after an out-of-range call must work normally.
TEST(OptixRunnerMaxRayDepth, ValidSetAfterClamp)
{
    OptixRunner runner;
    testing::internal::CaptureStderr();
    runner.set_max_ray_depth(0);
    testing::internal::GetCapturedStderr();

    runner.set_max_ray_depth(8);
    EXPECT_EQ(runner.get_max_ray_depth(), 8u);
}

// ---------------------------------------------------------------------------
// GPU trace test: verify the depth limit is actually enforced during tracing.
//
// The two-plate scene produces exactly 2 element interactions per ray under
// normal conditions (reflect off plate 1, absorb on plate 2).  Setting
// max_ray_depth = 2 reduces the maximum interactions per ray to 1, so the
// second interaction is cut off.  We verify that no ray in the hit buffer
// carries more than (max_ray_depth - 1) element interactions.
// ---------------------------------------------------------------------------
TEST(OptixRunnerMaxRayDepth, TraceDepthNotExceeded)
{
    SimulationData sd;
    element_ptr plate1, plate2;
    make_two_plate_sd(sd, plate1, plate2);

    const uint_fast64_t test_max_depth = 2; // allows at most 1 interaction per ray
    OptixRunner runner;
    runner.set_max_ray_depth(test_max_depth);
    ASSERT_EQ(runner.get_max_ray_depth(), static_cast<uint8_t>(test_max_depth));

    RunnerStatus sts = runner.initialize();
    ASSERT_EQ(sts, RunnerStatus::SUCCESS);
    sts = runner.setup_simulation(&sd);
    ASSERT_EQ(sts, RunnerStatus::SUCCESS);
    sts = runner.run_simulation();
    ASSERT_EQ(sts, RunnerStatus::SUCCESS);

    OptixCSP::SolTraceSystem *sys = runner.get_optix_system();
    std::vector<float4> hp_vec;
    std::vector<uint_fast64_t> raynumber_vec;
    std::vector<int32_t> element_id_vec;
    std::vector<uint8_t> hit_type_vec;
    sys->get_hp_output(hp_vec, raynumber_vec, element_id_vec, hit_type_vec);

    // Walk through every record.  HIT_CREATE marks the start of a new ray;
    // all other types are element interactions.  No ray may accumulate more
    // than (max_ray_depth - 1) interactions.
    const uint_fast64_t max_interactions = test_max_depth - 1;
    uint_fast64_t current_interactions = 0;
    for (uint8_t ht : hit_type_vec)
    {
        if (ht == OptixCSP::HitType::HIT_CREATE)
        {
            current_interactions = 0;
        }
        else
        {
            ++current_interactions;
            EXPECT_LE(current_interactions, max_interactions)
                << "A ray exceeded max_ray_depth - 1 element interactions";
        }
    }
}

// ---------------------------------------------------------------------------
// GPU trace tests: verify the depth-exceeded counter behaves correctly.
// ---------------------------------------------------------------------------

// With max_depth at the default (5), the two-plate scene (reflect→absorb) uses
// only 2 depth slots per ray — well within the limit.  The counter must stay 0.
TEST(OptixRunnerMaxRayDepth, DepthExceededCounterIsZeroWhenDepthNotExceeded)
{
    SimulationData sd;
    element_ptr plate1, plate2;
    make_two_plate_sd(sd, plate1, plate2);

    OptixRunner runner;
    // Leave max_ray_depth at the default (5); the scene only needs depth 2.
    RunnerStatus sts = runner.initialize();
    ASSERT_EQ(sts, RunnerStatus::SUCCESS);
    sts = runner.setup_simulation(&sd);
    ASSERT_EQ(sts, RunnerStatus::SUCCESS);
    sts = runner.run_simulation();
    ASSERT_EQ(sts, RunnerStatus::SUCCESS);

    EXPECT_EQ(runner.get_N_depth_exceeded_rays(), 0u);
}

// With max_depth=2, the two-plate scene where BOTH plates are reflective forces
// rays that reach plate 2 to exceed the depth limit (new_depth=2 >= max_depth=2,
// and plate 2 is non-absorbing).  Not all reflections from plate 1 geometrically
// reach plate 2, so counter <= n_hit.  The key check is that counter > 0 and
// never exceeds the total number of hit rays.
TEST(OptixRunnerMaxRayDepth, DepthExceededCounterCountsTerminatedReflectedRays)
{
    SimulationData sd;
    element_ptr plate1, plate2;
    make_two_plate_sd(sd, plate1, plate2);

    // Override plate 2 to be reflective so hitting it at max_depth triggers the counter.
    SolTrace::Data::OpticalPropertySet reflective_optics(
        SolTrace::Data::InteractionType::REFLECTION,
        0.0,
        0.0,
        "max_depth_reflective_override");
    reflective_optics.set_ideal_reflection(OpticalSide::Both);
    auto reflective_optics_ref = sd.add_optical_property_set(reflective_optics);
    plate2->set_optical_property_set(reflective_optics_ref);

    OptixRunner runner;
    runner.set_max_ray_depth(2); // max interactions per ray = 1 (plate 1 only)
    ASSERT_EQ(runner.get_max_ray_depth(), 2u);

    RunnerStatus sts = runner.initialize();
    ASSERT_EQ(sts, RunnerStatus::SUCCESS);
    sts = runner.setup_simulation(&sd);
    ASSERT_EQ(sts, RunnerStatus::SUCCESS);
    sts = runner.run_simulation();
    ASSERT_EQ(sts, RunnerStatus::SUCCESS);

    const uint_fast64_t counter = runner.get_N_depth_exceeded_rays();
    const uint_fast64_t n_hit = runner.get_number_rays_traced();

    // Every ray that reached plate 2 (a subset of the plate 1 hits —
    // some reflections miss plate 2 entirely) must have incremented the counter.
    EXPECT_GT(counter, 0u) << "Expected depth-exceeded counter to be non-zero";
    EXPECT_LE(counter, n_hit)
        << "Counter cannot exceed the total number of hit rays";
}
