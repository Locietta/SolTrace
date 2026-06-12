#include "determine_element_intersection_new.hpp"

#include "simulation_data_export.hpp"

namespace SolTrace::NativeRunner {

void DetermineElementIntersectionNew(TElement *Element,
                                     glm::dvec3 PosRayIn,
                                     glm::dvec3 CosRayIn,
                                     glm::dvec3 &PosRayOut,
                                     glm::dvec3 &CosRayOut,
                                     glm::dvec3 &DFXYZ,
                                     double *PathLength,
                                     int *ErrorFlag,
                                     int *Intercept,
                                     int *BacksideFlag)
{
    // double x, y;

    *ErrorFlag = 0;

    // find intersection with surface first
    // Intersect(PosRayIn, CosRayIn, Element,
    //           PosRayOut, CosRayOut, DFXYZ, PathLength, ErrorFlag);
    *ErrorFlag = Element->icalc->intersect(PosRayIn, CosRayIn,
                                           PosRayOut, CosRayOut,
                                           DFXYZ, PathLength);
    if (*ErrorFlag > 0 || *PathLength < 0)
    {
        *Intercept = 0;
        PosRayOut = glm::dvec3{0.0};
        CosRayOut = glm::dvec3{0.0};
        DFXYZ = glm::dvec3{0.0};
        *BacksideFlag = 0;
        *PathLength = 0.0;
        return;
    }

    // x = PosRayOut[0];
    // y = PosRayOut[1];

    // // std::cout << "x: " << x << "  y: " << y
    // //           << "\nis in: " << Element->aperture->is_in(x, y)
    // //           << "\nz: " << PosRayOut[2]
    // //           << "\nZAp: " << Element->ZAperture
    // //           << std::endl;

    // if (Element->aperture->is_in(x, y))
    // {
    //     *BacksideFlag = DOT(CosRayIn, DFXYZ) < 0 ? 0 : 1;
    //     *Intercept = 1;
    // }
    // else
    // {
    //     *Intercept = 0;
    //     PosRayOut[0] = 0.0;
    //     PosRayOut[1] = 0.0;
    //     PosRayOut[2] = 0.0;
    //     CosRayOut[0] = 0.0;
    //     CosRayOut[1] = 0.0;
    //     CosRayOut[2] = 0.0;
    //     DFXYZ[0] = 0.0;
    //     DFXYZ[1] = 0.0;
    //     DFXYZ[2] = 0.0;
    //     *PathLength = 0.0;
    //     *ErrorFlag = 0;
    //     *BacksideFlag = 0;
    // }

    *BacksideFlag = glm::dot(CosRayIn, DFXYZ) < 0.0 ? 0 : 1;
    *Intercept = 1;

    // if hit on backside of element then slope of surface is reversed
    if (*BacksideFlag)
    {
        DFXYZ = -DFXYZ;
        // DFXYZ[0] = -DFXYZ[0];
        // DFXYZ[1] = -DFXYZ[1];
        // DFXYZ[2] = -DFXYZ[2];
    }

    return;
}

} // namespace SolTrace::NativeRunner
