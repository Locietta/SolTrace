#ifndef SOLTRACE_FIND_ELEMENT_HIT_H
#define SOLTRACE_FIND_ELEMENT_HIT_H

#include <cstdint>

#include <embree4/rtcore.h>

#include <glm/vec3.hpp>

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
        bool &StageHit
        );

} // namespace SolTrace::EmbreeRunner

#endif
