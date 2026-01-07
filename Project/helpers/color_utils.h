#ifndef _COLOR_H_
#define _COLOR_H_
#include "AEEngine.h"

typedef struct {
	/// <summary>
	/// 0 to 255
	/// </summary>
	float r, g, b, a;
} Color;

/// <summary>
/// All values 0 to 255
/// </summary>
/// <returns>Color struct</returns>
Color CreateColor(float _r, float _g, float _b, float _a);

void SetColor(Color* col, float _r, float _g, float _b, float _a);
void SetColorAlpha(Color* col, float _a);
void SetRGB(Color* col, float _r, float _g, float _b);

/// <summary>
/// Converts a Color struct to a hex version Eg 0xFFFFFFFF
/// </summary>
u32 ColToHex(Color col);
#endif // !_COLOR_H_
