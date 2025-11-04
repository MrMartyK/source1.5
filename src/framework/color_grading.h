//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Color grading and tonemapping utilities for Source 1.5
//          Engine-agnostic color operations (no tier dependencies)
//
//=============================================================================

#ifndef COLOR_GRADING_H
#define COLOR_GRADING_H

namespace S15 {

// Simple 3D vector for RGB color operations
struct Vector3 {
	float x, y, z;

	Vector3() : x(0.0f), y(0.0f), z(0.0f) {
	}
	Vector3(float _x, float _y, float _z) : x(_x), y(_y), z(_z) {
	}
};

/**
 * ACES Filmic Tonemap (Narkowicz 2015 approximation)
 *
 * Maps HDR color values (0 to infinity) to LDR range (0 to 1)
 * using the ACES (Academy Color Encoding System) filmic curve.
 *
 * This is a close approximation to the full ACES RRT/ODT transform
 * using a simple polynomial fit.
 *
 * Reference: "ACES Filmic Tone Mapping Curve" by Krzysztof Narkowicz
 *            https://knarkowicz.wordpress.com/2016/01/06/aces-filmic-tone-mapping-curve/
 *
 * @param x Input HDR color (linear RGB, 0 to infinity)
 * @return Output LDR color (sRGB-ready, 0 to 1)
 */
Vector3 ACESFilm(const Vector3& x);

/**
 * Convert linear RGB to gamma-corrected sRGB
 *
 * Applies gamma 2.2 curve for display output.
 * Formula: sRGB = linear^(1/2.2)
 *
 * @param linear Input color in linear space
 * @return Output color in sRGB gamma space
 */
Vector3 LinearToGamma(const Vector3& linear);

/**
 * Convert gamma-corrected sRGB to linear RGB
 *
 * Inverse of LinearToGamma, converts display colors to linear for processing.
 * Formula: linear = sRGB^2.2
 *
 * @param srgb Input color in sRGB gamma space
 * @return Output color in linear space
 */
Vector3 GammaToLinear(const Vector3& srgb);

/**
 * Adjust exposure using EV (exposure value) stops
 *
 * Exposure adjustment in photographic stops (EV).
 * Each stop doubles or halves the brightness.
 * Formula: result = color * 2^ev
 *
 * @param color Input HDR color
 * @param ev Exposure adjustment in stops (0 = no change, +1 = double, -1 = half)
 * @return Exposure-adjusted color
 */
Vector3 AdjustExposure(const Vector3& color, float ev);

/**
 * Adjust color saturation
 *
 * Controls the intensity of colors relative to grayscale.
 * 0 = full desaturation (grayscale)
 * 1 = no change
 * >1 = increased saturation
 *
 * @param color Input color
 * @param saturation Saturation multiplier (0 to 2+)
 * @return Saturation-adjusted color
 */
Vector3 AdjustSaturation(const Vector3& color, float saturation);

/**
 * Adjust color temperature (white balance)
 *
 * Adjusts the color temperature in Kelvin to simulate different lighting conditions.
 * 6500K = neutral (daylight)
 * <6500K = warm (orange/red tint)
 * >6500K = cool (blue tint)
 *
 * @param color Input color
 * @param kelvin Temperature in Kelvin (1000-40000)
 * @return Temperature-adjusted color
 */
Vector3 AdjustColorTemperature(const Vector3& color, float kelvin);

} // namespace S15

#endif // COLOR_GRADING_H
