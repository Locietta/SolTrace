#ifndef SOLTRACE_NEWTON_CALCULATOR_H
#define SOLTRACE_NEWTON_CALCULATOR_H

#include <cstdint>

#include "aperture.hpp"
#include "surface_intersection_calculator.hpp"

// #include "surface.hpp"

namespace SolTrace::NativeRunner
{

    class NewtonCalculator : public SurfaceIntersectionCalculator
    {
    public:
        NewtonCalculator(SolTrace::Data::aperture_ptr ap,
                         double tol = 1e-6,
                         uint_fast64_t max_iters = 20);
        virtual ~NewtonCalculator() {}

        virtual int intersect(const glm::dvec3 PosLoc,
                              const glm::dvec3 CosLoc,
                              glm::dvec3 &PosXYZ,
                              glm::dvec3 &CosKLM,
                              glm::dvec3 &DFXYZ,
                              double *PathLength) override;

        // For x = PosXYZ[0], y = PosXYZ[1], make a guess at
        // value of z and place in PosXYZ[2].
        virtual void set_zstart(glm::dvec3 &PosXYZ) = 0;
        virtual void surface_and_jacobian(glm::dvec3 PosXYZ,
                                          double *F,
                                          glm::dvec3 &DFXYZ) = 0;

        inline double get_tolerance() const
        {
            return this->tolerance;
        }
        inline uint_fast64_t get_max_iters() const
        {
            return this->max_iters;
        }

    private:
        SolTrace::Data::aperture_ptr aper;
        double tolerance;
        uint_fast64_t max_iters;
    };

} // namespace SolTrace::NativeRunner

#endif
