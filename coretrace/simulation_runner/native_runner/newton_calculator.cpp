
#include "newton_calculator.hpp"

#include <cmath>
#include <cstdint>

#include "simulation_data_export.hpp"
// #include "glm::dvec3.hpp"

namespace SolTrace::NativeRunner
{

    NewtonCalculator::NewtonCalculator(aperture_ptr ap,
                                       double tol,
                                       uint_fast64_t max_iters)
        : SurfaceIntersectionCalculator(),
          aper(ap),
          tolerance(tol),
          max_iters(max_iters)
    {
    }

    int NewtonCalculator::intersect(const glm::dvec3 PosLoc,
                                    const glm::dvec3 CosLoc,
                                    glm::dvec3 &PosXYZ,
                                    glm::dvec3 &CosKLM,
                                    glm::dvec3 &DFXYZ,
                                    double *PathLength)
    {
        int sts = 1;
        uint_fast64_t count = 0;

        // double x0 = PosLoc[0], y0 = PosLoc[1], z0 = PosLoc[2];
        // double mx = CosLoc[0], my = CosLoc[1], mz = CosLoc[2];
        glm::dvec3 u0(PosLoc);
        glm::dvec3 dt(CosLoc);

        double t0 = 0.0;
        double delta = 0.0;
        // double res;
        double fvalue;
        double fprime;
        glm::dvec3 v0(PosLoc);
        glm::dvec3 dv;

        *PathLength = 0.0;
        PosXYZ = {};
        CosKLM = {};
        DFXYZ = {};

        // this->set_zstart(v0.data);
        this->surface_and_jacobian(v0, &fvalue, dv);
        fprime = glm::dot(dv, dt);

        // std::cout << "Tolerance: " << this->tolerance << std::endl;

        while (fabs(fvalue) > this->tolerance && count <= this->max_iters)
        {
            // std::cout << "**** ITER: " << count << " ****"
            //           << "\nt = " << t0
            //           << "\ndelta = " << delta
            //           << "\nf(t) = " << fvalue
            //           << "\nf'(t) = " << fprime
            //           << "\nu0 = " << v0
            //           << std::endl;
            delta = fvalue / fprime;
            t0 -= delta;
            // Updates v0 to (x0, y0, z0) + t0 * (mx, my, mz)
            v0 = u0 + t0 * dt;
            this->surface_and_jacobian(v0, &fvalue, dv);
            fprime = glm::dot(dv, dt);
            ++count;
        }

        if (fabs(fvalue) <= this->tolerance &&
            this->aper->is_in(v0[0], v0[1]))
        {
            sts = 0;
            *PathLength = t0;
            CosKLM = CosLoc;
            PosXYZ = v0;
            DFXYZ = dv;
        }

        return sts;
    }

} // namespace SolTrace::NativeRunner
