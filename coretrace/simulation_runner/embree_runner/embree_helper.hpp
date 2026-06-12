#ifndef SOLTRACE_EMBREE_HELPER_H
#define SOLTRACE_EMBREE_HELPER_H

#include <embree4/rtcore.h>

#include <native_runner_types.hpp>

#include <cstdint>
#include <limits>

namespace SolTrace::EmbreeRunner
{

    struct RayIntersectPayload
    {
        RTCRayQueryContext context; // Embree built-in context (MUST come first)

        int LastHitBackSide = -1;
        glm::dvec3 LastDFXYZ{0.0};

        // glm::dvec3 LastPosRaySurfGlob{0.0};
        // glm::dvec3 LastCosRaySurfGlob{0.0};

        glm::dvec3 LastPosRaySurfStage{0.0};
        glm::dvec3 LastCosRaySurfStage{0.0};

        glm::dvec3 LastPosRaySurfElement{0.0};
        glm::dvec3 LastCosRaySurfElement{0.0};

        int_fast64_t element_number = -1;
        int ErrorFlag = 0;

        glm::dvec3 PosRayGlobIn{0.0};
        glm::dvec3 CosRayGlobIn{0.0};

        double LastPathLength = std::numeric_limits<double>::infinity();
    };


	inline unsigned int embree_mask(int_fast64_t stage_index)
	{
		return 1u << stage_index;
	}

	void error_function(void *userPtr,
						RTCError error,
						const char *str);

	RTCScene make_scene(RTCDevice &device,
						SolTrace::NativeRunner::TSystem &system);

    bool validate_intersect(
        glm::dvec3 &LastPosRaySurfElement1,
        glm::dvec3 &LastCosRaySurfElement1,
        glm::dvec3 &LastDFXYZ1,
        uint_fast64_t &LastElementNumber1,
        uint_fast64_t &LastRayNumber1,
        glm::dvec3 &LastPosRaySurfStage1,
        glm::dvec3 &LastCosRaySurfStage1,
        int &ErrorFlag1,
        int &LastHitBackSide1,
        bool &StageHit1,

        glm::dvec3 &LastPosRaySurfElement2,
        glm::dvec3 &LastCosRaySurfElement2,
        glm::dvec3 &LastDFXYZ2,
        uint_fast64_t &LastElementNumber2,
        uint_fast64_t &LastRayNumber2,
        glm::dvec3 &LastPosRaySurfStage2,
        glm::dvec3 &LastCosRaySurfStage2,
        int &ErrorFlag2,
        int &LastHitBackSide2,
        bool &StageHit2
    );


} // namespace SolTrace::EmbreeRunner

#endif
