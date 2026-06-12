#ifndef SOLTRACE_FLAT_CALCULATOR_H
#define SOLTRACE_FLAT_CALCULATOR_H

#include "aperture.hpp"
#include "surface.hpp"
#include "surface_intersection_calculator.hpp"

namespace SolTrace::NativeRunner
{

    class FlatCalculator : public SurfaceIntersectionCalculator
    {
    public:
        FlatCalculator(SolTrace::Data::surface_ptr surf,
                       SolTrace::Data::aperture_ptr ap);
        virtual ~FlatCalculator();
        virtual int intersect(const glm::dvec3 PosLoc,
                              const glm::dvec3 CosLoc,
                              glm::dvec3 &PosXYZ,
                              glm::dvec3 &CosKLM,
                              glm::dvec3 &DFXYZ,
                              double *PathLength) override;

        virtual double compute_z_aperture(SolTrace::Data::aperture_ptr ap) override;

    private:
        SolTrace::Data::aperture_ptr aper;
    };

} // namespace SolTrace::NativeRunner

#endif
