#include <gtest/gtest.h>

#include <optix_runner.hpp>
#include <simulation_data_export.hpp>
#include <simulation_result_export.hpp>
#include <simulation_runner.hpp>

using SolTrace::Runner::RunnerStatus;

const double Z_ELEM = 50.0;
const double Z_BACKSTOP = Z_ELEM - 0.5 * Z_ELEM;
const double TOL = 1e-6;
const uint_fast64_t NRAYS = 10000;

element_id set_default_sd(SimulationData &sd,
                          surface_ptr surf,
                          aperture_ptr ap)
{
    sd.clear();

    // Sun
    auto sun = make_ray_source<Sun>();
    sun->set_position(0, 0, 100);
    sd.add_ray_source(sun);

    // Make target element
    element_ptr el = make_element<SingleElement>();
    el->set_origin(0, 0, Z_ELEM);
    el->set_aim_vector(0, 0, 100); // Face up towards sun
    el->set_surface(surf);
    el->set_aperture(ap);

    OpticalPropertySet optics(InteractionType::REFLECTION, 0, 0);
    optics.set_ideal_absorption(OpticalSide::Both);
    auto opt_ref = sd.add_optical_property_set(optics);
    el->set_optical_property_set(opt_ref);

    el->set_name("el");

    // Add element to stage
    element_id id = sd.add_element(el);

    // Back stop element that is bigger than the created element so that the
    // testing element casts a shadow on this big thing.
    element_ptr stop = make_element<SingleElement>();
    double xlb, xub, ylb, yub;
    ap->bounding_box(xlb, xub, ylb, yub);
    const double sx = std::max(fabs(xlb), fabs(xub)) + 1.0;
    const double sy = std::max(fabs(ylb), fabs(yub)) + 1.0;
    stop->set_origin(0, 0, Z_BACKSTOP);
    stop->set_aim_vector(0, 0, 100);
    stop->set_surface(make_surface<Flat>());
    stop->set_aperture(make_aperture<Rectangle>(sx, sy));
    stop->set_optical_property_set(opt_ref);
    sd.add_element(stop);

    // Set parameters
    SimulationParameters &params = sd.get_simulation_parameters();
    params.number_of_rays = NRAYS;
    params.max_number_of_rays = params.number_of_rays * 100;
    params.include_optical_errors = false;
    params.include_sun_shape_errors = false;
    params.seed = 123;

    return id;
}

TEST(OptixRunner, FlatRectangle)
{
    const double XL = 10.0;
    const double YL = 5.0;
    auto surf = make_surface<Flat>();
    auto aper = make_aperture<Rectangle>(XL, YL);

    SimulationData sd;
    element_id test_elid = set_default_sd(sd, surf, aper);
    SimulationResult result;

    OptixRunner runner;
    RunnerStatus sts = runner.initialize();
    ASSERT_EQ(sts, RunnerStatus::SUCCESS);
    sts = runner.setup_simulation(&sd);
    ASSERT_EQ(sts, RunnerStatus::SUCCESS);
    sts = runner.run_simulation();
    ASSERT_EQ(sts, RunnerStatus::SUCCESS);
    sts = runner.report_simulation(&result, 0);
    ASSERT_EQ(sts, RunnerStatus::SUCCESS);

    ASSERT_EQ(result.get_number_of_records(),
              sd.get_simulation_parameters().number_of_rays);
    for (int i = 0; i < (int)result.get_number_of_records(); ++i)
    {
        auto rr = result[i];
        ASSERT_GE(rr->get_number_of_interactions(), 2);
        glm::dvec3 p0, p1;
        rr->get_position(0, p0);
        rr->get_position(1, p1);
        auto id = rr->get_element(1);
        EXPECT_NEAR(p0[0], p1[0], TOL) << "ray " << i;
        EXPECT_NEAR(p0[1], p1[1], TOL) << "ray " << i;

        if (id == test_elid)
        {
            // We hit the test element. Check that the height is as expected
            EXPECT_NEAR(p1[2], Z_ELEM, TOL * Z_ELEM) << "ray " << i;
            // And that we are in the aperture
            EXPECT_TRUE(aper->is_in(p1[0], p1[1]));
        }
        else
        {
            // We hit the back stop element. Check that the height is as expected
            EXPECT_NEAR(p1[2], Z_BACKSTOP, TOL * Z_ELEM);
            // And that we are not in the aperture.
            EXPECT_FALSE(aper->is_in(p1[0], p1[1]));
        }
    }
}

TEST(OptixRunner, FlatEquilateralTriangle)
{
    const double d = 4.0;
    auto surf = make_surface<Flat>();
    auto aper = make_aperture<EquilateralTriangle>(d);

    SimulationData sd;
    element_id test_elid = set_default_sd(sd, surf, aper);
    SimulationResult result;

    OptixRunner runner;
    RunnerStatus sts = runner.initialize();
    ASSERT_EQ(sts, RunnerStatus::SUCCESS);
    sts = runner.setup_simulation(&sd);
    ASSERT_EQ(sts, RunnerStatus::SUCCESS);
    sts = runner.run_simulation();
    ASSERT_EQ(sts, RunnerStatus::SUCCESS);
    sts = runner.report_simulation(&result, 0);
    ASSERT_EQ(sts, RunnerStatus::SUCCESS);

    ASSERT_EQ(result.get_number_of_records(),
              sd.get_simulation_parameters().number_of_rays);
    for (int i = 0; i < (int)result.get_number_of_records(); ++i)
    {
        auto rr = result[i];
        ASSERT_GE(rr->get_number_of_interactions(), 2);
        glm::dvec3 p0, p1;
        rr->get_position(0, p0);
        rr->get_position(1, p1);
        auto id = rr->get_element(1);
        EXPECT_NEAR(p0[0], p1[0], TOL) << "ray " << i;
        EXPECT_NEAR(p0[1], p1[1], TOL) << "ray " << i;

        if (id == test_elid)
        {
            EXPECT_NEAR(p1[2], Z_ELEM, TOL * Z_ELEM) << "ray " << i;
            EXPECT_TRUE(aper->is_in(p1[0], p1[1]));
        }
        else
        {
            EXPECT_NEAR(p1[2], Z_BACKSTOP, TOL * Z_ELEM);
            EXPECT_FALSE(aper->is_in(p1[0], p1[1]));
        }
    }
}

TEST(OptixRunner, FlatTriangle)
{
    const double x1 = 0.0, x2 = 1.0, x3 = 2.0 * x2;
    const double y1 = 0.0, y2 = 2.0, y3 = y1;
    auto surf = make_surface<Flat>();
    auto aper = make_aperture<IrregularTriangle>(x1, y1, x2, y2, x3, y3);

    SimulationData sd;
    element_id test_elid = set_default_sd(sd, surf, aper);
    SimulationResult result;

    OptixRunner runner;
    RunnerStatus sts = runner.initialize();
    ASSERT_EQ(sts, RunnerStatus::SUCCESS);
    sts = runner.setup_simulation(&sd);
    ASSERT_EQ(sts, RunnerStatus::SUCCESS);
    sts = runner.run_simulation();
    ASSERT_EQ(sts, RunnerStatus::SUCCESS);
    sts = runner.report_simulation(&result, 0);
    ASSERT_EQ(sts, RunnerStatus::SUCCESS);

    ASSERT_EQ(result.get_number_of_records(),
              sd.get_simulation_parameters().number_of_rays);
    for (int i = 0; i < (int)result.get_number_of_records(); ++i)
    {
        auto rr = result[i];
        ASSERT_GE(rr->get_number_of_interactions(), 2);
        glm::dvec3 p0, p1;
        rr->get_position(0, p0);
        rr->get_position(1, p1);
        auto id = rr->get_element(1);
        EXPECT_NEAR(p0[0], p1[0], TOL) << "ray " << i;
        EXPECT_NEAR(p0[1], p1[1], TOL) << "ray " << i;

        if (id == test_elid)
        {
            EXPECT_NEAR(p1[2], Z_ELEM, TOL * Z_ELEM) << "ray " << i;
            EXPECT_TRUE(aper->is_in(p1[0], p1[1]));
        }
        else
        {
            EXPECT_NEAR(p1[2], Z_BACKSTOP, TOL * Z_ELEM);
            EXPECT_FALSE(aper->is_in(p1[0], p1[1]));
        }
    }
}

TEST(OptixRunner, FlatQuadrilateral)
{
    // Parallelogram
    const double x1 = 0.0, x2 = 3.0, x3 = (x2 - x1) + 1.0, x4 = x3 - x2 + x1;
    const double y1 = 0.0, y2 = y1, y3 = 2.0, y4 = y3;
    auto surf = make_surface<Flat>();
    auto aper = make_aperture<IrregularQuadrilateral>(
        x1, y1, x2, y2, x3, y3, x4, y4);

    SimulationData sd;
    element_id test_elid = set_default_sd(sd, surf, aper);
    SimulationResult result;

    OptixRunner runner;
    RunnerStatus sts = runner.initialize();
    ASSERT_EQ(sts, RunnerStatus::SUCCESS);
    sts = runner.setup_simulation(&sd);
    ASSERT_EQ(sts, RunnerStatus::SUCCESS);
    sts = runner.run_simulation();
    ASSERT_EQ(sts, RunnerStatus::SUCCESS);
    sts = runner.report_simulation(&result, 0);
    ASSERT_EQ(sts, RunnerStatus::SUCCESS);

    ASSERT_EQ(result.get_number_of_records(),
              sd.get_simulation_parameters().number_of_rays);
    for (int i = 0; i < (int)result.get_number_of_records(); ++i)
    {
        auto rr = result[i];
        ASSERT_GE(rr->get_number_of_interactions(), 2);
        glm::dvec3 p0, p1;
        rr->get_position(0, p0);
        rr->get_position(1, p1);
        auto id = rr->get_element(1);
        EXPECT_NEAR(p0[0], p1[0], TOL) << "ray " << i;
        EXPECT_NEAR(p0[1], p1[1], TOL) << "ray " << i;

        if (id == test_elid)
        {
            EXPECT_NEAR(p1[2], Z_ELEM, TOL * Z_ELEM) << "ray " << i;
            EXPECT_TRUE(aper->is_in(p1[0], p1[1]));
        }
        else
        {
            EXPECT_NEAR(p1[2], Z_BACKSTOP, TOL * Z_ELEM);
            EXPECT_FALSE(aper->is_in(p1[0], p1[1]));
        }
    }
}

TEST(OptixRunner, ParabolaRectangle)
{
    constexpr double CX = 0.5;
    constexpr double CY = 1.0;
    constexpr double FX = 0.5 / CX;
    constexpr double FY = 0.5 / CY;
    const double XL = 10.0, YL = 5.0;
    auto surf = make_surface<Parabola>(FX, FY);
    auto aper = make_aperture<Rectangle>(XL, YL);

    SimulationData sd;
    element_id test_elid = set_default_sd(sd, surf, aper);
    SimulationResult result;

    OptixRunner runner;
    RunnerStatus sts = runner.initialize();
    ASSERT_EQ(sts, RunnerStatus::SUCCESS);
    sts = runner.setup_simulation(&sd);
    ASSERT_EQ(sts, RunnerStatus::SUCCESS);
    sts = runner.run_simulation();
    ASSERT_EQ(sts, RunnerStatus::SUCCESS);
    sts = runner.report_simulation(&result, 0);
    ASSERT_EQ(sts, RunnerStatus::SUCCESS);

    ASSERT_EQ(result.get_number_of_records(),
              sd.get_simulation_parameters().number_of_rays);
    for (int i = 0; i < (int)result.get_number_of_records(); ++i)
    {
        auto rr = result[i];
        ASSERT_GE(rr->get_number_of_interactions(), 2);
        glm::dvec3 p0, p1;
        rr->get_position(0, p0);
        rr->get_position(1, p1);
        auto id = rr->get_element(1);
        EXPECT_NEAR(p0[0], p1[0], TOL) << "ray " << i;
        EXPECT_NEAR(p0[1], p1[1], TOL) << "ray " << i;

        if (id == test_elid)
        {
            const double z1 = Z_ELEM + 0.5 * CX * p1[0] * p1[0] + 0.5 * CY * p1[1] * p1[1];
            EXPECT_NEAR(p1[2], z1, TOL * Z_ELEM) << "ray " << i;
            EXPECT_TRUE(aper->is_in(p1[0], p1[1]));
        }
        else
        {
            EXPECT_NEAR(p1[2], Z_BACKSTOP, TOL * Z_ELEM);
            EXPECT_FALSE(aper->is_in(p1[0], p1[1]));
        }
    }
}

TEST(OptixRunner, Cylinder)
{
    const double R = 5.0;
    const double YL = 3.0; // Total cylinder length
    auto surf = make_surface<Cylinder>(R);
    auto aper = make_aperture<Rectangle>(2 * R, YL);

    SimulationData sd;
    element_id test_elid = set_default_sd(sd, surf, aper);
    SimulationResult result;

    OptixRunner runner;
    RunnerStatus sts = runner.initialize();
    ASSERT_EQ(sts, RunnerStatus::SUCCESS);
    sts = runner.setup_simulation(&sd);
    ASSERT_EQ(sts, RunnerStatus::SUCCESS);
    sts = runner.run_simulation();
    ASSERT_EQ(sts, RunnerStatus::SUCCESS);
    sts = runner.report_simulation(&result, 0);
    ASSERT_EQ(sts, RunnerStatus::SUCCESS);

    ASSERT_EQ(result.get_number_of_records(),
              sd.get_simulation_parameters().number_of_rays);
    for (int i = 0; i < (int)result.get_number_of_records(); ++i)
    {
        auto rr = result[i];
        ASSERT_GE(rr->get_number_of_interactions(), 2);
        glm::dvec3 p0, p1;
        rr->get_position(0, p0);
        rr->get_position(1, p1);
        auto id = rr->get_element(1);
        EXPECT_NEAR(p0[0], p1[0], TOL) << "ray " << i;
        EXPECT_NEAR(p0[1], p1[1], TOL) << "ray " << i;

        if (id == test_elid)
        {
            const double z1 = Z_ELEM + sqrt(R * R - p1[0] * p1[0]);
            EXPECT_NEAR(p1[2], z1, TOL * Z_ELEM) << "ray " << i;
            EXPECT_TRUE(aper->is_in(p1[0], p1[1]));
        }
        else
        {
            EXPECT_NEAR(p1[2], Z_BACKSTOP, TOL * Z_ELEM);
            EXPECT_FALSE(aper->is_in(p1[0], p1[1]));
        }
    }
}

TEST(OptixRunner, FlatCircle)
{
    const double R = 5.0;
    auto surf = make_surface<Flat>();
    auto aper = make_aperture<Circle>(2 * R);

    SimulationData sd;
    element_id test_elid = set_default_sd(sd, surf, aper);
    SimulationResult result;

    OptixRunner runner;
    RunnerStatus sts = runner.initialize();
    ASSERT_EQ(sts, RunnerStatus::SUCCESS);
    sts = runner.setup_simulation(&sd);
    ASSERT_EQ(sts, RunnerStatus::SUCCESS);
    sts = runner.run_simulation();
    ASSERT_EQ(sts, RunnerStatus::SUCCESS);
    sts = runner.report_simulation(&result, 0);
    ASSERT_EQ(sts, RunnerStatus::SUCCESS);

    ASSERT_EQ(result.get_number_of_records(),
              sd.get_simulation_parameters().number_of_rays);
    for (int i = 0; i < (int)result.get_number_of_records(); ++i)
    {
        auto rr = result[i];
        ASSERT_GE(rr->get_number_of_interactions(), 2);
        glm::dvec3 p0, p1;
        rr->get_position(0, p0);
        rr->get_position(1, p1);
        auto id = rr->get_element(1);
        EXPECT_NEAR(p0[0], p1[0], TOL) << "ray " << i;
        EXPECT_NEAR(p0[1], p1[1], TOL) << "ray " << i;

        if (id == test_elid)
        {
            EXPECT_NEAR(p1[2], Z_ELEM, TOL * Z_ELEM) << "ray " << i;
            EXPECT_TRUE(aper->is_in(p1[0], p1[1]));
        }
        else
        {
            EXPECT_NEAR(p1[2], Z_BACKSTOP, TOL * Z_ELEM);
            EXPECT_FALSE(aper->is_in(p1[0], p1[1]));
        }
    }
}

TEST(OptixRunner, FlatHexagon)
{
    const double S = 5.0;
    auto surf = make_surface<Flat>();
    auto aper = make_aperture<Hexagon>(2 * S);

    SimulationData sd;
    element_id test_elid = set_default_sd(sd, surf, aper);
    SimulationResult result;

    OptixRunner runner;
    RunnerStatus sts = runner.initialize();
    ASSERT_EQ(sts, RunnerStatus::SUCCESS);
    sts = runner.setup_simulation(&sd);
    ASSERT_EQ(sts, RunnerStatus::SUCCESS);
    sts = runner.run_simulation();
    ASSERT_EQ(sts, RunnerStatus::SUCCESS);
    sts = runner.report_simulation(&result, 0);
    ASSERT_EQ(sts, RunnerStatus::SUCCESS);

    ASSERT_EQ(result.get_number_of_records(),
              sd.get_simulation_parameters().number_of_rays);
    for (int i = 0; i < (int)result.get_number_of_records(); ++i)
    {
        auto rr = result[i];
        ASSERT_GE(rr->get_number_of_interactions(), 2);
        glm::dvec3 p0, p1;
        rr->get_position(0, p0);
        rr->get_position(1, p1);
        auto id = rr->get_element(1);
        EXPECT_NEAR(p0[0], p1[0], TOL) << "ray " << i;
        EXPECT_NEAR(p0[1], p1[1], TOL) << "ray " << i;

        if (id == test_elid)
        {
            EXPECT_NEAR(p1[2], Z_ELEM, TOL * Z_ELEM) << "ray " << i;
            EXPECT_TRUE(aper->is_in(p1[0], p1[1]));
        }
        else
        {
            EXPECT_NEAR(p1[2], Z_BACKSTOP, TOL * Z_ELEM);
            EXPECT_FALSE(aper->is_in(p1[0], p1[1]));
        }
    }
}

TEST(OptixRunner, FlatAnnulus)
{
    const double R0 = 5.0;
    const double R1 = 10.0;
    const double ARC = 2 * PI;
    auto surf = make_surface<Flat>();
    auto aper = make_aperture<Annulus>(R0, R1, ARC);

    SimulationData sd;
    element_id test_elid = set_default_sd(sd, surf, aper);
    SimulationResult result;

    OptixRunner runner;
    RunnerStatus sts = runner.initialize();
    ASSERT_EQ(sts, RunnerStatus::SUCCESS);
    sts = runner.setup_simulation(&sd);
    ASSERT_EQ(sts, RunnerStatus::SUCCESS);
    sts = runner.run_simulation();
    ASSERT_EQ(sts, RunnerStatus::SUCCESS);
    sts = runner.report_simulation(&result, 0);
    ASSERT_EQ(sts, RunnerStatus::SUCCESS);

    ASSERT_EQ(result.get_number_of_records(),
              sd.get_simulation_parameters().number_of_rays);
    for (int i = 0; i < (int)result.get_number_of_records(); ++i)
    {
        auto rr = result[i];
        ASSERT_GE(rr->get_number_of_interactions(), 2);
        glm::dvec3 p0, p1;
        rr->get_position(0, p0);
        rr->get_position(1, p1);
        auto id = rr->get_element(1);
        EXPECT_NEAR(p0[0], p1[0], TOL) << "ray " << i;
        EXPECT_NEAR(p0[1], p1[1], TOL) << "ray " << i;

        if (id == test_elid)
        {
            EXPECT_NEAR(p1[2], Z_ELEM, TOL * Z_ELEM) << "ray " << i;
            EXPECT_TRUE(aper->is_in(p1[0], p1[1]));
        }
        else
        {
            EXPECT_NEAR(p1[2], Z_BACKSTOP, TOL * Z_ELEM);
            EXPECT_FALSE(aper->is_in(p1[0], p1[1]));
        }
    }
}
