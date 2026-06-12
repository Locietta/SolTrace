#include <gtest/gtest.h>

#include <iomanip>
#include <cmath>
#include <sstream>

#include "common.hpp"

#include <glm/gtx/io.hpp>

TEST(LinearAlgebra, VectorBasics)
{
    const double TOL = 1e-12;

    glm::dvec3 x(1.0, 1.0, 3.0);
    glm::dvec3 y(-1.0, 2.0, -1.0);
    glm::dvec3 z = glm::dvec3(0.0);

    EXPECT_EQ(z[0], 0.0);
    EXPECT_EQ(z[1], 0.0);
    EXPECT_EQ(z[2], 0.0);

    z = 1.0 * x + 1.0 * y;
    EXPECT_NEAR(z[0], 0.0, TOL);
    EXPECT_NEAR(z[1], 3.0, TOL);
    EXPECT_NEAR(z[2], 2.0, TOL);

    z = 0.5 * x + -2.0 * y;
    EXPECT_NEAR(z[0], 2.5, TOL);
    EXPECT_NEAR(z[1], -3.5, TOL);
    EXPECT_NEAR(z[2], 3.5, TOL);

    z = glm::max(x, y);
    EXPECT_EQ(z[0], 1.0);
    EXPECT_EQ(z[1], 2.0);
    EXPECT_EQ(z[2], 3.0);

    z = glm::min(x, y);
    EXPECT_EQ(z[0], -1.0);
    EXPECT_EQ(z[1], 1.0);
    EXPECT_EQ(z[2], -1.0);

    z = {1.0, 2.0, 3.0};
    z *= -0.5;
    EXPECT_NEAR(z[0], -0.5, TOL);
    EXPECT_NEAR(z[1], -1.0, TOL);
    EXPECT_NEAR(z[2], -1.5, TOL);

    double dp = glm::dot(x, y);
    EXPECT_NEAR(dp, -2.0, TOL);

    double mag = glm::length(x);
    EXPECT_NEAR(mag, sqrt(11.0), TOL);

    x = glm::normalize(x);
    EXPECT_NEAR(x[0], 1.0 / mag, TOL);
    EXPECT_NEAR(x[1], 1.0 / mag, TOL);
    EXPECT_NEAR(x[2], 3.0 / mag, TOL);

    mag = glm::length(y);
    EXPECT_NEAR(mag, sqrt(6.0), TOL);

    y = glm::normalize(y);
    EXPECT_NEAR(y[0], -1.0 / mag, TOL);
    EXPECT_NEAR(y[1], 2.0 / mag, TOL);
    EXPECT_NEAR(y[2], -1.0 / mag, TOL);

    z = {};
    EXPECT_EQ(z[0], 0.0);
    EXPECT_EQ(z[1], 0.0);
    EXPECT_EQ(z[2], 0.0);

    z[0] = 1.0;
    z[1] = 2.0;
    z[2] = 3.0;
    EXPECT_EQ(z[0], 1.0);
    EXPECT_EQ(z[1], 2.0);
    EXPECT_EQ(z[2], 3.0);

    glm::dvec3 u(z);
    EXPECT_EQ(z[0], u[0]);
    EXPECT_EQ(z[1], u[1]);
    EXPECT_EQ(z[2], u[2]);

    glm::dvec3 ihat(1.0, 0.0, 0.0);
    glm::dvec3 jhat(0.0, 1.0, 0.0);
    glm::dvec3 khat(0.0, 0.0, 1.0);
    glm::dvec3 result;
    result = glm::cross(ihat, jhat);
    EXPECT_NEAR(result[0], 0.0, TOL);
    EXPECT_NEAR(result[1], 0.0, TOL);
    EXPECT_NEAR(result[2], 1.0, TOL);
    result = glm::dvec3(0.0);

    result = glm::cross(ihat, khat);
    EXPECT_NEAR(result[0], 0.0, TOL);
    EXPECT_NEAR(result[1], -1.0, TOL);
    EXPECT_NEAR(result[2], 0.0, TOL);
    result = glm::dvec3(0.0);

    result = glm::cross(jhat, khat);
    EXPECT_NEAR(result[0], 1.0, TOL);
    EXPECT_NEAR(result[1], 0.0, TOL);
    EXPECT_NEAR(result[2], 0.0, TOL);
    result = glm::dvec3(0.0);

    double l2 = SolTrace::Data::error(ihat, jhat);
    EXPECT_NEAR(l2, sqrt(2.0), TOL);
    double linf = SolTrace::Data::error_inf(ihat, jhat);
    EXPECT_NEAR(linf, 1.0, TOL);
}

TEST(LinearAlgebra, MatrixVectorProduct)
{
    glm::dvec3 x(0.5, 1.0, -2.0);
    glm::dvec3 y;

    glm::dmat3 A = {0.0};
    A[0][0] = 1.0;
    A[1][1] = 1.0;
    A[2][2] = 1.0;

    y = A * x;
    EXPECT_TRUE(is_identical(x, y));

    A[1][0] = 2.0;
    A[2][1] = 3.0;
    y = A * x;
    EXPECT_NEAR(y[0], 2.5, 1e-12);
    EXPECT_NEAR(y[1], -5.0, 1e-12);
    EXPECT_NEAR(y[2], -2.0, 1e-12);
}

TEST(LinearAlgebra, MatrixMatrixProduct)
{
    glm::dmat3 A = {0.0};
    A[1][0] = 1.0;
    A[2][0] = 2.0;
    A[0][1] = -1.0;
    A[1][1] = 1.0;
    A[0][2] = 2.0;
    A[2][2] = 1.0;

    glm::dmat3 B = {0.0};
    B[0][0] = -1.0;
    B[1][0] = 1.0;
    B[1][1] = 1.0;
    B[2][1] = -2.0;
    B[2][2] = 1.0;

    glm::dmat3 Ctrue = {0.0};
    Ctrue[1][0] = 1.0;
    Ctrue[0][1] = 1.0;
    Ctrue[2][1] = -2.0;
    Ctrue[0][2] = -2.0;
    Ctrue[1][2] = 2.0;
    Ctrue[2][2] = 1.0;

    glm::dmat3 C{0.0};

    C = A * B;
    EXPECT_TRUE(is_identical(C, Ctrue));
}

TEST(LinearAlgebra, CoordinateTransforms)
{
    // TODO: Implement tests for compute_transform_matrices,
    // transform_to_local, and transform_to_reference functions
    glm::dmat3 A = {0.0};
    glm::dmat3 B = {0.0};

    EXPECT_TRUE(is_identical(A, B));
}

TEST(LinearAlgebra, dvec3OutputOperator)
{
    glm::dvec3 v1(1.0, 2.0, 3.0);
    glm::dvec3 v2(-0.5, 0.0, 10.5);
    glm::dvec3 v3(0.0, 0.0, 0.0);

    std::ostringstream oss1;
    oss1 << v1;
    EXPECT_EQ(oss1.str(), "[    1.000,    2.000,    3.000]");

    std::ostringstream oss2;
    oss2 << v2;
    EXPECT_EQ(oss2.str(), "[   -0.500,    0.000,   10.500]");

    std::ostringstream oss3;
    oss3 << v3;
    EXPECT_EQ(oss3.str(), "[    0.000,    0.000,    0.000]");

    // Test with different precision
    glm::dvec3 v4(1.23456789, -2.34567891, 3.45678912);
    std::ostringstream oss4;
    oss4 << std::fixed << std::setprecision(3) << v4;
    EXPECT_EQ(oss4.str(), "[    1.235,   -2.346,    3.457]");
}

TEST(LinearAlgebra, dmat3OutputOperator)
{
    glm::dmat3 A;
    A[0][0] = 1.0;
    A[1][0] = 2.0;
    A[2][0] = 3.0;
    A[0][1] = 4.0;
    A[1][1] = 5.0;
    A[2][1] = 6.0;
    A[0][2] = 7.0;
    A[1][2] = 8.0;
    A[2][2] = 9.0;

    std::ostringstream oss;
    oss << A;
    EXPECT_EQ(oss.str(),
              "\n[[    1.000,    2.000,    3.000]\n [    4.000,    5.000,    6.000]\n [    7.000,  "
              "  8.000,    9.000]]");

    // Test identity matrix
    glm::dmat3 I = glm::identity<glm::dmat3>();
    std::ostringstream oss_identity;
    oss_identity << I;
    EXPECT_EQ(oss_identity.str(),
              "\n[[    1.000,    0.000,    0.000]\n [    0.000,    1.000,    0.000]\n [    0.000,  "
              "  0.000,    1.000]]");

    // Test zero matrix
    glm::dmat3 Z(0.0);
    std::ostringstream oss_zero;
    oss_zero << Z;
    EXPECT_EQ(oss_zero.str(),
              "\n[[    0.000,    0.000,    0.000]\n [    0.000,    0.000,    0.000]\n [    0.000,  "
              "  0.000,    0.000]]");

    // Test with negative values and different precision
    glm::dmat3 B;
    B[0][0] = -1.5;
    B[1][0] = 2.25;
    B[2][0] = -3.75;
    B[0][1] = 0.0;
    B[1][1] = -0.5;
    B[2][1] = 1.0;
    B[0][2] = 10.0;
    B[1][2] = -20.0;
    B[2][2] = 30.0;

    std::ostringstream oss_negative;
    oss_negative << std::fixed << std::setprecision(2) << B;
    EXPECT_EQ(oss_negative.str(),
              "\n[[   -1.500,    2.250,   -3.750]\n [    0.000,   -0.500,    1.000]\n [   10.000,  "
              "-20.000,   30.000]]");
}

TEST(LinearAlgebra, dmat3GetValue)
{
    glm::dmat3 A(0.0);

    // Test getting values from zero matrix
    for (int i = 0; i < 3; ++i)
    {
        for (int j = 0; j < 3; ++j)
        {
            EXPECT_EQ(A[i][j], 0.0);
        }
    }

    // Set specific values and test retrieval
    A[0][0] = 1.5;
    A[1][0] = -2.5;
    A[2][0] = 3.7;
    A[0][1] = -4.2;
    A[1][1] = 5.8;
    A[2][1] = -6.1;
    A[0][2] = 7.9;
    A[1][2] = -8.4;
    A[2][2] = 9.6;

    EXPECT_DOUBLE_EQ(A[0][0], 1.5);
    EXPECT_DOUBLE_EQ(A[1][0], -2.5);
    EXPECT_DOUBLE_EQ(A[2][0], 3.7);
    EXPECT_DOUBLE_EQ(A[0][1], -4.2);
    EXPECT_DOUBLE_EQ(A[1][1], 5.8);
    EXPECT_DOUBLE_EQ(A[2][1], -6.1);
    EXPECT_DOUBLE_EQ(A[0][2], 7.9);
    EXPECT_DOUBLE_EQ(A[1][2], -8.4);
    EXPECT_DOUBLE_EQ(A[2][2], 9.6);

    // Test identity matrix
    glm::dmat3 I = glm::identity<glm::dmat3>();
    EXPECT_DOUBLE_EQ(I[0][0], 1.0);
    EXPECT_DOUBLE_EQ(I[1][0], 0.0);
    EXPECT_DOUBLE_EQ(I[2][0], 0.0);
    EXPECT_DOUBLE_EQ(I[0][1], 0.0);
    EXPECT_DOUBLE_EQ(I[1][1], 1.0);
    EXPECT_DOUBLE_EQ(I[2][1], 0.0);
    EXPECT_DOUBLE_EQ(I[0][2], 0.0);
    EXPECT_DOUBLE_EQ(I[1][2], 0.0);
    EXPECT_DOUBLE_EQ(I[2][2], 1.0);

    // Test very small and very large values
    A[0][0] = 1e-15;
    A[1][1] = 1e15;
    A[2][2] = -1e-10;

    EXPECT_DOUBLE_EQ(A[0][0], 1e-15);
    EXPECT_DOUBLE_EQ(A[1][1], 1e15);
    EXPECT_DOUBLE_EQ(A[2][2], -1e-10);
}
