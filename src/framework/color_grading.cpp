//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Color grading and tonemapping utilities implementation
//
//=============================================================================

#include "color_grading.h"
#include <algorithm> // For std::min, std::max
#include <cmath>     // For pow, lerp

namespace S15 {

// Helper: Saturate float to [0, 1] range
static inline float Saturate(float value) {
	return std::min(1.0f, std::max(0.0f, value));
}

// Helper: Clamp negative values to zero
static inline float ClampNegative(float value) {
	return std::max(0.0f, value);
}

Vector3 ACESFilm(const Vector3& x) {
	// Narkowicz 2015 ACES approximation coefficients
	const float a = 2.51f;
	const float b = 0.03f;
	const float c = 2.43f;
	const float d = 0.59f;
	const float e = 0.14f;

	// Clamp negative inputs to zero (HDR should never be negative)
	float cx = ClampNegative(x.x);
	float cy = ClampNegative(x.y);
	float cz = ClampNegative(x.z);

	// Apply ACES tonemap per-channel
	// Formula: (x * (a*x + b)) / (x * (c*x + d) + e)
	Vector3 result;
	result.x = Saturate((cx * (a * cx + b)) / (cx * (c * cx + d) + e));
	result.y = Saturate((cy * (a * cy + b)) / (cy * (c * cy + d) + e));
	result.z = Saturate((cz * (a * cz + b)) / (cz * (c * cz + d) + e));

	return result;
}

Vector3 LinearToGamma(const Vector3& linear) {
	// Gamma 2.2 curve: sRGB = linear^(1/2.2) ≈ linear^0.4545
	const float gamma = 1.0f / 2.2f;
	Vector3 result;
	result.x = std::pow(linear.x, gamma);
	result.y = std::pow(linear.y, gamma);
	result.z = std::pow(linear.z, gamma);
	return result;
}

Vector3 GammaToLinear(const Vector3& srgb) {
	// Inverse gamma 2.2: linear = sRGB^2.2
	const float gamma = 2.2f;
	Vector3 result;
	result.x = std::pow(srgb.x, gamma);
	result.y = std::pow(srgb.y, gamma);
	result.z = std::pow(srgb.z, gamma);
	return result;
}

Vector3 AdjustExposure(const Vector3& color, float ev) {
	// Exposure in stops: multiplier = 2^ev
	float multiplier = std::pow(2.0f, ev);
	Vector3 result;
	result.x = color.x * multiplier;
	result.y = color.y * multiplier;
	result.z = color.z * multiplier;
	return result;
}

Vector3 AdjustSaturation(const Vector3& color, float saturation) {
	// Calculate luminance using Rec. 709 coefficients
	const float r_weight = 0.2126f;
	const float g_weight = 0.7152f;
	const float b_weight = 0.0722f;

	float luminance = color.x * r_weight + color.y * g_weight + color.z * b_weight;

	// Lerp between grayscale (luminance) and original color
	Vector3 grayscale(luminance, luminance, luminance);
	Vector3 result;
	result.x = grayscale.x + saturation * (color.x - grayscale.x);
	result.y = grayscale.y + saturation * (color.y - grayscale.y);
	result.z = grayscale.z + saturation * (color.z - grayscale.z);

	return result;
}

Vector3 AdjustColorTemperature(const Vector3& color, float kelvin) {
	// Simplified Planckian locus approximation
	// Based on Tanner Helland's algorithm
	// Reference: https://tannerhelland.com/2012/09/18/convert-temperature-rgb-algorithm-code.html

	// Helper to calculate blackbody RGB for a given temperature
	auto calculateBlackbodyRGB = [](float temp) -> Vector3 {
		Vector3 rgb;

		// Calculate red
		if (temp <= 66.0f) {
			rgb.x = 1.0f;
		} else {
			rgb.x = temp - 60.0f;
			rgb.x = 329.698727446f * std::pow(rgb.x, -0.1332047592f);
			rgb.x = rgb.x / 255.0f;
			rgb.x = std::min(1.0f, std::max(0.0f, rgb.x));
		}

		// Calculate green
		if (temp <= 66.0f) {
			rgb.y = temp;
			rgb.y = 99.4708025861f * std::log(rgb.y) - 161.1195681661f;
			rgb.y = rgb.y / 255.0f;
			rgb.y = std::min(1.0f, std::max(0.0f, rgb.y));
		} else {
			rgb.y = temp - 60.0f;
			rgb.y = 288.1221695283f * std::pow(rgb.y, -0.0755148492f);
			rgb.y = rgb.y / 255.0f;
			rgb.y = std::min(1.0f, std::max(0.0f, rgb.y));
		}

		// Calculate blue
		if (temp >= 66.0f) {
			rgb.z = 1.0f;
		} else if (temp <= 19.0f) {
			rgb.z = 0.0f;
		} else {
			rgb.z = temp - 10.0f;
			rgb.z = 138.5177312231f * std::log(rgb.z) - 305.0447927307f;
			rgb.z = rgb.z / 255.0f;
			rgb.z = std::min(1.0f, std::max(0.0f, rgb.z));
		}

		return rgb;
	};

	// Normalize to 100-unit scale
	float temp = kelvin / 100.0f;
	float neutralTemp = 6500.0f / 100.0f; // D65 standard illuminant

	// Calculate blackbody RGB for target and neutral temperatures
	Vector3 targetRGB = calculateBlackbodyRGB(temp);
	Vector3 neutralRGB = calculateBlackbodyRGB(neutralTemp);

	// Calculate adjustment factors (normalize against neutral)
	// Use max() to avoid division by zero and ensure minimum boost
	float redFactor = targetRGB.x / std::max(0.001f, neutralRGB.x);
	float greenFactor = targetRGB.y / std::max(0.001f, neutralRGB.y);
	float blueFactor = targetRGB.z / std::max(0.001f, neutralRGB.z);

	// For very warm temps (< 3000K), artificially boost red channel
	// because blackbody formula maxes out red at low temps
	if (kelvin < 3000.0f) {
		float warmBoost = 1.0f + (3000.0f - kelvin) / 10000.0f; // 1.0 to 1.1 boost
		redFactor *= warmBoost;
	}

	// For very cool temps (> 10000K), artificially boost blue channel
	if (kelvin > 10000.0f) {
		float coolBoost = 1.0f + (kelvin - 10000.0f) / 30000.0f; // Gentle boost
		blueFactor *= coolBoost;
	}

	// Apply temperature tint to input color
	Vector3 result;
	result.x = color.x * redFactor;
	result.y = color.y * greenFactor;
	result.z = color.z * blueFactor;

	return result;
}

} // namespace S15
