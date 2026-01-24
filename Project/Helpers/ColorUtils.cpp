#include "ColorUtils.h"
#include "AEMath.h"

Color CreateColor(float _r, float _g, float _b, float _a)
{
    Color col = { _r, _g, _b, _a };
    return col;
}

void SetColor(Color* col, float _r, float _g, float _b, float _a)
{
    SetRGB(col, _r, _g, _b);
    SetColorAlpha(col, _a);
}

void SetColorAlpha(Color* col, float _a)
{
    col->a = AEClamp(_a, 0, 255);
}

void SetRGB(Color* col, float _r, float _g, float _b)
{
    col->r = AEClamp(_r, 0, 255);
    col->g = AEClamp(_g, 0, 255);
    col->b = AEClamp(_b, 0, 255);
}

u32 ColToHex(Color col)
{
    //AA RR GG BB
    return (u32)((((int)col.a & 0xFF) << 24) + (((int)col.r & 0xFF) << 16) + (((int)col.g & 0xFF) << 8) + ((int)col.b & 0xFF));
}
