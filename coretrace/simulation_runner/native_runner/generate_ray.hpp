#ifndef SOLTRACE_GENERATE_RAY_H
#define SOLTRACE_GENERATE_RAY_H

#include "mtrand.hpp"
#include "native_runner_types.hpp"

namespace SolTrace::NativeRunner {

void GenerateRay(MTRand& myrng,
                 const glm::dvec3& PosSunStage,
                 glm::dvec3& Origin,
                 glm::dmat3& RLocToRef,
                 TSun* Sun,
                 uint_fast64_t sample_index,
                 glm::dvec3& PosRayGlobal,
                 glm::dvec3& CosRayGlobal,
                 glm::dvec3& PosRaySun,
                 int& ErrorFlag);


} // namespace SolTrace::NativeRunner

#endif
