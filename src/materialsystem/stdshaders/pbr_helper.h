//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: PBR (Physically-Based Rendering) helper functions
//          Based on Thexa4's source-pbr implementation
//          Reference: https://github.com/thexa4/source-pbr
//
//=============================================================================

#ifndef PBR_HELPER_H
#define PBR_HELPER_H

//=============================================================================
// PBR Constants
//=============================================================================

#define PI 3.14159265359

//=============================================================================
// Fresnel-Schlick Approximation
//=============================================================================

/**
 * Fresnel-Schlick approximation
 *
 * Calculates the Fresnel reflection coefficient using Schlick's approximation.
 * This determines how much light is reflected vs refracted at a surface.
 *
 * @param cosTheta Dot product of view direction and half vector (V·H)
 * @param F0 Base reflectivity at normal incidence (0° angle)
 * @return Fresnel reflection coefficient (0 to 1)
 */
float3 FresnelSchlick(float cosTheta, float3 F0)
{
	// Schlick's approximation: F0 + (1 - F0) * (1 - cos(theta))^5
	return F0 + (1.0 - F0) * pow(1.0 - saturate(cosTheta), 5.0);
}

/**
 * Fresnel-Schlick approximation with roughness
 *
 * Modified Fresnel that accounts for surface roughness.
 * Rougher surfaces have less pronounced Fresnel effect.
 *
 * @param cosTheta Dot product of view direction and normal (V·N)
 * @param F0 Base reflectivity at normal incidence
 * @param roughness Surface roughness (0 = smooth, 1 = rough)
 * @return Roughness-adjusted Fresnel coefficient
 */
float3 FresnelSchlickRoughness(float cosTheta, float3 F0, float roughness)
{
	// Account for roughness by lerping with (1 - roughness)
	return F0 + (max(float3(1.0 - roughness, 1.0 - roughness, 1.0 - roughness), F0) - F0)
	           * pow(1.0 - saturate(cosTheta), 5.0);
}

//=============================================================================
// GGX Normal Distribution Function
//=============================================================================

/**
 * GGX (Trowbridge-Reitz) Normal Distribution Function
 *
 * Statistically models the distribution of microfacet normals.
 * GGX provides realistic specular highlights with longer tails than Blinn-Phong.
 *
 * @param N Surface normal
 * @param H Half-vector between view and light directions
 * @param roughness Surface roughness (0 = smooth, 1 = rough)
 * @return Probability of microfacets aligned with H
 */
float DistributionGGX(float3 N, float3 H, float roughness)
{
	float a = roughness * roughness;
	float a2 = a * a;
	float NdotH = saturate(dot(N, H));
	float NdotH2 = NdotH * NdotH;

	float nom = a2;
	float denom = (NdotH2 * (a2 - 1.0) + 1.0);
	denom = PI * denom * denom;

	return nom / max(denom, 0.0001); // Prevent divide by zero
}

//=============================================================================
// Smith's Geometry Shadowing Function
//=============================================================================

/**
 * Schlick-GGX geometry function
 *
 * Models the probability that a microfacet is visible (not shadowed or masked).
 * Used as a building block for Smith's geometry function.
 *
 * @param NdotV Dot product of normal and view/light direction
 * @param roughness Surface roughness
 * @return Visibility probability (0 to 1)
 */
float GeometrySchlickGGX(float NdotV, float roughness)
{
	float r = (roughness + 1.0);
	float k = (r * r) / 8.0; // Direct lighting

	float nom = NdotV;
	float denom = NdotV * (1.0 - k) + k;

	return nom / max(denom, 0.0001);
}

/**
 * Smith's Geometry Shadowing-Masking Function
 *
 * Accounts for both shadowing (light blocked by microfacets) and
 * masking (view blocked by microfacets) using Smith's model.
 *
 * @param N Surface normal
 * @param V View direction
 * @param L Light direction
 * @param roughness Surface roughness
 * @return Combined shadowing-masking probability
 */
float GeometrySmith(float3 N, float3 V, float3 L, float roughness)
{
	float NdotV = saturate(dot(N, V));
	float NdotL = saturate(dot(N, L));
	float ggx2 = GeometrySchlickGGX(NdotV, roughness); // Masking
	float ggx1 = GeometrySchlickGGX(NdotL, roughness); // Shadowing

	return ggx1 * ggx2;
}

//=============================================================================
// Cook-Torrance Specular BRDF
//=============================================================================

/**
 * Cook-Torrance microfacet specular BRDF
 *
 * Combines Fresnel, Distribution, and Geometry functions to compute
 * the specular reflection for physically-based rendering.
 *
 * @param N Surface normal
 * @param V View direction (toward camera)
 * @param L Light direction (toward light)
 * @param H Half-vector (normalized V + L)
 * @param F0 Base reflectivity (for dielectrics ~0.04, metals use albedo)
 * @param roughness Surface roughness (0 = mirror, 1 = matte)
 * @return Specular reflection coefficient
 */
float3 CookTorranceSpecular(float3 N, float3 V, float3 L, float3 H, float3 F0, float roughness)
{
	// Calculate each component of the Cook-Torrance BRDF
	float D = DistributionGGX(N, H, roughness);        // Normal distribution
	float3 F = FresnelSchlick(saturate(dot(H, V)), F0); // Fresnel
	float G = GeometrySmith(N, V, L, roughness);       // Geometry shadowing

	// Cook-Torrance specular BRDF: (D * F * G) / (4 * (N·V) * (N·L))
	float3 numerator = D * F * G;
	float NdotV = saturate(dot(N, V));
	float NdotL = saturate(dot(N, L));
	float denominator = 4.0 * NdotV * NdotL;

	return numerator / max(denominator, 0.0001);
}

//=============================================================================
// Environment BRDF Approximation (Lazarov 2013)
//=============================================================================

/**
 * Environment BRDF approximation for image-based lighting
 *
 * Precomputed approximation of the specular integral for environment maps.
 * Based on "Getting More Physical in Call of Duty: Black Ops II" by Lazarov 2013.
 *
 * @param F0 Base reflectivity
 * @param roughness Surface roughness
 * @param ndotv Dot product of normal and view direction
 * @return Approximated environment BRDF scale and bias
 */
float2 EnvBRDFApprox(float3 F0, float roughness, float ndotv)
{
	// Polynomial approximation from Lazarov 2013
	const float4 c0 = float4(-1.0, -0.0275, -0.572, 0.022);
	const float4 c1 = float4(1.0, 0.0425, 1.04, -0.04);
	float4 r = roughness * c0 + c1;
	float a004 = min(r.x * r.x, exp2(-9.28 * ndotv)) * r.x + r.y;
	float2 AB = float2(-1.04, 1.04) * a004 + r.zw;

	return AB;
}

//=============================================================================
// Utility Functions
//=============================================================================

/**
 * Calculate F0 (base reflectivity) from metalness and albedo
 *
 * Dielectrics have F0 ~0.04, metals use albedo as F0.
 * This function lerps between the two based on metalness.
 *
 * @param albedo Base color
 * @param metalness Metalness value (0 = dielectric, 1 = metallic)
 * @return Base reflectivity at normal incidence
 */
float3 CalculateF0(float3 albedo, float metalness)
{
	// Dielectrics: F0 = 0.04 (plastic, glass, etc.)
	// Metals: F0 = albedo (colored reflection)
	float3 dielectricF0 = float3(0.04, 0.04, 0.04);
	return lerp(dielectricF0, albedo, metalness);
}

/**
 * Calculate diffuse contribution based on metalness
 *
 * Metals have no diffuse component (energy goes to specular).
 * Dielectrics split energy between diffuse and specular.
 *
 * @param albedo Base color
 * @param metalness Metalness value (0 = dielectric, 1 = metallic)
 * @param F Fresnel reflection coefficient
 * @return Diffuse color contribution
 */
float3 CalculateDiffuse(float3 albedo, float metalness, float3 F)
{
	// Energy conservation: kd = 1 - ks (where ks = F)
	float3 kd = 1.0 - F;

	// Metals have no diffuse component
	kd *= (1.0 - metalness);

	// Lambert diffuse: albedo / PI
	return kd * albedo / PI;
}

#endif // PBR_HELPER_H
