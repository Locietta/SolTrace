#include "embree_helper.hpp"

#include "bbox_calculator.hpp"

// SimulationData headers
#include <matvec.hpp>

// NativeRunner headers
#include <determine_element_intersection_new.hpp>
#include <native_runner_types.hpp>

#include <embree4/rtcore.h>

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <limits>
#include <string>

namespace SolTrace::EmbreeRunner
{

    using SolTrace::Data::CopyVec3;
    using SolTrace::Data::TransformToLocal;
    using SolTrace::Data::TransformToReference;

    using SolTrace::NativeRunner::TElement;
    using SolTrace::NativeRunner::telement_ptr;
    using SolTrace::NativeRunner::TStage;
    using SolTrace::NativeRunner::tstage_ptr;

    bool compare_Vec3(const glm::dvec3 &v1,
                      const glm::dvec3 &v2,
                      double rel_tol,
                      double abs_tol = 1e-5)
    {
        auto diff = v1 - v2;
        auto scale = glm::max(glm::abs(v1), glm::abs(v2));

        auto rel_tol_v = glm::max(glm::dvec3(abs_tol), rel_tol * scale);

        if (glm::any(glm::greaterThan(diff, rel_tol_v))) {
            return false;
        }
        return true;
    }

    void error_function(void *userPtr,
                        RTCError error,
                        const char *str)
    {
        printf("error %d: %s\n", error, str);
    }

    int bounds_error(const RTCBoundsFunctionArguments *args)
    {
        // Set bounds to zero and report error
        RTCBounds *bounds_o = args->bounds_o;
        bounds_o->lower_x = std::numeric_limits<float>::quiet_NaN();
        bounds_o->upper_x = std::numeric_limits<float>::quiet_NaN();
        bounds_o->lower_y = std::numeric_limits<float>::quiet_NaN();
        bounds_o->upper_y = std::numeric_limits<float>::quiet_NaN();
        bounds_o->lower_z = std::numeric_limits<float>::quiet_NaN();
        bounds_o->upper_z = std::numeric_limits<float>::quiet_NaN();

        error_function(args->geometryUserPtr,
                       RTC_ERROR_INVALID_OPERATION,
                       "Bounding box computation failed for some element");

        throw std::runtime_error("Embree scene commit failed");

        return 0;
    }

    void bounds_function(const RTCBoundsFunctionArguments *args)
    {
        // Get element
        TElement *st_element = (TElement *)args->geometryUserPtr;

        // Get bounds
        glm::vec3 min_coord_global;
        glm::vec3 max_coord_global;
        bool success = get_bounds(st_element,
                                  min_coord_global,
                                  max_coord_global);

        // Check error
        if (!success)
        {
            bounds_error(args);
        }

        // Assign bounds
        RTCBounds *bounds_o = args->bounds_o;
        bounds_o->lower_x = min_coord_global[0];
        bounds_o->upper_x = max_coord_global[0];
        bounds_o->lower_y = min_coord_global[1];
        bounds_o->upper_y = max_coord_global[1];
        bounds_o->lower_z = min_coord_global[2];
        bounds_o->upper_z = max_coord_global[2];
    }

    void intersect_function(const RTCIntersectFunctionNArguments *args)
    {
        // Get payload object
        RayIntersectPayload *payload = (RayIntersectPayload *)args->context;

        glm::dvec3 PosRayGlob, CosRayGlob;
        PosRayGlob = payload->PosRayGlobIn;
        CosRayGlob = payload->CosRayGlobIn;

        // Get Element data
        TElement *st_element = (TElement *)args->geometryUserPtr;
        tstage_ptr const& st_stage = st_element->parent_stage;

        // First, convert ray coordinates to element
        // Global -> stage -> element
        // transform the global incoming ray to local stage coordinates
        glm::dvec3 PosRayStage, CosRayStage;
        TransformToLocal(PosRayGlob, CosRayGlob,
                         st_stage->Origin, st_stage->RRefToLoc,
                         PosRayStage, CosRayStage);

        //  {Transform ray to element[j] coord system of Stage[i]}
        glm::dvec3 PosRayElement, CosRayElement;
        TransformToLocal(PosRayStage, CosRayStage,
                         st_element->Origin, st_element->RRefToLoc,
                         PosRayElement, CosRayElement);

        // Increment position by tiny amount to get off the element if 
        // tracing to the same element.
        PosRayElement += 1.0e-4 * CosRayElement;

        // Call DeterminElementIntersectionNew
        glm::dvec3 PosRaySurfElement = {0.0, 0.0, 0.0};
        glm::dvec3 CosRaySurfElement = {0.0, 0.0, 0.0};
        glm::dvec3 DFXYZ = {0.0, 0.0, 0.0};
        double PathLength = 0;

        int InterceptFlag = 0;
        int HitBackSide = 0;
        SolTrace::NativeRunner::DetermineElementIntersectionNew(
            st_element, PosRayElement, CosRayElement,
            PosRaySurfElement, CosRaySurfElement, DFXYZ,
            &PathLength, &payload->ErrorFlag, &InterceptFlag, &HitBackSide);

        // Update rayhit info (if hit)
        if (InterceptFlag != 0)
        {
            // Get rayhit data
            RTCRayHit *rayhit_out = (RTCRayHit *)args->rayhit;

            // Check if hit is closer than other hits
            // Using payload pathlength for double precision
            // If pathlength == lastpathlength, use which element number is lower
            // to match results with original code
            if ((PathLength < payload->LastPathLength) ||
                ((PathLength == payload->LastPathLength) &&
                 (st_element->element_number < payload->element_number)))
            {

                if (PosRaySurfElement[2] <= st_element->ZAperture)
                {

                    // Transform ray back to stage coordinate system
                    glm::dvec3 PosRaySurfStage = {0.0, 0.0, 0.0};
                    glm::dvec3 CosRaySurfStage = {0.0, 0.0, 0.0};
                    TransformToReference(PosRaySurfElement, CosRaySurfElement,
                                         st_element->Origin, st_element->RLocToRef,
                                         PosRaySurfStage, CosRaySurfStage);

                    rayhit_out->ray.tfar = (float)PathLength; // Update intersection distance
                    rayhit_out->hit.geomID = args->geomID;
                    rayhit_out->hit.primID = 0; // Single primitive

                    // Define Ng (will not be used)
                    rayhit_out->hit.Ng_x = std::numeric_limits<float>::quiet_NaN();
                    rayhit_out->hit.Ng_y = std::numeric_limits<float>::quiet_NaN();
                    rayhit_out->hit.Ng_z = std::numeric_limits<float>::quiet_NaN();

                    // Assign custom outputs
                    payload->LastHitBackSide = HitBackSide;
                    payload->LastDFXYZ = DFXYZ;
                    // CopyVec3(payload->LastPosRaySurfGlob, PosRaySurfGlob);
                    // CopyVec3(payload->LastCosRaySurfGlob, CosRaySurfGlob);
                    payload->LastPosRaySurfStage = PosRaySurfStage;
                    payload->LastCosRaySurfStage = CosRaySurfStage;
                    payload->LastPosRaySurfElement = PosRaySurfElement;
                    payload->LastCosRaySurfElement = CosRaySurfElement;
                    payload->element_number = st_element->element_number;
                    payload->LastPathLength = PathLength;
                }
            }
        }

        return;
    }

    RTCScene make_scene(RTCDevice &device,
                        SolTrace::NativeRunner::TSystem &system)
    {
        // Make scene
        RTCScene scene = rtcNewScene(device);

        // Loop through stages
        unsigned int stage_index = 1;
        for (tstage_ptr stage : system.StageList)
        {
            // Loop through elements in each stage
            unsigned int stage_mask = 1u << stage_index;
            for (telement_ptr st_element : stage->ElementList)
            {
                // Make embree geometry for each element
                RTCGeometry embree_geom = rtcNewGeometry(device, RTC_GEOMETRY_TYPE_USER);
                rtcSetGeometryUserPrimitiveCount(embree_geom, 1);

                // Attach st element to embree geometry
                rtcSetGeometryUserData(embree_geom, st_element.get());

                // Assign bounds function
                rtcSetGeometryBoundsFunction(embree_geom, bounds_function, NULL);

                // Assign intersect function
                rtcSetGeometryIntersectFunction(embree_geom, intersect_function);

                // Set mask (stage number)
                rtcSetGeometryMask(embree_geom, stage_mask);

                // Commit geometry
                rtcCommitGeometry(embree_geom);

                // Attach geometry
                unsigned int geomID = rtcAttachGeometry(scene, embree_geom);

                // Release geometry (it is owned by the scene now)
                rtcReleaseGeometry(embree_geom);
            }

            stage_index++;
        }

        return scene;
    }

    bool validate_intersect(glm::dvec3 &LastPosRaySurfElement1,
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
                            bool &StageHit2)
    {
        if (StageHit1 == false && StageHit2 == false)
            return true;

        double tol_diff = 1e-4;
        if (StageHit1 != StageHit2)
            return false;
        if (LastElementNumber1 != LastElementNumber2)
            return false;
        if (LastHitBackSide1 != LastHitBackSide2)
            return false;
        if (!compare_Vec3(LastPosRaySurfElement1, LastPosRaySurfElement2, tol_diff))
            return false;
        if (!compare_Vec3(LastCosRaySurfElement1, LastCosRaySurfElement2, tol_diff))
            return false;
        if (!compare_Vec3(LastDFXYZ1, LastDFXYZ2, tol_diff))
            return false;
        if (!compare_Vec3(LastPosRaySurfStage1, LastPosRaySurfStage2, tol_diff))
            return false;
        if (!compare_Vec3(LastCosRaySurfStage1, LastCosRaySurfStage2, tol_diff))
            return false;

        return true;
    }

} // namespace SolTrace::EmbreeRunner
