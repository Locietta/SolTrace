#ifndef SOLTRACE_SPHERE_CALCULATOR_H
#define SOLTRACE_SPHERE_CALCULATOR_H

#include "surface_intersection_calculator.hpp"

#include "aperture.hpp"
#include "surface.hpp"

namespace SolTrace::NativeRunner
{

    class SphereCalculator : public SurfaceIntersectionCalculator
    {
    public:
        SphereCalculator(SolTrace::Data::surface_ptr surf,
                         SolTrace::Data::aperture_ptr ap);
        virtual ~SphereCalculator() {}
        virtual int intersect(const glm::dvec3 PosLoc,
                              const glm::dvec3 CosLoc,
                              glm::dvec3& PosXYZ,
                              glm::dvec3& CosKLM,
                              glm::dvec3& DFXYZ,
                              double *PathLength) override;

        void surface_normal(const glm::dvec3 PosXYZ, glm::dvec3& DFXYZ);

        virtual double compute_z_aperture(SolTrace::Data::aperture_ptr ap) override;

    private:
        double radius;
        double curvature;

        SolTrace::Data::aperture_ptr aper;
    };

} // namespace SolTrace::NativeRunner

#endif
