
#include "simulation_data_export.hpp"

#include "determine_element_intersection_new.hpp"
#include "find_element_hit.hpp"
#include "native_runner_types.hpp"

namespace SolTrace::NativeRunner
{

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
		bool &StageHit)
	{
		// Initialize Variables
		double LastPathLength = 1e99;
		int HitBackSide = 0;
		int InterceptFlag = 0;
        glm::dvec3 DFXYZ = glm::dvec3{0.0};
        glm::dvec3 PosRayElement = glm::dvec3{0.0};
        glm::dvec3 CosRayElement = glm::dvec3{0.0};
        glm::dvec3 PosRaySurfStage = glm::dvec3{0.0};
        glm::dvec3 CosRaySurfStage = glm::dvec3{0.0};
        glm::dvec3 PosRaySurfElement = glm::dvec3{0.0};
        glm::dvec3 CosRaySurfElement = glm::dvec3{0.0};
        StageHit = false;

        for (uint_fast64_t j = 0; j < nintelements; j++)
		{
			TElement *Element; // = Stage->ElementList[j];
			if (i == 0 && !PT_override)
			{
				if (in_multi_hit_loop)
				{
					if (AsPowerTower)
					{
						Element = (TElement *)reflint_elements.at(j);
					}
					else
					{
						Element = Stage->ElementList[j].get();
					}
				}
				else
				{
					Element = (TElement *)sunint_elements.at(j);
				}
			}
			else
			{
				Element = Stage->ElementList[j].get();
			}

			// if (!Element->Enabled)
			// 	continue;

			//  {Transform ray to element[j] coord system of Stage[i]}
            Data::TransformToLocal(PosRayStage,
                                   CosRayStage,
                                   Element->Origin,
                                   Element->RRefToLoc,
                                   PosRayElement,
                                   CosRayElement);

            ErrorFlag = 0;
            HitBackSide = 0;
            InterceptFlag = 0;
			double PathLength = 0;

			// increment position by tiny amount to get off the element
			// if tracing to the same element
            PosRayElement = PosRayElement + 1.0e-5 * CosRayElement;
            // PosRayElement[0] = PosRayElement[0] + 1.0e-5 * CosRayElement[0];
            // PosRayElement[1] = PosRayElement[1] + 1.0e-5 * CosRayElement[1];
            // PosRayElement[2] = PosRayElement[2] + 1.0e-5 * CosRayElement[2];

            // {Determine if ray intersects element[j]; if so, Find intersection
			// point with surface of element[j] }
			DetermineElementIntersectionNew(Element,
											PosRayElement,
											CosRayElement,
											PosRaySurfElement,
											CosRaySurfElement,
											DFXYZ,
											&PathLength,
											&ErrorFlag,
											&InterceptFlag,
											&HitBackSide);

			if (InterceptFlag)
			{
				// {If hit multiple elements, this loop determines which one hit
				// first. Also makes sure that correct part of closed surface is
				// hit. Also, handles wavy, but close to flat zernikes and
				// polynomials correctly.}
				if (PathLength < LastPathLength)
				{
					// if (PosRaySurfElement[2] <= Element->ZAperture ||
					// 	Element->SurfaceIndex == 'm' ||
					// 	Element->SurfaceIndex == 'M' ||
					// 	Element->SurfaceIndex == 'r' ||
					// 	Element->SurfaceIndex == 'R')
					// TODO: Is this the correct thing to do?
                    if (PosRaySurfElement.z <= Element->ZAperture)
                    {
                        StageHit = true;
						LastPathLength = PathLength;
                        LastPosRaySurfElement = PosRaySurfElement;
                        LastCosRaySurfElement = CosRaySurfElement;
                        LastDFXYZ = DFXYZ;
						// LastElementNumber = ((i == 0 && !PT_override) ? Element->element_number : j + 1); // mjw change from j index to element id
						LastElementNumber = Element->element_number;
						LastRayNumber = RayNumber;
                        Data::TransformToReference(PosRaySurfElement,
                                                   CosRaySurfElement,
                                                   Element->Origin,
                                                   Element->RLocToRef,
                                                   PosRaySurfStage,
                                                   CosRaySurfStage);

                        LastPosRaySurfStage = PosRaySurfStage;
                        LastCosRaySurfStage = CosRaySurfStage;
						LastHitBackSide = HitBackSide;
                    }
                }
			}
		}
	}

} // namespace SolTrace::NativeRunner
