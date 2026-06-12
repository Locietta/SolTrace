#include "Aperture.h"
#include "constants.hpp"

using namespace OptixCSP;

class Element;

// Aperture base class implementations
Aperture::Aperture() = default;

double Aperture::get_width() const { return 0.0; }
double Aperture::get_height() const { return 0.0; }
double Aperture::get_radius() const { return 0.0; }


// ApertureCircle implementations
ApertureCircle::ApertureCircle() : radius(1.0) {}
ApertureCircle::ApertureCircle(double r) : radius(r) {}

ApertureType ApertureCircle::get_aperture_type() const {
    return ApertureType::CIRCLE;
}

void ApertureCircle::set_size(double r) { 
    radius = r; 
}

double ApertureCircle::get_radius() const { 
    return radius; 
}

// double ApertureCircle::get_width() const { 
//     return 2.0 * radius; 
// }

// double ApertureCircle::get_height() const { 
//     return 2.0 * radius; 
// }

// ApertureHexagon implementations
ApertureHexagon::ApertureHexagon() : side_length(1.0) {}
ApertureHexagon::ApertureHexagon(double s) : side_length(s) {}

ApertureType ApertureHexagon::get_aperture_type() const {
    return ApertureType::HEXAGON;
}

double ApertureHexagon::get_side_length() const {
    return side_length;
}

// ApertureRectangleEasy implementations
//ApertureRectangle::ApertureRectangle() : x_dim(1.0), y_dim(1.0) {}
ApertureRectangle::ApertureRectangle(double xlen, double ylen, double xl, double yl) 
    : x_length(xlen), y_length(ylen), x_coord(xl), y_coord(yl)
{
}

ApertureType ApertureRectangle::get_aperture_type() const {
    return ApertureType::RECTANGLE;
}

//float3 ApertureRectangle::get_origin() { return m_origin; }
//float3 ApertureRectangle::get_v1() { return m_x_axis; }
//float3 ApertureRectangle::get_v2() { return m_y_axis; }
double ApertureRectangle::get_width() const { return x_length; }
double ApertureRectangle::get_height() const { return y_length; }
double ApertureRectangle::get_x_coord() const { return x_coord; }
double ApertureRectangle::get_y_coord() const { return y_coord; }

// Aperture Triangle
ApertureTriangle::ApertureTriangle(Vec3d v0, Vec3d v1, Vec3d v2) : m_v0(v0), m_v1(v1), m_v2(v2) {}
ApertureTriangle::ApertureTriangle() : m_v0(0, 0, 0), m_v1(1, 0, 0), m_v2(0, 1, 0) {}
ApertureType ApertureTriangle::get_aperture_type() const { 
    return ApertureType::TRIANGLE; 
}

Vec3d ApertureTriangle::get_v0() const { return m_v0; }
Vec3d ApertureTriangle::get_v1() const { return m_v1; }
Vec3d ApertureTriangle::get_v2() const { return m_v2; }


ApertureQuadrilateral::ApertureQuadrilateral(Vec3d p0, Vec3d p1, Vec3d p2, Vec3d p3) : m_p0(p0), m_p1(p1), m_p2(p2), m_p3(p3) {}
ApertureQuadrilateral::ApertureQuadrilateral() : m_p0(0,0,0), m_p1(1,0,0), m_p2(1,1,0), m_p3(0,1,0) {}
ApertureType ApertureQuadrilateral::get_aperture_type() const {
  return ApertureType::QUADRILATERAL;
}

Vec3d ApertureQuadrilateral::get_p0() const { return m_p0; }
Vec3d ApertureQuadrilateral::get_p1() const { return m_p1; }
Vec3d ApertureQuadrilateral::get_p2() const { return m_p2; }
Vec3d ApertureQuadrilateral::get_p3() const { return m_p3; }

ApertureAnnulus::ApertureAnnulus() : ri(0.5), ro(1.0), arc(2.0 * SolTrace::Data::PI) {}
ApertureAnnulus::ApertureAnnulus(double r_inner, double r_outer, double arc)
  : ri(r_inner), ro(r_outer), arc(arc) {}
ApertureType ApertureAnnulus::get_aperture_type() const {
    return ApertureType::ANNULUS;
}
double ApertureAnnulus::get_radius_inner() const { return ri; }
double ApertureAnnulus::get_radius_outer() const { return ro; }
double ApertureAnnulus::get_arc() const { return arc; }
