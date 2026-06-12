#include "bbox_calculator.hpp"

#include <algorithm>

#include <matvec.hpp>
#include <native_runner_types.hpp>

#include <glm/gtc/type_ptr.hpp>

namespace SolTrace::EmbreeRunner
{
    using SolTrace::NativeRunner::TElement;
    using SolTrace::NativeRunner::telement_ptr;
    using SolTrace::NativeRunner::TStage;
    using SolTrace::NativeRunner::tstage_ptr;

    // float get_absolute_minmax(const float values[], int size, bool is_max)
    // {
    //     float minmax_abs = std::abs(values[0]);

    //     if (is_max)
    //     {
    //         for (int i = 1; i < size; i++)
    //         {
    //             minmax_abs = std::max(std::abs(values[i]),
    //                                   minmax_abs);
    //         }
    //     }
    //     else
    //     {
    //         for (int i = 1; i < size; i++)
    //         {
    //             minmax_abs = std::min(std::abs(values[i]),
    //                                   minmax_abs);
    //         }
    //     }

    //     return minmax_abs;
    // }

    // void process_zernike_bounds(TElement* st_element, float x_minmax[2], float y_minmax[2],
    // 	float(&z_minmax)[2])
    // {
    // 	double z, x, y;

    // 	float& z_min = z_minmax[0];
    // 	float& z_max = z_minmax[1];

    // 	z_min = std::numeric_limits<double>::infinity();
    // 	z_max = -std::numeric_limits<double>::infinity();

    // 	float x_range[3] = { x_minmax[0], x_minmax[1], 0.f };
    // 	float y_range[3] = { y_minmax[0], y_minmax[1], 0.f };

    // 	for (int xi = 0; xi < 3; ++xi)
    // 	{
    // 		for (int yi = 0; yi < 3; ++yi)
    // 		{
    // 			x = x_range[xi];
    // 			y = y_range[yi];
    // 			EvalMono(x, y, st_element->BCoefficients, st_element->FitOrder, 0.0, 0.0, &z);

    // 			if (z < z_min) z_min = z;
    // 			if (z > z_max) z_max = z;
    // 		}
    // 	}
    // }

    // void process_poly_bounds(TElement* st_element, float x_minmax[2], float y_minmax[2],
    // 	float(&z_minmax)[2])
    // {
    // 	double z, x, y;

    // 	float& z_min = z_minmax[0];
    // 	float& z_max = z_minmax[1];

    // 	z_min = std::numeric_limits<double>::infinity();
    // 	z_max = -std::numeric_limits<double>::infinity();

    // 	float x_range[3] = { x_minmax[0], x_minmax[1], 0.f };
    // 	float y_range[3] = { y_minmax[0], y_minmax[1], 0.f };

    // 	for (int xi = 0; xi < 3; ++xi)
    // 	{
    // 		for (int yi = 0; yi < 3; ++yi)
    // 		{
    // 			x = x_range[xi];
    // 			y = y_range[yi];
    // 			EvalPoly(x, y, st_element->PolyCoeffs, st_element->FitOrder, &z);

    // 			if (z < z_min) z_min = z;
    // 			if (z > z_max) z_max = z;
    // 		}
    // 	}
    // }

    // bool find_spline_extrema(std::vector<double>& xa,
    // 	std::vector<double>& ya,
    // 	std::vector<double>& y2a,
    // 	double xMin, double xMax,
    // 	double& yMin, double& yMax)
    // {
    // 	if (xa.size() < 2 || xa.size() != ya.size() || xa.size() != y2a.size())
    // 		return false;

    // 	int n = xa.size();
    // 	yMin = INFINITY;
    // 	yMax = -INFINITY;

    // 	// Check all interval critical points
    // 	for (int i = 0; i < n - 1; i++) {
    // 		double h = xa[i + 1] - xa[i];
    // 		if (h == 0) continue;

    // 		// The spline derivative equal to zero gives us a quadratic equation
    // 		// Coefficients derived from the derivative expression in splint function
    // 		double A = h * (y2a[i + 1] - y2a[i]) / 2.0;
    // 		double B = h * y2a[i] / 2.0 - (ya[i + 1] - ya[i]) / h;
    // 		double C = -h * y2a[i] / 6.0;

    // 		// Solve quadratic equation: A*t² + B*t + C = 0 where t = (x-xa[i])/h
    // 		double discriminant = B * B - 4 * A * C;

    // 		if (std::abs(A) < 1e-10) {
    // 			// Linear case
    // 			if (std::abs(B) > 1e-10) {
    // 				double t = -C / B;
    // 				double x = xa[i] + t * h;
    // 				if (x >= xMin && x <= xMax && x >= xa[i] && x <= xa[i + 1]) {
    // 					double y, dydx;
    // 					if (splint(xa, ya, y2a, n, x, &y, &dydx)) {
    // 						yMin = std::min(yMin, y);
    // 						yMax = std::max(yMax, y);
    // 					}
    // 				}
    // 			}
    // 		}
    // 		else if (discriminant >= 0) {
    // 			// Two possible roots
    // 			double t1 = (-B + sqrt(discriminant)) / (2 * A);
    // 			double t2 = (-B - sqrt(discriminant)) / (2 * A);

    // 			double x1 = xa[i] + t1 * h;
    // 			double x2 = xa[i] + t2 * h;

    // 			// Check if critical points are in this interval and the overall range
    // 			if (x1 >= xa[i] && x1 <= xa[i + 1] && x1 >= xMin && x1 <= xMax) {
    // 				double y, dydx;
    // 				if (splint(xa, ya, y2a, n, x1, &y, &dydx)) {
    // 					yMin = std::min(yMin, y);
    // 					yMax = std::max(yMax, y);
    // 				}
    // 			}

    // 			if (x2 >= xa[i] && x2 <= xa[i + 1] && x2 >= xMin && x2 <= xMax) {
    // 				double y, dydx;
    // 				if (splint(xa, ya, y2a, n, x2, &y, &dydx)) {
    // 					yMin = std::min(yMin, y);
    // 					yMax = std::max(yMax, y);
    // 				}
    // 			}
    // 		}
    // 	}

    // 	// Also check endpoints and knot points within range
    // 	for (int i = 0; i < n; i++) {
    // 		if (xa[i] >= xMin && xa[i] <= xMax) {
    // 			double y, dydx;
    // 			if (splint(xa, ya, y2a, n, xa[i], &y, &dydx)) {
    // 				yMin = std::min(yMin, y);
    // 				yMax = std::max(yMax, y);
    // 			}
    // 		}
    // 	}

    // 	// Check range endpoints if they're not knot points
    // 	double y, dydx;
    // 	if (splint(xa, ya, y2a, n, xMin, &y, &dydx)) {
    // 		yMin = std::min(yMin, y);
    // 		yMax = std::max(yMax, y);
    // 	}

    // 	if (splint(xa, ya, y2a, n, xMax, &y, &dydx)) {
    // 		yMin = std::min(yMin, y);
    // 		yMax = std::max(yMax, y);
    // 	}

    // 	// Check x = 0;
    // 	if (splint(xa, ya, y2a, n, 0, &y, &dydx)) {
    // 		yMin = std::min(yMin, y);
    // 		yMax = std::max(yMax, y);
    // 	}

    // 	return yMin != INFINITY && yMax != -INFINITY;
    // }

    // void process_cubic_spline_bounds(TElement* st_element, float x_minmax[2], float y_minmax[2],
    // 	float(&z_minmax)[2])
    // {
    // 	double z, x, y;

    // 	float& z_min = z_minmax[0];
    // 	float& z_max = z_minmax[1];

    // 	z_min = std::numeric_limits<double>::infinity();
    // 	z_max = -std::numeric_limits<double>::infinity();

    // 	float x_min_abs = get_absolute_minmax(x_minmax, 2, false);
    // 	float x_max_abs = get_absolute_minmax(x_minmax, 2, true);
    // 	float y_min_abs = get_absolute_minmax(y_minmax, 2, false);
    // 	float y_max_abs = get_absolute_minmax(y_minmax, 2, true);

    // 	double Rho_min = sqrt(x_min_abs * x_min_abs + y_min_abs * y_min_abs);
    // 	double Rho_max = sqrt(x_max_abs * x_max_abs + y_max_abs * y_max_abs);
    // 	double z_min_test;
    // 	double z_max_test;

    // 	find_spline_extrema(st_element->CubicSplineXData,
    // 		st_element->CubicSplineYData,
    // 		st_element->CubicSplineY2Data,
    // 		Rho_min, Rho_max, z_min_test, z_max_test);

    // 	/*for (int xi = 0; xi < 3; ++xi)
    // 	{
    // 		for (int yi = 0; yi < 3; ++yi)
    // 		{
    // 			x = x_range[xi];
    // 			y = y_range[yi];
    // 			double Rho = sqrt(x * x + y * y);
    // 			double dummy;
    // 			splint(st_element->CubicSplineXData,
    // 				st_element->CubicSplineYData,
    // 				st_element->CubicSplineY2Data,
    // 				st_element->CubicSplineXData.size(),
    // 				Rho, &z, &dummy);

    // 			if (z < z_min) z_min = z;
    // 			if (z > z_max) z_max = z;
    // 		}
    // 	}*/

    // 	z_min = z_min_test;
    // 	z_max = z_max_test;
    // }

    // void process_FE_bounds(TElement* st_element, float x_minmax[2], float y_minmax[2],
    // 	float(&z_minmax)[2])
    // {
    // 	float& z_min = z_minmax[0];
    // 	float& z_max = z_minmax[1];

    // 	z_min = std::numeric_limits<double>::infinity();
    // 	z_max = -std::numeric_limits<double>::infinity();

    // 	const MatDoub& xyz_nodes = st_element->FEData.nodes;

    // 	for (const std::vector<double>& fe_node : xyz_nodes)
    // 	{
    // 		if (fe_node[2] > z_max)
    // 			z_max = fe_node[2];
    // 		if (fe_node[2] < z_min)
    // 			z_min = fe_node[2];
    // 	}
    // }

    void transform_to_global(glm::vec3 coord_element,
                             const tstage_ptr& st_stage,
                             const TElement *st_element,
                             glm::vec3& coord_global)
    {
        auto PosDumStage = st_element->RLocToRef * coord_element;
        auto coord_stage = PosDumStage + st_element->Origin;

        auto PosDumGlob = st_stage->RLocToRef * coord_stage;
        coord_global = PosDumGlob + st_stage->Origin;
        return;
    }

    void transform_bounds(glm::vec3 min_coord_element,
                          glm::vec3 max_coord_element,
                          const tstage_ptr& st_stage,
                          const TElement *st_element,
                          glm::vec3 &min_coord_global,
                          glm::vec3 &max_coord_global)
    {
        // Transform min and max bounding box from element coordinates to global
        glm::vec3 corners_element[8] =
            {
                {min_coord_element.x, min_coord_element.y, min_coord_element.z},
                {min_coord_element.x, min_coord_element.y, max_coord_element.z},
                {min_coord_element.x, max_coord_element.y, min_coord_element.z},
                {min_coord_element.x, max_coord_element.y, max_coord_element.z},
                {max_coord_element.x, min_coord_element.y, min_coord_element.z},
                {max_coord_element.x, min_coord_element.y, max_coord_element.z},
                {max_coord_element.x, max_coord_element.y, min_coord_element.z},
                {max_coord_element.x, max_coord_element.y, max_coord_element.z}};

        // Convert corners to global coordinates
        glm::vec3 corners_global[8];
        for (int i = 0; i < 8; i++)
            transform_to_global(corners_element[i], st_stage, st_element, corners_global[i]);

        // Find min and max xyz
        min_coord_global = corners_global[0];
        max_coord_global = corners_global[0];
        for (int i = 1; i < 8; i++)
        {
            min_coord_global = glm::min(corners_global[i], min_coord_global);
            max_coord_global = glm::max(corners_global[i], max_coord_global);
        }
    }

    bool get_bounds(const TElement *st_element,
                    glm::vec3 &min_coord_global,
                    glm::vec3 &max_coord_global)
    {
        // Get stage
        tstage_ptr const& st_stage = st_element->parent_stage;

        // Define element coord bounds
        glm::dvec2 x_minmax;
        glm::dvec2 y_minmax;
        glm::dvec2 z_minmax;

        st_element->aperture->bounding_box(x_minmax.x,
                                           x_minmax.y,
                                           y_minmax.x,
                                           y_minmax.y);

        st_element->surface->bounding_box(glm::value_ptr(x_minmax),
                                          glm::value_ptr(y_minmax),
                                          z_minmax.x,
                                          z_minmax.y);

        // Expand bounding boxes slightly to account for float precision
        glm::dvec2 expand = {-1e-3f, 1e-3f};
        x_minmax += expand;
        y_minmax += expand;
        z_minmax += expand;

        // Assign points to min/max coordinate element arrays
        glm::vec3 min_coord_element = {x_minmax.x, y_minmax.x, z_minmax.x};
        glm::vec3 max_coord_element = {x_minmax.y, y_minmax.y, z_minmax.y};

        // Convert local element bounds, to global xyz
        transform_bounds(min_coord_element, max_coord_element,
                         st_stage, st_element,
                         min_coord_global, max_coord_global);

        return true;
    }

} // namespace SolTrace::EmbreeRunner
