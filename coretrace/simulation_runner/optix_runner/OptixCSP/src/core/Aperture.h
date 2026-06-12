#pragma once
#include "soltrace_type.h"  // For ApertureType enum
#include "vec3d.h"


namespace OptixCSP {
    // Forward declarations
    class Element;
    class Vec3d;
    class Matrix33d;


    // Abstract base class for apertures (e.g., rectangle, circle)
    class Aperture {
    public:
        Aperture();
        virtual ~Aperture() = default;

        virtual ApertureType get_aperture_type() const = 0;

        virtual double get_width() const;
        virtual double get_height() const;
        virtual double get_radius() const;

        //// interface for defining the size of the aperture for device data
        //virtual void compute_device_aperture(Vec3d pos, Vec3d normal) = 0; 

        // get anchor, v1 and v2
        //virtual float3 get_origin() = 0;
        //virtual float3 get_v1() = 0;
        //virtual float3 get_v2() = 0;
    };

    // Concrete class for a circular aperture.
    class ApertureCircle : public Aperture {
    public:
        ApertureCircle();
        ApertureCircle(double r);
        virtual ~ApertureCircle() = default;

        virtual ApertureType get_aperture_type() const override;
        void set_size(double r);
        virtual double get_radius() const override;
        // virtual double get_width() const override;
        // virtual double get_height() const override;

    private:
        double radius;
    };

    // Concrete class for a circular aperture.
    class ApertureHexagon : public Aperture {
    public:
        ApertureHexagon();
        ApertureHexagon(double s);
        virtual ~ApertureHexagon() = default;

        virtual ApertureType get_aperture_type() const override;
        virtual double get_side_length() const;

    private:
        double side_length;
    };

    // Concrete class for an easy rectangular aperture.
    class ApertureRectangle : public Aperture {
    public:
        //ApertureRectangle();
        ApertureRectangle(double xlen, double ylen, double xl, double yl);
        virtual ~ApertureRectangle() = default;

        virtual ApertureType get_aperture_type() const override;
        //virtual float3 get_origin() override;
        //virtual float3 get_v1() override;
        //virtual float3 get_v2() override;
        virtual double get_width() const override;
        virtual double get_height() const override;
        double get_x_coord() const;
        double get_y_coord() const;

    private:
        double x_length;
        double y_length;
        double x_coord;
        double y_coord;

        //float3 m_origin;
        //float3 m_x_axis;
        //float3 m_y_axis;
    };


    class ApertureTriangle : public Aperture {
    public:
        ApertureTriangle();
		// input three vertices in 3D space
		// sequence matters, front side is determined by right-hand rule
        ApertureTriangle(Vec3d v0, Vec3d v1, Vec3d v2);
        virtual ~ApertureTriangle() = default;
        virtual ApertureType get_aperture_type() const override;
        Vec3d get_v0() const;
		Vec3d get_v1() const;
        Vec3d get_v2() const;

    private:
        Vec3d m_v0;
        Vec3d m_v1;
		Vec3d m_v2;
    };

    class ApertureQuadrilateral : public Aperture {
    public:
        ApertureQuadrilateral();
        // input four vertices in 3D space
		// sequence matters, front side is determined by right-hand rule
        ApertureQuadrilateral(Vec3d p0, Vec3d p1, Vec3d p2, Vec3d p3);
        virtual ~ApertureQuadrilateral() = default;
        virtual ApertureType get_aperture_type() const override;
        Vec3d get_p0() const;
        Vec3d get_p1() const;
        Vec3d get_p2() const;
        Vec3d get_p3() const;

    private:
        Vec3d m_p0;
        Vec3d m_p1;
        Vec3d m_p2;
        Vec3d m_p3;
    };

    class ApertureAnnulus : public Aperture {
    public:
        ApertureAnnulus();
      ApertureAnnulus(double ri, double ro, double arc);
        virtual ~ApertureAnnulus() = default;
        virtual ApertureType get_aperture_type() const override;
        double get_radius_inner() const;
        double get_radius_outer() const;
      double get_arc() const;
    private:
        double ri;
        double ro;
      double arc; // Measured in radians
    };

}
