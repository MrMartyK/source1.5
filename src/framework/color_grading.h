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

/**
 * Adjust contrast
 *
 * Controls the difference between light and dark values, pivoting around midpoint (0.5).
 * 0.0 = no contrast (flat gray at 0.5)
 * 1.0 = normal (no change)
 * 2.0 = doubled contrast
 *
 * @param color Input color
 * @param contrast Contrast multiplier (0 to 2+)
 * @return Contrast-adjusted color
 */
Vector3 AdjustContrast(const Vector3& color, float contrast);

/**
 * Adjust brightness
 *
 * Uniformly scales all color values (simple brightness control).
 * 0.0 = black
 * 1.0 = normal (no change)
 * 2.0 = doubled brightness
 *
 * @param color Input color
 * @param brightness Brightness multiplier (0 to 2+)
 * @return Brightness-adjusted color
 */
Vector3 AdjustBrightness(const Vector3& color, float brightness);

/**
 * Generate SSAO sample kernel
 *
 * Creates a hemisphere of random sample points oriented around +Z axis.
 * Samples are lerped toward center for better distribution.
 *
 * @param sampleCount Number of samples (typically 16, 32, or 64)
 * @param kernel Output array of Vector3 samples (must be pre-allocated)
 */
void GenerateSSAOKernel(int sampleCount, Vector3* kernel);

/**
 * Calculate SSAO occlusion factor
 *
 * Compares depth samples in hemisphere around surface normal.
 * Returns occlusion factor (0 = fully occluded, 1 = no occlusion).
 *
 * @param sampleDepths Array of sampled depths (scene depth buffer)
 * @param centerDepth Depth at current fragment
 * @param radius SSAO sampling radius
 * @param sampleCount Number of depth samples
 * @return Occlusion factor (0 to 1)
 */
float CalculateSSAOOcclusion(const float* sampleDepths, float centerDepth, float radius, int sampleCount);

/**
 * Generate SSAO noise texture data (4x4 random rotation vectors)
 *
 * Creates random tangent-space rotation vectors for SSAO sampling.
 * This creates a 4x4 tiled noise pattern to randomize SSAO samples.
 *
 * @param noiseData Output array of 16 Vector3 (4x4 texture)
 */
void GenerateSSAONoise(Vector3* noiseData);

} // namespace S15

#endif // COLOR_GRADING_H
