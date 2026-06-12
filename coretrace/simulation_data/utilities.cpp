
#include "utilities.hpp"

#include <cassert>
#include <cmath>

#include "constants.hpp"

#include <glm/glm.hpp>

namespace SolTrace::Data {

void project_onto_plane(const glm::dvec3 &n,
                        const glm::dvec3 &u,
                        glm::dvec3 &uproj)
{
    double nmag = glm::length(n);
    double coef = -glm::dot(n, u) / (nmag * nmag);
    uproj = u + coef * n;
    return;
}

void project_onto_plane(const glm::dvec3 &n,
                        glm::dvec3 &u)
{
    double nmag = glm::length(n);
    double coef = -glm::dot(n,u) / (nmag * nmag);
    u = coef * n + u;
    return;
}

void project_onto_vector(const glm::dvec3 &u,
                         const glm::dvec3 &v,
                         glm::dvec3 &vproj)
{
    double umag = glm::length(u);
    double coef = glm::dot(u, v) / (umag * umag);
    vproj = coef * u;
    return;
}

void project_onto_vector(const glm::dvec3 &u,
                         glm::dvec3 &v)
{
    double umag = glm::length(u);
    double coef = glm::dot(u, v) / (umag * umag);
    v = coef * u;
    return;
}

void rotate_vector_degrees(const glm::dvec3 &k,
                           const glm::dvec3 &v,
                           double theta,
                           glm::dvec3 &vrot)
{
    theta *= D2R;
    return rotate_vector_radians(k, v, theta, vrot);
}

void rotate_vector_radians(const glm::dvec3 &k,
                           const glm::dvec3 &v,
                           double theta,
                           glm::dvec3 &vrot)
{
    assert(fabs(glm::length(k) - 1.0) < 1e-12);

    glm::dvec3 scratch = glm::cross(k,v);

    vrot = sin(theta) * scratch + cos(theta) * v;

    double coef = (1.0 - cos(theta)) * glm::dot(k, v);
    vrot = coef* k + vrot;

    return;
}

void sun_position_vector_degrees(glm::dvec3 &sun_pos,
                                 double azimuth,
                                 double elevation)
{
    return sun_position_vector_radians(sun_pos,
                                       D2R * azimuth,
                                       D2R * elevation);
}

void sun_position_vector_radians(glm::dvec3 &sun_pos,
                                 double azimuth,
                                 double elevation)
{
    double x = sin(azimuth) * cos(elevation);
    double y = cos(azimuth) * cos(elevation);
    double z = sin(elevation);
    sun_pos = glm::dvec3(x, y, z);
    return;
}

} // namespace SolTrace::Data
