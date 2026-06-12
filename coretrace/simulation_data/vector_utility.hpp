#pragma once

#define GLM_ENABLE_EXPERIMENTAL 1
#include <glm/geometric.hpp>
#include <glm/gtx/norm.hpp>
#include <glm/vec3.hpp>

#include <cmath>

namespace SolTrace::Data {

inline double error(const glm::dvec3 &u, const glm::dvec3 &v)
{
    //double err = 0.0;
    //double dx;
    // for (int i = 0; i < 3; ++i) {
    //     dx = u[i] - v[i];
    //     err += dx * dx;
    // }
    return glm::distance(u,v);
    //return sqrt(err);
}

inline double error_inf(const glm::dvec3 &u, const glm::dvec3 &v)
{
    // double err = 0.0;
    // for (int i = 0; i < 3; ++i) {
    //     err = std::max(err, fabs(u[i] - v[i]));
    // }
    // return err;

    return glm::compMax(glm::abs(u - v));
}

inline void normalize_inplace(glm::dvec3 &v)
{
    assert(glm::length2(v) > 0.0);
    v = glm::normalize(v);
}

} // namespace SolTrace::Data
