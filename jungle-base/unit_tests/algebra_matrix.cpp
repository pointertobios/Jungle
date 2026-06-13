// Copyright (C) 2026 pointer-to-bios <pointer-to-bios@outlook.com>
// SPDX-License-Identifier: MIT

#include <array>

#include "jungle/algebra/matrix.h"
#include "jungle/test/test.h"

using namespace jungle::algebra;

JUNGLE_SYNC_TEST(matrix_construction_from_array) {
    matrix<float, 2, 2> m({{
            {1.0f, 2.0f},
            {3.0f, 4.0f},
        }});

    JUNGLE_SYNC_ASSERT((m[0, 0]) == 1.0f, "element (0,0) should match source array");
    JUNGLE_SYNC_ASSERT((m[0, 1]) == 2.0f, "element (0,1) should match source array");
    JUNGLE_SYNC_ASSERT((m[1, 0]) == 3.0f, "element (1,0) should match source array");
    JUNGLE_SYNC_ASSERT((m[1, 1]) == 4.0f, "element (1,1) should match source array");
    JUNGLE_SYNC_SUCCESS();
}

JUNGLE_SYNC_TEST(matrix_element_access_read_and_write) {
    matrix<float, 3, 2> m;

    (m[0, 0]) = 1.0f;
    (m[0, 1]) = 2.0f;
    (m[1, 0]) = 3.0f;
    (m[1, 1]) = 4.0f;
    (m[2, 0]) = 5.0f;
    (m[2, 1]) = 6.0f;

    JUNGLE_SYNC_ASSERT((m[0, 0]) == 1.0f, "written value should be readable at (0,0)");
    JUNGLE_SYNC_ASSERT((m[0, 1]) == 2.0f, "written value should be readable at (0,1)");
    JUNGLE_SYNC_ASSERT((m[1, 0]) == 3.0f, "written value should be readable at (1,0)");
    JUNGLE_SYNC_ASSERT((m[1, 1]) == 4.0f, "written value should be readable at (1,1)");
    JUNGLE_SYNC_ASSERT((m[2, 0]) == 5.0f, "written value should be readable at (2,0)");
    JUNGLE_SYNC_ASSERT((m[2, 1]) == 6.0f, "written value should be readable at (2,1)");
    JUNGLE_SYNC_SUCCESS();
}

JUNGLE_SYNC_TEST(matrix_zero_factory) {
    constexpr auto m = matrix<int, 3, 3>::zero();

    JUNGLE_SYNC_ASSERT((m[0, 0]) == 0, "zero matrix element should be 0 at (0,0)");
    JUNGLE_SYNC_ASSERT((m[0, 1]) == 0, "zero matrix element should be 0 at (0,1)");
    JUNGLE_SYNC_ASSERT((m[0, 2]) == 0, "zero matrix element should be 0 at (0,2)");
    JUNGLE_SYNC_ASSERT((m[1, 0]) == 0, "zero matrix element should be 0 at (1,0)");
    JUNGLE_SYNC_ASSERT((m[1, 1]) == 0, "zero matrix element should be 0 at (1,1)");
    JUNGLE_SYNC_ASSERT((m[1, 2]) == 0, "zero matrix element should be 0 at (1,2)");
    JUNGLE_SYNC_ASSERT((m[2, 0]) == 0, "zero matrix element should be 0 at (2,0)");
    JUNGLE_SYNC_ASSERT((m[2, 1]) == 0, "zero matrix element should be 0 at (2,1)");
    JUNGLE_SYNC_ASSERT((m[2, 2]) == 0, "zero matrix element should be 0 at (2,2)");
    JUNGLE_SYNC_SUCCESS();
}

JUNGLE_SYNC_TEST(matrix_add) {
    matrix<float, 2, 2> a({{
            {1.0f, 2.0f},
            {3.0f, 4.0f},
        }});
    matrix<float, 2, 2> b({{
            {5.0f, 6.0f},
            {7.0f, 8.0f},
        }});

    auto r = a.add(b);

    JUNGLE_SYNC_ASSERT((r[0, 0]) == 6.0f, "1+5 should be 6 at (0,0)");
    JUNGLE_SYNC_ASSERT((r[0, 1]) == 8.0f, "2+6 should be 8 at (0,1)");
    JUNGLE_SYNC_ASSERT((r[1, 0]) == 10.0f, "3+7 should be 10 at (1,0)");
    JUNGLE_SYNC_ASSERT((r[1, 1]) == 12.0f, "4+8 should be 12 at (1,1)");
    JUNGLE_SYNC_SUCCESS();
}

JUNGLE_SYNC_TEST(matrix_subtract) {
    matrix<float, 2, 2> a({{
            {5.0f, 7.0f},
            {9.0f, 11.0f},
        }});
    matrix<float, 2, 2> b({{
            {1.0f, 2.0f},
            {3.0f, 4.0f},
        }});

    auto r = a.subtract(b);

    JUNGLE_SYNC_ASSERT((r[0, 0]) == 4.0f, "5-1 should be 4 at (0,0)");
    JUNGLE_SYNC_ASSERT((r[0, 1]) == 5.0f, "7-2 should be 5 at (0,1)");
    JUNGLE_SYNC_ASSERT((r[1, 0]) == 6.0f, "9-3 should be 6 at (1,0)");
    JUNGLE_SYNC_ASSERT((r[1, 1]) == 7.0f, "11-4 should be 7 at (1,1)");
    JUNGLE_SYNC_SUCCESS();
}

JUNGLE_SYNC_TEST(matrix_multiply_scalar) {
    matrix<float, 2, 3> m({{
            {1.0f, 2.0f, 3.0f},
            {4.0f, 5.0f, 6.0f},
        }});

    auto r = m.multiply(2.0f);

    JUNGLE_SYNC_ASSERT((r[0, 0]) == 2.0f, "1*2 should be 2 at (0,0)");
    JUNGLE_SYNC_ASSERT((r[0, 1]) == 4.0f, "2*2 should be 4 at (0,1)");
    JUNGLE_SYNC_ASSERT((r[0, 2]) == 6.0f, "3*2 should be 6 at (0,2)");
    JUNGLE_SYNC_ASSERT((r[1, 0]) == 8.0f, "4*2 should be 8 at (1,0)");
    JUNGLE_SYNC_ASSERT((r[1, 1]) == 10.0f, "5*2 should be 10 at (1,1)");
    JUNGLE_SYNC_ASSERT((r[1, 2]) == 12.0f, "6*2 should be 12 at (1,2)");
    JUNGLE_SYNC_SUCCESS();
}

JUNGLE_SYNC_TEST(matrix_multiply_scalar_integer_type) {
    matrix<int, 2, 2> m({{
            {1, 2},
            {3, 4},
        }});

    auto r = m.multiply(3);

    JUNGLE_SYNC_ASSERT((r[0, 0]) == 3, "1*3 should be 3 at (0,0)");
    JUNGLE_SYNC_ASSERT((r[0, 1]) == 6, "2*3 should be 6 at (0,1)");
    JUNGLE_SYNC_ASSERT((r[1, 0]) == 9, "3*3 should be 9 at (1,0)");
    JUNGLE_SYNC_ASSERT((r[1, 1]) == 12, "4*3 should be 12 at (1,1)");
    JUNGLE_SYNC_SUCCESS();
}

JUNGLE_SYNC_TEST(matrix_multiply_matrix) {
    matrix<float, 2, 3> a({{
            {1.0f, 2.0f, 3.0f},
            {4.0f, 5.0f, 6.0f},
        }});
    matrix<float, 3, 2> b({{
            {7.0f, 8.0f},
            {9.0f, 10.0f},
            {11.0f, 12.0f},
        }});

    auto r = a.multiply(b);

    JUNGLE_SYNC_ASSERT((r[0, 0]) == 58.0f, "1*7+2*9+3*11 should be 58 at (0,0)");
    JUNGLE_SYNC_ASSERT((r[0, 1]) == 64.0f, "1*8+2*10+3*12 should be 64 at (0,1)");
    JUNGLE_SYNC_ASSERT((r[1, 0]) == 139.0f, "4*7+5*9+6*11 should be 139 at (1,0)");
    JUNGLE_SYNC_ASSERT((r[1, 1]) == 154.0f, "4*8+5*10+6*12 should be 154 at (1,1)");
    JUNGLE_SYNC_SUCCESS();
}

JUNGLE_SYNC_TEST(matrix_multiply_matrix_square) {
    matrix<float, 2, 2> a({{
            {1.0f, 2.0f},
            {3.0f, 4.0f},
        }});
    matrix<float, 2, 2> b({{
            {2.0f, 0.0f},
            {1.0f, 2.0f},
        }});

    auto r = a.multiply(b);

    JUNGLE_SYNC_ASSERT((r[0, 0]) == 4.0f, "1*2+2*1 should be 4 at (0,0)");
    JUNGLE_SYNC_ASSERT((r[0, 1]) == 4.0f, "1*0+2*2 should be 4 at (0,1)");
    JUNGLE_SYNC_ASSERT((r[1, 0]) == 10.0f, "3*2+4*1 should be 10 at (1,0)");
    JUNGLE_SYNC_ASSERT((r[1, 1]) == 8.0f, "3*0+4*2 should be 8 at (1,1)");
    JUNGLE_SYNC_SUCCESS();
}

JUNGLE_SYNC_TEST(matrix_dot_product) {
    matrix<float, 2, 2> a({{
        {1.0f, 2.0f},
        {3.0f, 4.0f},
    }});
    matrix<float, 2, 2> b({{
        {5.0f, 6.0f},
        {7.0f, 8.0f},
    }});

    auto d = a.dot(b);

    JUNGLE_SYNC_ASSERT(d == 70.0f, "1*5+2*6+3*7+4*8 should be 70");
    JUNGLE_SYNC_SUCCESS();
}

JUNGLE_SYNC_TEST(matrix_dot_product_int) {
    matrix<int, 2, 2> a({{
        {1, 2},
        {3, 4},
    }});
    matrix<int, 2, 2> b({{
        {1, 1},
        {1, 1},
    }});

    auto d = a.dot(b);

    JUNGLE_SYNC_ASSERT(d == 10, "1+2+3+4 should be 10");
    JUNGLE_SYNC_SUCCESS();
}

JUNGLE_SYNC_TEST(matrix_norm) {
    matrix<float, 2, 2> m({{
        {1.0f, 2.0f},
        {3.0f, 4.0f},
    }});

    auto n = m.norm();

    JUNGLE_SYNC_ASSERT(n > 5.47f && n < 5.48f, "sqrt(1+4+9+16)=sqrt(30)≈5.477");
    JUNGLE_SYNC_SUCCESS();
}

JUNGLE_SYNC_TEST(matrix_transpose_rectangular) {
    matrix<float, 2, 3> m({{
        {1.0f, 2.0f, 3.0f},
        {4.0f, 5.0f, 6.0f},
    }});

    auto t = m.transpose();

    JUNGLE_SYNC_ASSERT((t[0, 0]) == 1.0f, "transpose (0,0) should be original (0,0)");
    JUNGLE_SYNC_ASSERT((t[0, 1]) == 4.0f, "transpose (0,1) should be original (1,0)");
    JUNGLE_SYNC_ASSERT((t[1, 0]) == 2.0f, "transpose (1,0) should be original (0,1)");
    JUNGLE_SYNC_ASSERT((t[1, 1]) == 5.0f, "transpose (1,1) should be original (1,1)");
    JUNGLE_SYNC_ASSERT((t[2, 0]) == 3.0f, "transpose (2,0) should be original (0,2)");
    JUNGLE_SYNC_ASSERT((t[2, 1]) == 6.0f, "transpose (2,1) should be original (1,2)");
    JUNGLE_SYNC_SUCCESS();
}

JUNGLE_SYNC_TEST(matrix_transpose_square) {
    matrix<float, 2, 2> m({{
        {1.0f, 2.0f},
        {3.0f, 4.0f},
    }});

    auto t = m.transpose();

    JUNGLE_SYNC_ASSERT((t[0, 0]) == 1.0f, "transpose (0,0) should be 1");
    JUNGLE_SYNC_ASSERT((t[0, 1]) == 3.0f, "transpose (0,1) should be 3");
    JUNGLE_SYNC_ASSERT((t[1, 0]) == 2.0f, "transpose (1,0) should be 2");
    JUNGLE_SYNC_ASSERT((t[1, 1]) == 4.0f, "transpose (1,1) should be 4");
    JUNGLE_SYNC_SUCCESS();
}

JUNGLE_SYNC_TEST(matrix_kronecker_multiply) {
    matrix<float, 2, 2> a({{
        {1.0f, 2.0f},
        {3.0f, 4.0f},
    }});
    matrix<float, 2, 3> b({{
        {0.0f, 5.0f, 0.0f},
        {6.0f, 7.0f, 0.0f},
    }});

    auto k = a.kronecker_multiply(b);

    JUNGLE_SYNC_ASSERT((k[0, 0]) == 0.0f, "1*0 should be 0 at (0,0)");
    JUNGLE_SYNC_ASSERT((k[0, 1]) == 5.0f, "1*5 should be 5 at (0,1)");
    JUNGLE_SYNC_ASSERT((k[0, 2]) == 0.0f, "1*0 should be 0 at (0,2)");
    JUNGLE_SYNC_ASSERT((k[0, 3]) == 0.0f, "2*0 should be 0 at (0,3)");
    JUNGLE_SYNC_ASSERT((k[0, 4]) == 10.0f, "2*5 should be 10 at (0,4)");
    JUNGLE_SYNC_ASSERT((k[0, 5]) == 0.0f, "2*0 should be 0 at (0,5)");
    JUNGLE_SYNC_ASSERT((k[1, 0]) == 6.0f, "1*6 should be 6 at (1,0)");
    JUNGLE_SYNC_ASSERT((k[1, 1]) == 7.0f, "1*7 should be 7 at (1,1)");
    JUNGLE_SYNC_ASSERT((k[2, 0]) == 0.0f, "3*0 should be 0 at (2,0)");
    JUNGLE_SYNC_ASSERT((k[2, 1]) == 15.0f, "3*5 should be 15 at (2,1)");
    JUNGLE_SYNC_ASSERT((k[3, 0]) == 18.0f, "3*6 should be 18 at (3,0)");
    JUNGLE_SYNC_SUCCESS();
}

JUNGLE_SYNC_TEST(square_matrix_identity) {
    constexpr auto m = matrix<int, 3>::identity();

    JUNGLE_SYNC_ASSERT((m[0, 0]) == 1, "identity diagonal (0,0) should be 1");
    JUNGLE_SYNC_ASSERT((m[0, 1]) == 0, "identity off-diagonal (0,1) should be 0");
    JUNGLE_SYNC_ASSERT((m[0, 2]) == 0, "identity off-diagonal (0,2) should be 0");
    JUNGLE_SYNC_ASSERT((m[1, 0]) == 0, "identity off-diagonal (1,0) should be 0");
    JUNGLE_SYNC_ASSERT((m[1, 1]) == 1, "identity diagonal (1,1) should be 1");
    JUNGLE_SYNC_ASSERT((m[1, 2]) == 0, "identity off-diagonal (1,2) should be 0");
    JUNGLE_SYNC_ASSERT((m[2, 0]) == 0, "identity off-diagonal (2,0) should be 0");
    JUNGLE_SYNC_ASSERT((m[2, 1]) == 0, "identity off-diagonal (2,1) should be 0");
    JUNGLE_SYNC_ASSERT((m[2, 2]) == 1, "identity diagonal (2,2) should be 1");
    JUNGLE_SYNC_SUCCESS();
}

JUNGLE_SYNC_TEST(square_matrix_identity_multiply_yields_original) {
    matrix<float, 3, 3> m({{
        {2.0f, 3.0f, 5.0f},
        {7.0f, 11.0f, 13.0f},
        {17.0f, 19.0f, 23.0f},
    }});
    auto id = matrix<float, 3>::identity();

    auto r = m.multiply(id);

    JUNGLE_SYNC_ASSERT((r[0, 0]) == 2.0f, "M*I should preserve (0,0)");
    JUNGLE_SYNC_ASSERT((r[0, 1]) == 3.0f, "M*I should preserve (0,1)");
    JUNGLE_SYNC_ASSERT((r[0, 2]) == 5.0f, "M*I should preserve (0,2)");
    JUNGLE_SYNC_ASSERT((r[1, 0]) == 7.0f, "M*I should preserve (1,0)");
    JUNGLE_SYNC_ASSERT((r[1, 1]) == 11.0f, "M*I should preserve (1,1)");
    JUNGLE_SYNC_ASSERT((r[1, 2]) == 13.0f, "M*I should preserve (1,2)");
    JUNGLE_SYNC_ASSERT((r[2, 0]) == 17.0f, "M*I should preserve (2,0)");
    JUNGLE_SYNC_ASSERT((r[2, 1]) == 19.0f, "M*I should preserve (2,1)");
    JUNGLE_SYNC_ASSERT((r[2, 2]) == 23.0f, "M*I should preserve (2,2)");
    JUNGLE_SYNC_SUCCESS();
}

JUNGLE_SYNC_TEST(vector_construction_from_array) {
    vector<float, 3> v({1.0f, 2.0f, 3.0f});

    JUNGLE_SYNC_ASSERT(v[0] == 1.0f, "vector element 0 should match source array");
    JUNGLE_SYNC_ASSERT(v[1] == 2.0f, "vector element 1 should match source array");
    JUNGLE_SYNC_ASSERT(v[2] == 3.0f, "vector element 2 should match source array");
    JUNGLE_SYNC_SUCCESS();
}

JUNGLE_SYNC_TEST(vector_construction_from_matrix) {
    matrix<float, 4, 1> m(
        std::array<std::array<float, 1>, 4>{{
            {5.0f},
            {6.0f},
            {7.0f},
            {8.0f},
        }});
    vector<float, 4> v(m);

    JUNGLE_SYNC_ASSERT(v[0] == 5.0f, "vector from matrix should preserve element 0");
    JUNGLE_SYNC_ASSERT(v[1] == 6.0f, "vector from matrix should preserve element 1");
    JUNGLE_SYNC_ASSERT(v[2] == 7.0f, "vector from matrix should preserve element 2");
    JUNGLE_SYNC_ASSERT(v[3] == 8.0f, "vector from matrix should preserve element 3");
    JUNGLE_SYNC_SUCCESS();
}

JUNGLE_SYNC_TEST(vector_element_access) {
    vector<float, 3> v;

    v[0] = 10.0f;
    v[1] = 20.0f;
    v[2] = 30.0f;

    JUNGLE_SYNC_ASSERT(v[0] == 10.0f, "vector element 0 read-back should match written value");
    JUNGLE_SYNC_ASSERT(v[1] == 20.0f, "vector element 1 read-back should match written value");
    JUNGLE_SYNC_ASSERT(v[2] == 30.0f, "vector element 2 read-back should match written value");
    JUNGLE_SYNC_SUCCESS();
}

JUNGLE_SYNC_TEST(vector_normalize) {
    vector<float, 2> v({3.0f, 4.0f});

    auto n = v.normalize();

    JUNGLE_SYNC_ASSERT(n[0] > 0.59f && n[0] < 0.61f, "normalized x should be 3/5=0.6");
    JUNGLE_SYNC_ASSERT(n[1] > 0.79f && n[1] < 0.81f, "normalized y should be 4/5=0.8");
    JUNGLE_SYNC_SUCCESS();
}

JUNGLE_SYNC_TEST(vector_cross_product) {
    vector3f a({1.0f, 0.0f, 0.0f});
    vector3f b({0.0f, 1.0f, 0.0f});

    auto c = a.cross(b);

    JUNGLE_SYNC_ASSERT(c[0] == 0.0f, "i×j should have x=0");
    JUNGLE_SYNC_ASSERT(c[1] == 0.0f, "i×j should have y=0");
    JUNGLE_SYNC_ASSERT(c[2] == 1.0f, "i×j should have z=1");

    vector3f d({2.0f, 3.0f, 4.0f});
    vector3f e({5.0f, 6.0f, 7.0f});

    auto f = d.cross(e);

    JUNGLE_SYNC_ASSERT(f[0] == -3.0f, "3*7-4*6 should be -3");
    JUNGLE_SYNC_ASSERT(f[1] == 6.0f, "4*5-2*7 should be 6");
    JUNGLE_SYNC_ASSERT(f[2] == -3.0f, "2*6-3*5 should be -3");
    JUNGLE_SYNC_SUCCESS();
}

JUNGLE_SYNC_TEST(matrix_type_aliases_compile) {
    matrix3<float> m3;
    matrix4<float> m4;
    matrix3f m3f;
    matrix4f m4f;

    (void)m3;
    (void)m4;
    (void)m3f;
    (void)m4f;
    JUNGLE_SYNC_SUCCESS();
}

JUNGLE_SYNC_TEST(vector_type_aliases_compile) {
    vector2<float> v2;
    vector3<float> v3;
    vector2f v2f;
    vector3f v3f;

    (void)v2;
    (void)v3;
    (void)v2f;
    (void)v3f;
    JUNGLE_SYNC_SUCCESS();
}

JUNGLE_SYNC_TEST(matrix_add_subtract_chain) {
    matrix<float, 2, 2> a({{
            {10.0f, 20.0f},
            {30.0f, 40.0f},
        }});
    matrix<float, 2, 2> b({{
            {1.0f, 2.0f},
            {3.0f, 4.0f},
        }});
    matrix<float, 2, 2> c({{
            {5.0f, 6.0f},
            {7.0f, 8.0f},
        }});

    auto r = a.subtract(b).add(c);

    JUNGLE_SYNC_ASSERT((r[0, 0]) == 14.0f, "10-1+5 should be 14 at (0,0)");
    JUNGLE_SYNC_ASSERT((r[0, 1]) == 24.0f, "20-2+6 should be 24 at (0,1)");
    JUNGLE_SYNC_ASSERT((r[1, 0]) == 34.0f, "30-3+7 should be 34 at (1,0)");
    JUNGLE_SYNC_ASSERT((r[1, 1]) == 44.0f, "40-4+8 should be 44 at (1,1)");
    JUNGLE_SYNC_SUCCESS();
}


