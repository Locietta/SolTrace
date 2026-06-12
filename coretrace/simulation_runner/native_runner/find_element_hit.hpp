#ifndef SOLTRACE_FIND_ELEMENT_HIT_H
#define SOLTRACE_FIND_ELEMENT_HIT_H

#include "native_runner_types.hpp"

namespace SolTrace::NativeRunner {

void FindElementHit(
    // stage info
    const int i,
    const tstage_ptr Stage,
    const bool PT_override,
    const bool AsPowerTower,
    // element info
    const int nintelements,
    const std::vector<void *> &sunint_elements,
    const std::vector<void *> &reflint_elements,
    // ray info
    const int RayNumber,
    const bool in_multi_hit_loop,
    glm::dvec3 &PosRayStage,
    glm::dvec3 &CosRayStage,
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
    bool &StageHit);

} // namespace SolTrace::NativeRunner

#endif
