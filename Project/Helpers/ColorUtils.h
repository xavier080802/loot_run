#ifndef _COLOR_H_
#define _COLOR_H_
#include "AEEngine.h"

//Allows us to use rgba(0-255) where AE might use u32
struct Color {
	/// <summary>
	/// 0 to 255
	/// </summary>
	float r, g, b, a;

	Color(float _r = 0, float _g = 0, float _b = 0, float _a = 0) : r{ _r }, g{ _g }, b{ _b }, a{ _a } {}
};

/// <summary>
/// All values 0 to 255
/// </summary>
/// <returns>Color struct</returns>
Color CreateColor(float _r, float _g, float _b, float _a);

//Set the values of col to the given values
void SetColor(Color* col, float _r, float _g, float _b, float _a);
//Set the alpha of col
void SetColorAlpha(Color* col, float _a);
//Set only RGB of col
void SetRGB(Color* col, float _r, float _g, float _b);

/// <summary>
/// Converts a Color struct to a hex version Eg 0xFFFFFFFF
/// </summary>
u32 ColToHex(Color col);
#endif // !_COLOR_H_
