#pragma once
#include "device_util.h"
#include "soltrace_constants.h"
#ifndef __CUDACC_RTC__
#include <cassert>
#include <cstdint>
#else
#define assert(x) /*nop*/
#endif

// TODO: get rid of ST suffix, no clue what it was for ...

namespace OptixCSP
{
    struct GeometryDataST
    {
        enum Type
        {
            PARALLELOGRAM = 0,
            CYLINDER_Y = 1,
            RECTANGLE_PARABOLIC = 2,
            UNKNOWN_TYPE = 3,
            RECTANGLE_FLAT = 4,
            TRIANGLE_FLAT = 5,
            QUADRILATERAL_FLAT = 6,
            CIRCLE_FLAT = 7,
            HEXAGON_FLAT = 8,
	    ANNULUS_FLAT = 9
        };

        struct Parallelogram
        {
            Parallelogram() = default;
            Parallelogram(float3 v1, float3 v2, float3 anchor)
                : v1(v1), v2(v2), anchor(anchor)
            {
                float3 normal = normalize(cross(v1, v2));
                float d = dot(normal, anchor);
                this->v1 *= 1.0f / dot(v1, v1);
                this->v2 *= 1.0f / dot(v2, v2);
                plane = make_float4(normal, d);
            }
            float4 plane;
            float3 v1;
            float3 v2;
            float3 anchor;
        };

        // same as parallelogram, however defined with different attributes
        struct Rectangle_Flat
        {
            Rectangle_Flat() = default;
            Rectangle_Flat(float3 center, float3 x, float3 y, float width, float height)
                : center(center), x(x), y(y), width(width), height(height)
            {
                float3 normal = normalize(cross(x, y));
                float d = dot(normal, center);
                plane = make_float4(normal, d);
            }

            float4 plane;
            float3 center;
            float3 x;
            float3 y;
            float width;
            float height;
        };

        struct Cylinder_Y
        {
            Cylinder_Y() = default;
            Cylinder_Y(float3 center, float radius, float half_height, float3 base_x, float3 base_z)
                : center(center), radius(radius), half_height(half_height), base_x(base_x), base_z(base_z)
            {
                assert(dot(base_x, base_z) < 1e-3f);
            }

            float3 center;
            float radius;
            float half_height;
            float3 base_x; // x axis of the cylinder
            float3 base_z; // z axis of the cylinder
        };

        struct Rectangle_Parabolic
        {

            Rectangle_Parabolic() = default;
            Rectangle_Parabolic(float3 v1, float3 v2, float3 anchor, float curv_x, float curv_y)
                : v1(v1), v2(v2), anchor(anchor), curv_x(curv_x), curv_y(curv_y)
            {
                float3 normal = normalize(cross(v1, v2));
                float d = dot(normal, anchor);
                this->v1 *= 1.0f / dot(v1, v1);
                this->v2 *= 1.0f / dot(v2, v2);
                plane = make_float4(normal, d);
            }

            float4 plane;
            float3 v1;
            float3 v2;
            float3 anchor;
            // float3 focus;
            float curv_x;
            float curv_y;
        };

        struct Triangle_Flat
        {
            Triangle_Flat() = default;
            Triangle_Flat(const float3 &a, const float3 &b, const float3 &c)
                : v0(a), e1(b - a), e2(c - a)
            {
                normal = normalize(cross(e1, e2));
                d = dot(normal, v0);
            }
            float3 v0;     // base vertex
            float3 e1, e2; // edges
            float3 normal;
            float d; // plane distance
        };

        struct Quadrilateral_Flat
        {
            Quadrilateral_Flat() = default;
            Quadrilateral_Flat(const float3 &a, const float3 &b,
                               const float3 &c, const float3 &d)
                : p0(a), p1(b), p2(c), p3(d)
            {
                float3 e1 = p1 - p0;
                float3 e2 = p3 - p0;
                normal = normalize(cross(e1, e2));
            }
            float3 p0, p1, p2, p3; // Vertices in counterclockwise order
            float3 normal;         // Positive direction follows right-hand rule
        };

        struct Circle_Flat
        {
            Circle_Flat() = default;
            // Circle_Flat(const float radius) : r(radius) {}
            Circle_Flat(const float3 &origin, const float3 &normal, const float &radius)
                : r(radius), center(origin)
            {
                plane = make_float4(normalize(normal), dot(center, normal));
            }
            float4 plane;
            float3 center;
            float r;
        };

        struct Hexagon_Flat
        {
            Hexagon_Flat() = default;
            Hexagon_Flat(const float3 &origin, const float3 &normal, const float &side_length)
                : s(side_length), center(origin)
            {
                plane = make_float4(normalize(normal), dot(center, normal));
            }
            float4 plane;
            float3 center;
            float s;
        };

        struct Annulus_Flat
        {
            Annulus_Flat() = default;
            Annulus_Flat(const float3 &origin, const float3 &normal,
                         const float &r_inner, const float &r_outer, const float &arc)
	      : center(origin), ri(r_inner), ro(r_outer), arc(arc)
            {
                plane = make_float4(normalize(normal), dot(center, normal));
            }
            float4 plane;
            float3 center;
            float ri;
            float ro;
	  float arc; // Arc angle in radians with x-axis in the middle
        };

        GeometryDataST() = default;

        void setParallelogram(const Parallelogram &p)
        {
            assert(type == UNKNOWN_TYPE);
            type = PARALLELOGRAM;
            parallelogram = p;
        }

        __host__ __device__ const Parallelogram &getParallelogram() const
        {
            assert(type == PARALLELOGRAM);
            return parallelogram;
        }

        void setRectangle_Flat(const Rectangle_Flat &r)
        {
            assert(type == UNKNOWN_TYPE);
            type = RECTANGLE_FLAT;
            rectangle_flat = r;
        }

        __host__ __device__ const Rectangle_Flat &getRectangle_Flat() const
        {
            assert(type == RECTANGLE_FLAT);
            return rectangle_flat;
        }

        void setCylinder_Y(const Cylinder_Y &c)
        {
            assert(type == UNKNOWN_TYPE);
            type = CYLINDER_Y;
            cylinder_y = c;
        }

        __host__ __device__ const Cylinder_Y &getCylinder_Y() const
        {
            assert(type == CYLINDER_Y);
            return cylinder_y;
        }

        void setRectangleParabolic(const Rectangle_Parabolic &r)
        {
            assert(type == UNKNOWN_TYPE);
            type = RECTANGLE_PARABOLIC;
            rectangle_parabolic = r;
        }

        __host__ __device__ const Rectangle_Parabolic &getRectangleParabolic() const
        {
            assert(type == RECTANGLE_PARABOLIC);
            return rectangle_parabolic;
        }

        void setTriangle_Flat(const Triangle_Flat &t)
        {
            assert(type == UNKNOWN_TYPE);
            type = TRIANGLE_FLAT;
            triangle_flat = t;
        }

        __host__ __device__ const Triangle_Flat &getTriangle_Flat() const
        {
            assert(type == TRIANGLE_FLAT);
            return triangle_flat;
        }

        void setQuadrilateral_Flat(const Quadrilateral_Flat &q)
        {
            assert(type == UNKNOWN_TYPE);
            type = QUADRILATERAL_FLAT;
            quadrilateral_flat = q;
        }

        __host__ __device__ const Quadrilateral_Flat &getQuadrilateral_Flat() const
        {
            assert(type == QUADRILATERAL_FLAT);
            return quadrilateral_flat;
        }

        void setCircle_Flat(const Circle_Flat &c)
        {
            assert(type == UNKNOWN_TYPE);
            type = CIRCLE_FLAT;
            circle_flat = c;
        }

        __host__ __device__ const Circle_Flat &getCircle_Flat() const
        {
            assert(type == CIRCLE_FLAT);
            return circle_flat;
        }

        void setHexagon_Flat(const Hexagon_Flat &h)
        {
            assert(type == UNKNOWN_TYPE);
            type = HEXAGON_FLAT;
            hexagon_flat = h;
        }

        __host__ __device__ const Hexagon_Flat &getHexagon_Flat() const
        {
            assert(type == HEXAGON_FLAT);
            return hexagon_flat;
        }

        void setAnnulus_Flat(const Annulus_Flat &anf)
        {
            assert(type == UNKNOWN_TYPE);
            type = ANNULUS_FLAT;
            annulus_flat = anf;
        }

        __host__ __device__ const Annulus_Flat &getAnnulus_Flat() const
        {
            assert(type == ANNULUS_FLAT);
            return annulus_flat;
        }

        Type type = UNKNOWN_TYPE;

        int32_t id = OptixCSP::kElementIdUnassigned;

    private:
        union
        {
            Parallelogram parallelogram;
            Cylinder_Y cylinder_y;
            Rectangle_Parabolic rectangle_parabolic;
            Rectangle_Flat rectangle_flat;
            Triangle_Flat triangle_flat;
            Quadrilateral_Flat quadrilateral_flat;
            Circle_Flat circle_flat;
            Hexagon_Flat hexagon_flat;
            Annulus_Flat annulus_flat;
        };
    };
}
