#include <algorithm>
#include "Helper.h"
#include "ZNFramework.h"
using namespace ZNFramework;

TEST(Matrix, Equal)
{
    auto body = [](auto m)
    {
        using M = decltype(m);
        M a, b, c;
        RandomObjectsWithValuesForEqualTest<M>(a, b, c);
        EXPECT_EQ(true, a == b);
        EXPECT_EQ(false, a == c);
    };
    body(ZNMatrix2());
    body(ZNMatrix3());
    body(ZNMatrix4());
}

TEST(Matrix, NotEqual)
{
    auto body = [](auto m)
    {
        using M = decltype(m);
        M a, b, c;
        RandomObjectsWithValuesForEqualTest<M>(a, b, c);
        EXPECT_EQ(false, a != b);
        EXPECT_EQ(true, a != c);
    };
    body(ZNMatrix2());
    body(ZNMatrix3());
    body(ZNMatrix4());
}

TEST(Matrix, Add)
{
    auto body = [](auto m)
    {
        using M = decltype(m);
        M a = RandomObjectWithValues<M>();
        M b = RandomObjectWithValues<M>();
        M r = SumEachValueOfObject<M>(a, b);
        EXPECT_EQ(r, a + b);
        a += b;
        EXPECT_EQ(r, a);
    };
    body(ZNMatrix2());
    body(ZNMatrix3());
    body(ZNMatrix4());
}

TEST(Matrix, Sub)
{
    auto body = [](auto m)
    {
        using M = decltype(m);
        M a = RandomObjectWithValues<M>();
        M b = RandomObjectWithValues<M>();
        M r = SubEachValueOfObject<M>(a, b);
        EXPECT_EQ(r, a - b);
        a -= b;
        EXPECT_EQ(r, a);
    };
    body(ZNMatrix2());
    body(ZNMatrix3());
    body(ZNMatrix4());
}

TEST(Matrix, ScalarMultiply)
{
    auto body = [](auto m)
    {
        using M = decltype(m);
        M a = RandomObjectWithValues<M>();
        float s = RandomFloat();
        M r = MulEachValueOfObject<M>(a, s);
        EXPECT_EQ(r, a * s);
        a *= s;
        EXPECT_EQ(r, a);
    };
    body(ZNMatrix2());
    body(ZNMatrix3());
    body(ZNMatrix4());
}

TEST(Matrix, Multiply)
{
    auto body = [](auto m)
    {
        using M = decltype(m);
        M m1 = RandomObjectWithValues<M>();
        M m2 = RandomObjectWithValues<M>();
        int dimension = sizeof(m1.m[0]) / sizeof(m1.m[0][0]);
        M r;
        for (int i = 0; i < dimension; ++i)
        {
            for (int j = 0; j < dimension; ++j)
            {
                r.m[i][j] = 0.0f;
                for (int k = 0; k < dimension; ++k)
                {
                     r.m[i][j] += m1.m[i][k] * m2.m[k][j];
                }
                
            }
        }
        EXPECT_EQ(r, m1 * m2);
        m1 *= m2;
        EXPECT_EQ(r, m1);
    };
    body(ZNMatrix2());
    body(ZNMatrix3());
    body(ZNMatrix4());
}

TEST(Matrix, Transpose)
{
    auto body = [](auto m)
    {
        using M = decltype(m);
        m = RandomObjectWithValues<M>();
        M r = m;
        int dimension = sizeof(m.m[0]) / sizeof(m.m[0][0]);
        for (int i = 0; i < dimension; ++i)
        {
            for (int j = i; j < dimension; ++j)
            {
                std::swap(m.m[i][j], m.m[j][i]);
            }
        }

        EXPECT_EQ(r, m.Transpose());
    };
    body(ZNMatrix2());
    body(ZNMatrix3());
    body(ZNMatrix4());
}

TEST(Matrix, Inverse)
{
    auto body = [](auto m)
    {
        using M = decltype(m);
        m = RandomObjectWithValues<M>();
        auto r = m * m.Inverse();
        int size = sizeof(m.value) / sizeof(m.value[0]);
        auto identity = M();
        for (int i = 0; i < size; ++i)
        {
            using elementType = std::remove_reference<decltype(m.value[i])>::type;
            EXPECT_EQ(true, ApproximatelyEqualAbsRel<elementType>(identity.value[i], 
                                                                  r.value[i], 
                                                                  1e-5f,
                                                                  1e-4f));
        }
    };
    body(ZNMatrix2());
    body(ZNMatrix3());
    body(ZNMatrix4());
}

TEST(Matrix, Determinant)
{
    auto body = [](auto m)
    {
        // 어떤 행렬의 deteminant는 그 행렬의 전치행렬과 값이 같다.
        using M = decltype(m);
        m = RandomObjectWithValues<M>();
        M r = m.Transpose();
        EXPECT_EQ(true, ApproximatelyEqualAbsRel(r.Determinant(), 
                                                 m.Determinant(), 
                                                 1e-5f, 
                                                 1e-4f));
    };
    body(ZNMatrix2());
    body(ZNMatrix3());
    body(ZNMatrix4());
}