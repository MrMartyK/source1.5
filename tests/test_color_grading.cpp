//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Unit tests for color grading and tonemapping utilities
//          Testing ACES tonemap, color conversions, and HDR handling
//
//=============================================================================

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include "color_grading.h"
#include <cmath>

using namespace S15;
using Catch::Matchers::WithinAbs;

TEST_CASE("ACESFilm tonemap handles standard values", "[color_grading][tonemap]") {
	SECTION("Black remains black") {
		Vector3 black(0.0f, 0.0f, 0.0f);
		Vector3 result = ACESFilm(black);
		REQUIRE_THAT(result.x, WithinAbs(0.0f, 0.001f));
		REQUIRE_THAT(result.y, WithinAbs(0.0f, 0.001f));
		REQUIRE_THAT(result.z, WithinAbs(0.0f, 0.001f));
	}

	SECTION("Mid-gray (~0.18) maps to visible range") {
		Vector3 midGray(0.18f, 0.18f, 0.18f);
		Vector3 result = ACESFilm(midGray);
		// ACES should map 18% gray to roughly 0.18-0.25 range
		REQUIRE(result.x > 0.15f);
		REQUIRE(result.x < 0.30f);
		REQUIRE_THAT(result.x, WithinAbs(result.y, 0.001f)); // Grayscale preserved
		REQUIRE_THAT(result.x, WithinAbs(result.z, 0.001f));
	}

	SECTION("White (1.0) remains close to white") {
		Vector3 white(1.0f, 1.0f, 1.0f);
		Vector3 result = ACESFilm(white);
		REQUIRE(result.x > 0.8f);
		REQUIRE(result.x <= 1.0f);
		REQUIRE_THAT(result.x, WithinAbs(result.y, 0.001f));
		REQUIRE_THAT(result.x, WithinAbs(result.z, 0.001f));
	}

	SECTION("HDR values (>1.0) saturate gracefully") {
		Vector3 hdr(5.0f, 5.0f, 5.0f);
		Vector3 result = ACESFilm(hdr);
		REQUIRE(result.x > 0.95f);
		REQUIRE(result.x <= 1.0f);
		REQUIRE_THAT(result.x, WithinAbs(result.y, 0.001f));
		REQUIRE_THAT(result.x, WithinAbs(result.z, 0.001f));
	}

	SECTION("Very high HDR (10.0+) approaches 1.0") {
		Vector3 veryHighHDR(10.0f, 10.0f, 10.0f);
		Vector3 result = ACESFilm(veryHighHDR);
		REQUIRE_THAT(result.x, WithinAbs(1.0f, 0.01f));
		REQUIRE_THAT(result.y, WithinAbs(1.0f, 0.01f));
		REQUIRE_THAT(result.z, WithinAbs(1.0f, 0.01f));
	}
}

TEST_CASE("ACESFilm tonemap is monotonically increasing", "[color_grading][tonemap]") {
	SECTION("Increasing input produces increasing output") {
		float prev = 0.0f;
		for (float i = 0.0f; i <= 10.0f; i += 0.1f) {
			Vector3 input(i, i, i);
			Vector3 result = ACESFilm(input);
			REQUIRE(result.x >= prev); // Monotonically increasing
			prev = result.x;
		}
	}
}

TEST_CASE("ACESFilm tonemap handles color channels independently", "[color_grading][tonemap]") {
	SECTION("Red channel only") {
		Vector3 red(1.0f, 0.0f, 0.0f);
		Vector3 result = ACESFilm(red);
		REQUIRE(result.x > 0.0f);
		REQUIRE_THAT(result.y, WithinAbs(0.0f, 0.001f));
		REQUIRE_THAT(result.z, WithinAbs(0.0f, 0.001f));
	}

	SECTION("Green channel only") {
		Vector3 green(0.0f, 1.0f, 0.0f);
		Vector3 result = ACESFilm(green);
		REQUIRE_THAT(result.x, WithinAbs(0.0f, 0.001f));
		REQUIRE(result.y > 0.0f);
		REQUIRE_THAT(result.z, WithinAbs(0.0f, 0.001f));
	}

	SECTION("Blue channel only") {
		Vector3 blue(0.0f, 0.0f, 1.0f);
		Vector3 result = ACESFilm(blue);
		REQUIRE_THAT(result.x, WithinAbs(0.0f, 0.001f));
		REQUIRE_THAT(result.y, WithinAbs(0.0f, 0.001f));
		REQUIRE(result.z > 0.0f);
	}

	SECTION("Mixed color preserves hue relationships") {
		Vector3 orange(1.0f, 0.5f, 0.0f);
		Vector3 result = ACESFilm(orange);
		// Red should be strongest, green medium, blue weakest
		REQUIRE(result.x > result.y);
		REQUIRE(result.y > result.z);
	}
}

TEST_CASE("ACESFilm tonemap handles edge cases", "[color_grading][tonemap]") {
	SECTION("Negative values clamp to zero") {
		Vector3 negative(-1.0f, -0.5f, -0.1f);
		Vector3 result = ACESFilm(negative);
		REQUIRE(result.x >= 0.0f);
		REQUIRE(result.y >= 0.0f);
		REQUIRE(result.z >= 0.0f);
	}

	SECTION("Very small positive values remain visible") {
		Vector3 tiny(0.001f, 0.001f, 0.001f);
		Vector3 result = ACESFilm(tiny);
		REQUIRE(result.x > 0.0f);
		REQUIRE(result.y > 0.0f);
		REQUIRE(result.z > 0.0f);
	}

	SECTION("Extremely high values saturate at 1.0") {
		Vector3 extreme(1000.0f, 1000.0f, 1000.0f);
		Vector3 result = ACESFilm(extreme);
		REQUIRE_THAT(result.x, WithinAbs(1.0f, 0.001f));
		REQUIRE_THAT(result.y, WithinAbs(1.0f, 0.001f));
		REQUIRE_THAT(result.z, WithinAbs(1.0f, 0.001f));
	}
}

TEST_CASE("ACESFilm tonemap matches reference values", "[color_grading][tonemap][reference]") {
	// Reference values from Narkowicz 2015 ACES approximation
	// These are verified outputs from the formula with coefficients:
	// a=2.51, b=0.03, c=2.43, d=0.59, e=0.14

	SECTION("0.5 input") {
		Vector3 input(0.5f, 0.5f, 0.5f);
		Vector3 result = ACESFilm(input);
		// Narkowicz approximation output: 0.616
		REQUIRE_THAT(result.x, WithinAbs(0.616f, 0.01f));
	}

	SECTION("2.0 input (HDR)") {
		Vector3 input(2.0f, 2.0f, 2.0f);
		Vector3 result = ACESFilm(input);
		// Narkowicz approximation output: 0.923
		REQUIRE_THAT(result.x, WithinAbs(0.923f, 0.01f));
	}

	SECTION("0.18 input (18% gray)") {
		Vector3 input(0.18f, 0.18f, 0.18f);
		Vector3 result = ACESFilm(input);
		// Narkowicz approximation output: 0.267
		REQUIRE_THAT(result.x, WithinAbs(0.267f, 0.01f));
	}
}

TEST_CASE("LinearToGamma converts linear to sRGB", "[color_grading][gamma]") {
	SECTION("Black remains black") {
		Vector3 black(0.0f, 0.0f, 0.0f);
		Vector3 result = LinearToGamma(black);
		REQUIRE_THAT(result.x, WithinAbs(0.0f, 0.001f));
		REQUIRE_THAT(result.y, WithinAbs(0.0f, 0.001f));
		REQUIRE_THAT(result.z, WithinAbs(0.0f, 0.001f));
	}

	SECTION("White remains white") {
		Vector3 white(1.0f, 1.0f, 1.0f);
		Vector3 result = LinearToGamma(white);
		REQUIRE_THAT(result.x, WithinAbs(1.0f, 0.001f));
		REQUIRE_THAT(result.y, WithinAbs(1.0f, 0.001f));
		REQUIRE_THAT(result.z, WithinAbs(1.0f, 0.001f));
	}

	SECTION("Mid-gray (0.5 linear = 0.735 sRGB)") {
		Vector3 midGray(0.5f, 0.5f, 0.5f);
		Vector3 result = LinearToGamma(midGray);
		// Linear 0.5 -> sRGB ~0.735 (pow(0.5, 1/2.2) ≈ 0.735)
		REQUIRE_THAT(result.x, WithinAbs(0.735f, 0.01f));
	}

	SECTION("18% gray (0.18 linear = 0.461 sRGB)") {
		Vector3 gray18(0.18f, 0.18f, 0.18f);
		Vector3 result = LinearToGamma(gray18);
		// Linear 0.18 -> sRGB ~0.461 (pow(0.18, 1/2.2) ≈ 0.461)
		REQUIRE_THAT(result.x, WithinAbs(0.461f, 0.01f));
	}

	SECTION("Per-channel independence") {
		Vector3 color(0.5f, 0.25f, 0.75f);
		Vector3 result = LinearToGamma(color);
		REQUIRE(result.x > result.y); // 0.5 > 0.25 in linear
		REQUIRE(result.z > result.x); // 0.75 > 0.5 in linear
	}

	SECTION("Values above 1.0 handled gracefully") {
		Vector3 hdr(2.0f, 2.0f, 2.0f);
		Vector3 result = LinearToGamma(hdr);
		// Should apply gamma curve even to HDR values
		REQUIRE(result.x > 1.0f);
	}
}

TEST_CASE("GammaToLinear converts sRGB to linear", "[color_grading][gamma]") {
	SECTION("Black remains black") {
		Vector3 black(0.0f, 0.0f, 0.0f);
		Vector3 result = GammaToLinear(black);
		REQUIRE_THAT(result.x, WithinAbs(0.0f, 0.001f));
		REQUIRE_THAT(result.y, WithinAbs(0.0f, 0.001f));
		REQUIRE_THAT(result.z, WithinAbs(0.0f, 0.001f));
	}

	SECTION("White remains white") {
		Vector3 white(1.0f, 1.0f, 1.0f);
		Vector3 result = GammaToLinear(white);
		REQUIRE_THAT(result.x, WithinAbs(1.0f, 0.001f));
		REQUIRE_THAT(result.y, WithinAbs(1.0f, 0.001f));
		REQUIRE_THAT(result.z, WithinAbs(1.0f, 0.001f));
	}

	SECTION("sRGB 0.5 = linear 0.214") {
		Vector3 srgb(0.5f, 0.5f, 0.5f);
		Vector3 result = GammaToLinear(srgb);
		// sRGB 0.5 -> Linear ~0.214 (pow(0.5, 2.2) ≈ 0.214)
		REQUIRE_THAT(result.x, WithinAbs(0.214f, 0.01f));
	}

	SECTION("Round-trip conversion is identity") {
		Vector3 original(0.5f, 0.25f, 0.75f);
		Vector3 gamma = LinearToGamma(original);
		Vector3 linear = GammaToLinear(gamma);
		REQUIRE_THAT(linear.x, WithinAbs(original.x, 0.01f));
		REQUIRE_THAT(linear.y, WithinAbs(original.y, 0.01f));
		REQUIRE_THAT(linear.z, WithinAbs(original.z, 0.01f));
	}
}

TEST_CASE("Exposure adjustment scales HDR values", "[color_grading][exposure]") {
	SECTION("Exposure 0 (no change)") {
		Vector3 color(0.5f, 0.5f, 0.5f);
		Vector3 result = AdjustExposure(color, 0.0f);
		REQUIRE_THAT(result.x, WithinAbs(0.5f, 0.001f));
		REQUIRE_THAT(result.y, WithinAbs(0.5f, 0.001f));
		REQUIRE_THAT(result.z, WithinAbs(0.5f, 0.001f));
	}

	SECTION("Exposure +1 (double brightness)") {
		Vector3 color(0.5f, 0.5f, 0.5f);
		Vector3 result = AdjustExposure(color, 1.0f);
		// 2^1 = 2.0 multiplier
		REQUIRE_THAT(result.x, WithinAbs(1.0f, 0.001f));
		REQUIRE_THAT(result.y, WithinAbs(1.0f, 0.001f));
		REQUIRE_THAT(result.z, WithinAbs(1.0f, 0.001f));
	}

	SECTION("Exposure -1 (half brightness)") {
		Vector3 color(0.8f, 0.8f, 0.8f);
		Vector3 result = AdjustExposure(color, -1.0f);
		// 2^-1 = 0.5 multiplier
		REQUIRE_THAT(result.x, WithinAbs(0.4f, 0.001f));
		REQUIRE_THAT(result.y, WithinAbs(0.4f, 0.001f));
		REQUIRE_THAT(result.z, WithinAbs(0.4f, 0.001f));
	}

	SECTION("Exposure +2 (4x brightness)") {
		Vector3 color(0.25f, 0.25f, 0.25f);
		Vector3 result = AdjustExposure(color, 2.0f);
		// 2^2 = 4.0 multiplier
		REQUIRE_THAT(result.x, WithinAbs(1.0f, 0.001f));
	}

	SECTION("Per-channel independence") {
		Vector3 color(0.5f, 0.25f, 0.75f);
		Vector3 result = AdjustExposure(color, 1.0f);
		REQUIRE_THAT(result.x, WithinAbs(1.0f, 0.001f));
		REQUIRE_THAT(result.y, WithinAbs(0.5f, 0.001f));
		REQUIRE_THAT(result.z, WithinAbs(1.5f, 0.001f));
	}
}

TEST_CASE("Saturation adjustment controls color intensity", "[color_grading][saturation]") {
	SECTION("Saturation 1.0 (no change)") {
		Vector3 color(1.0f, 0.5f, 0.25f);
		Vector3 result = AdjustSaturation(color, 1.0f);
		REQUIRE_THAT(result.x, WithinAbs(1.0f, 0.001f));
		REQUIRE_THAT(result.y, WithinAbs(0.5f, 0.001f));
		REQUIRE_THAT(result.z, WithinAbs(0.25f, 0.001f));
	}

	SECTION("Saturation 0.0 (full desaturation = grayscale)") {
		Vector3 color(1.0f, 0.5f, 0.25f);
		Vector3 result = AdjustSaturation(color, 0.0f);
		// Should be grayscale (all channels equal to luminance)
		REQUIRE_THAT(result.x, WithinAbs(result.y, 0.001f));
		REQUIRE_THAT(result.y, WithinAbs(result.z, 0.001f));
	}

	SECTION("Saturation 0.5 (half saturation)") {
		Vector3 color(1.0f, 0.0f, 0.0f); // Pure red
		Vector3 result = AdjustSaturation(color, 0.5f);
		// Should be less saturated (moving toward gray)
		REQUIRE(result.x < 1.0f);
		REQUIRE(result.y > 0.0f);
		REQUIRE(result.z > 0.0f);
	}

	SECTION("Saturation 2.0 (double saturation)") {
		Vector3 color(0.7f, 0.5f, 0.3f);
		Vector3 result = AdjustSaturation(color, 2.0f);
		// Should be more saturated (exaggerated color differences)
		float origDiff = color.x - color.z; // 0.4
		float newDiff = result.x - result.z;
		REQUIRE(newDiff > origDiff);
	}

	SECTION("Grayscale remains grayscale at any saturation") {
		Vector3 gray(0.5f, 0.5f, 0.5f);
		Vector3 result = AdjustSaturation(gray, 2.0f);
		REQUIRE_THAT(result.x, WithinAbs(0.5f, 0.001f));
		REQUIRE_THAT(result.y, WithinAbs(0.5f, 0.001f));
		REQUIRE_THAT(result.z, WithinAbs(0.5f, 0.001f));
	}
}

TEST_CASE("ColorTemperature adjusts white balance", "[color_grading][temperature]") {
	SECTION("Temperature 6500K (neutral, no change)") {
		Vector3 color(0.5f, 0.5f, 0.5f);
		Vector3 result = AdjustColorTemperature(color, 6500.0f);
		REQUIRE_THAT(result.x, WithinAbs(0.5f, 0.05f));
		REQUIRE_THAT(result.y, WithinAbs(0.5f, 0.05f));
		REQUIRE_THAT(result.z, WithinAbs(0.5f, 0.05f));
	}

	SECTION("Temperature 2000K (warm/orange)") {
		Vector3 color(0.5f, 0.5f, 0.5f);
		Vector3 result = AdjustColorTemperature(color, 2000.0f);
		// Warm: more red/orange, less blue
		REQUIRE(result.x > 0.5f); // More red
		REQUIRE(result.z < 0.5f); // Less blue
	}

	SECTION("Temperature 10000K (cool/blue)") {
		Vector3 color(0.5f, 0.5f, 0.5f);
		Vector3 result = AdjustColorTemperature(color, 10000.0f);
		// Cool: less red, more blue
		REQUIRE(result.x < 0.5f); // Less red
		REQUIRE(result.z > 0.5f); // More blue
	}

	SECTION("Temperature affects white balance") {
		Vector3 white(1.0f, 1.0f, 1.0f);
		Vector3 warm = AdjustColorTemperature(white, 3000.0f);
		Vector3 cool = AdjustColorTemperature(white, 8000.0f);
		// Warm should have more red, cool should have more blue
		REQUIRE(warm.x > cool.x);
		REQUIRE(warm.z < cool.z);
	}
}
