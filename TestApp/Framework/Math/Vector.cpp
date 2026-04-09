#include "Helper.h"
#include "ZNFramework.h"
using namespace ZNFramework;

TEST(Vector, Equal)
{
    auto body = [](auto v)
    {
        using V = decltype(v);
        V a, b, c;
        RandomObjectsWithValuesForEqualTest<V>(a, b, c);
        EXPECT_EQ(true, a == b);
        EXPECT_EQ(false, a == c);
    };
    body(ZNVector2());
    body(ZNVector3());
    body(ZNVector4());
}

TEST(Vector, NotEqual)
{
    auto body = [](auto v)
    {
        using V = decltype(v);
        V a, b, c;
        RandomObjectsWithValuesForEqualTest<V>(a, b, c);
        EXPECT_EQ(false, a != b);
        EXPECT_EQ(true, a != c);
    };
    body(ZNVector2());
    body(ZNVector3());
    body(ZNVector4());
}

TEST(Vector, Add)
{
    auto body = [](auto v)
    {
        using V = decltype(v);
        V a = RandomObjectWithValues<V>();
        V b = RandomObjectWithValues<V>();
        V r = SumEachValueOfObject<V>(a, b);
        EXPECT_EQ(r, a + b);
        a += b;
        EXPECT_EQ(r, a);
    };
    body(ZNVector2());
    body(ZNVector3());
    body(ZNVector4());
}

TEST(Vector, Sub)
{
    auto body = [](auto v)
    {
        using V = decltype(v);
        V a = RandomObjectWithValues<V>();
        V b = RandomObjectWithValues<V>();
        V r = SubEachValueOfObject<V>(a, b);
        EXPECT_EQ(r, a - b);
        a -= b;
        EXPECT_EQ(r, a);
    };
    body(ZNVector2());
    body(ZNVector3());
    body(ZNVector4());
}

TEST(Vector, ScalarMultiply)
{
    auto body = [](auto v)
    {
        using V = decltype(v);
        V a = RandomObjectWithValues<V>();
        float s = RandomFloat();
        V r = MulEachValueOfObject<V>(a, s);
        EXPECT_EQ(r, a * s);
        a *= s;
        EXPECT_EQ(r, a);
    };
    body(ZNVector2());
    body(ZNVector3());
    body(ZNVector4());
}

TEST(Vector, Dot)
{
    auto body = [](auto v)
    {
        using V = decltype(v);
        V a = RandomObjectWithValues<V>();
        V b = RandomObjectWithValues<V>();
        int dimension = sizeof(a.value) / sizeof(a.value[0]);
        float r = 0.0f;
        for (int i = 0; i < dimension; ++i)
        {
            r += a.value[i] * b.value[i];
        }
        EXPECT_EQ(r, V::Dot(a, b));
    };
    body(ZNVector2());
    body(ZNVector3());
    body(ZNVector4());
}

TEST(Vector, Cross)
{
    ZNVector3 v1{ 1.f, 0.f, 0.f };
    ZNVector3 v2{ 0.f, 1.f, 0.f };
    ZNVector3 answer{0.f, 0.f, 1.f};
    EXPECT_EQ(answer, ZNVector3::Cross(v1, v2));
}

TEST(Vector, MulMatrix)
{
    auto body = [](auto v, auto m)
    {
        using V = decltype(v);
        using M = decltype(m);
        v = RandomObjectWithValues<V>();
        m = RandomObjectWithValues<M>();
        int vDimension = sizeof(v.value) / sizeof(v.value[0]);
        int mDimension = sizeof(m.m[0]) / sizeof(m.m[0][0]);
        V r;
        for (int i = 0; i < vDimension; ++i)
        {
            for (int j = 0; j < mDimension; ++j)
            {
                r.value[i] += v.value[j] * m.m[j][i];
            }
        }
        EXPECT_EQ(r, v * m);
    };
    body(ZNVector2(), ZNMatrix2());
    body(ZNVector3(), ZNMatrix3());
    body(ZNVector4(), ZNMatrix4());
}

TEST(Vector, Length)
{
    auto body = [](auto v)
    {
        using V = decltype(v);
        v = RandomObjectWithValues<V>();
        int dimension = sizeof(v.value) / sizeof(v.value[0]);
        float r = 0.0f;
        for (int i = 0; i < dimension; ++i)
        {
            r += v.value[i] * v.value[i];
        }
        EXPECT_EQ(sqrtf(r), v.Length());
    };
    body(ZNVector2());
    body(ZNVector3());
    body(ZNVector4());
}

TEST(Vector, LengthSq)
{
    auto body = [](auto v)
    {
        using V = decltype(v);
        v = RandomObjectWithValues<V>();
        int dimension = sizeof(v.value) / sizeof(v.value[0]);
        float r = 0.0f;
        for (int i = 0; i < dimension; ++i)
        {
            r += v.value[i] * v.value[i];
        }
        EXPECT_EQ(r, v.LengthSq());
    };
    body(ZNVector2());
    body(ZNVector3());
    body(ZNVector4());
}

TEST(Vector, Normalize)
{
    auto body = [](auto v)
    {
        using V = decltype(v);
        v = RandomObjectWithValues<V>();
        V r = v;

        float lenSq = v.LengthSq();
        if (lenSq > 0.0f)
        {
            float lenInv = 1.0f / sqrt(lenSq);
            int vSize = sizeof(v.value) / sizeof(v.value[0]);
            for (int i = 0; i < vSize; ++i)
            {
                r.value[i] *= lenInv;
            }
        }

        EXPECT_EQ(r, v.Normalize());
    };
    body(ZNVector2());
    body(ZNVector3());
    body(ZNVector4());
}
