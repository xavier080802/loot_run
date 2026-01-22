#include "Vec2Utils.h"
#include "CoordUtils.h"

AEVec2 ToVec2(f32 x, f32 y)
{
    AEVec2 vec = { x, y };
    return vec;
}

AEVec2 VecZero(void)
{
    AEVec2 vec = { 0 };
    return vec;
}

AEVec2 VecOne(void)
{
    AEVec2 vec = { 1,1 };
    return vec;
}

AEVec2 GetMouseVec(void)
{
    s32 mouseX = 0, mouseY = 0;
    AEInputGetCursorPosition(&mouseX, &mouseY);
    AEVec2 world = ScreenToCameraWorld(ToVec2((float)mouseX, (float)mouseY));
    return world;
}

AEVec2 NegVec2(AEVec2 v)
{
    AEVec2 vec = { -v.x, -v.y };
    return vec;
}

AEVec2 MultVec2(AEVec2 a, AEVec2 b)
{
    AEVec2 vec = { a.x * b.x, a.y * b.y };
    return vec;
}

bool CompareVec2(AEVec2 const& a, AEVec2 const& b)
{
    return a.x == b.x && a.y == b.y;
}

AEVec2 operator-(AEVec2 lhs, AEVec2 rhs)
{
    AEVec2 out{ lhs.x - rhs.x, lhs.y - rhs.x };
    return out;
}

AEVec2 operator-=(AEVec2& lhs, AEVec2 rhs)
{
    lhs.x -= rhs.x;
    lhs.y -= rhs.y;
    return lhs;
}

AEVec2 operator+(AEVec2 lhs, AEVec2 rhs)
{
    AEVec2 out{ lhs.x + rhs.x, lhs.y + rhs.y };
    return out;
}

AEVec2 operator+=(AEVec2& lhs, AEVec2 rhs)
{
    lhs.x += rhs.x;
    lhs.y += rhs.y;
    return lhs;
}

AEVec2 operator*(AEVec2 lhs, float rhs)
{
    return {lhs.x * rhs, lhs.y * rhs};
}
