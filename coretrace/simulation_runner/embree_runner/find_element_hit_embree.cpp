#include "find_element_hit_embree.hpp"

#include <cstdint>

// Embree header
#include <embree4/rtcore.h>

#include <simulation_data_export.hpp>

#include "embree_helper.hpp"

namespace SolTrace::EmbreeRunner
{
void FindElementHit_embree(
    // Embree args
    const RTCScene &scene,
    // Ray info
    const int i,
    const uint_fast64_t RayNumber,
    const glm::dvec3 &PosRayGlob,
    const glm::dvec3 &CosRayGlob,
    // outputs
    glm::dvec3 &LastPosRaySurfElement,
    glm::dvec3 &LastCosRaySurfElement,
    glm::dvec3 &LastDFXYZ,
    uint_fast64_t &LastElementNumber,
    uint_fast64_t &LastRayNumber,
    glm::dvec3 &LastPosRaySurfStage,
    glm::dvec3 &LastCosRaySurfStage,
    int &ErrorFlag,
    int &LastHitBackSide,
    bool &StageHit)
{
    // Initialize outputs
    StageHit = false;

    // Make payload object to store intersect outputs
    RayIntersectPayload ray_payload;
    ray_payload.PosRayGlobIn = PosRayGlob; // Copy position (with full double precision)
    ray_payload.CosRayGlobIn = CosRayGlob; // Copy direction (with full double precision)
    rtcInitRayQueryContext(&ray_payload.context);
    RTCIntersectArguments args;
    rtcInitIntersectArguments(&args);
    args.context = &ray_payload.context;
    ray_payload.LastPathLength = std::numeric_limits<double>::infinity();

    // Make rayhit object
    RTCRayHit rayhit;
    rayhit.ray.org_x = PosRayGlob[0];
    rayhit.ray.org_y = PosRayGlob[1];
    rayhit.ray.org_z = PosRayGlob[2];
    rayhit.ray.dir_x = CosRayGlob[0];
    rayhit.ray.dir_y = CosRayGlob[1];
    rayhit.ray.dir_z = CosRayGlob[2];

    // Define rayhit outputs
    rayhit.ray.tnear = 0;
    rayhit.ray.tfar = std::numeric_limits<float>::infinity();
    rayhit.ray.mask = 1u << (i + 1);
    rayhit.ray.flags = 0;
    rayhit.hit.geomID = RTC_INVALID_GEOMETRY_ID;
    rayhit.hit.instID[0] = RTC_INVALID_GEOMETRY_ID;

    // Find intersection
    rtcIntersect1(scene, &rayhit, &args);

    // Check if ray hit any elements
    if (rayhit.hit.geomID != RTC_INVALID_GEOMETRY_ID) {
        // Collect intersection outputs
        StageHit = true;
        LastPosRaySurfElement = ray_payload.LastPosRaySurfElement;
        LastCosRaySurfElement = ray_payload.LastCosRaySurfElement;
        LastDFXYZ = ray_payload.LastDFXYZ;
        LastElementNumber = ray_payload.element_number;
        LastRayNumber = RayNumber;
        LastPosRaySurfStage = ray_payload.LastPosRaySurfStage;
        LastCosRaySurfStage = ray_payload.LastCosRaySurfStage;
        ErrorFlag = ray_payload.ErrorFlag;
        LastHitBackSide = ray_payload.LastHitBackSide;
    }

    // No hit
    else {
        StageHit = false;
    }
    }

} // namespace SolTrace::EmbreeRunner
