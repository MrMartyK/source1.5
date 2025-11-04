# PBR Shader Foundation - Source 1.5

**Date:** 2025-11-03 (Day 5 continuation)
**Status:** Foundation Complete, Build Integration Pending
**Phase:** 3 - PBR Materials (Week 3)

---

## Summary

PBR (Physically-Based Rendering) shader foundation implemented with complete BRDF functions, pixel shader, vertex shader, and material system integration. All core rendering mathematics in place following industry-standard Cook-Torrance microfacet model.

---

## Files Created

### 1. BRDF Helper Functions
**File:** `src/materialsystem/stdshaders/pbr_helper.h` (250 lines)

**Functions Implemented:**
```hlsl
// Fresnel
float3 FresnelSchlick(float cosTheta, float3 F0);
float3 FresnelSchlickRoughness(float cosTheta, float3 F0, float roughness);

// Normal Distribution
float DistributionGGX(float3 N, float3 H, float roughness);

// Geometry Shadowing
float GeometrySchlickGGX(float NdotV, float roughness);
float GeometrySmith(float3 N, float3 V, float3 L, float roughness);

// Cook-Torrance BRDF
float3 CookTorranceSpecular(float3 N, float3 V, float3 L, float3 H,
                            float3 F0, float roughness);

// Environment BRDF
float2 EnvBRDFApprox(float3 F0, float roughness, float ndotv);

// Utility
float3 CalculateF0(float3 albedo, float metalness);
float3 CalculateDiffuse(float3 albedo, float metalness, float3 F);
```

**Mathematics:**
- **GGX (Trowbridge-Reitz)** - Industry-standard normal distribution for realistic specular highlights
- **Smith's Geometry** - Accounts for shadowing and masking of microfacets
- **Schlick's Fresnel** - Approximation of Fresnel reflection with roughness support
- **Lazarov 2013** - Polynomial approximation for environment BRDF

### 2. Pixel Shader
**File:** `src/materialsystem/stdshaders/pbr_ps20b.fxc` (160 lines)

**Features:**
- MRAO texture sampling (Metalness, Roughness, AO)
- Tangent-space normal mapping with TBN matrix
- Image-based lighting (IBL) with environment cubemaps
- Fresnel-aware diffuse/specular energy conservation
- Ambient occlusion integration
- Emission texture support (optional)
- Source fog system integration
- Gamma correction for final output

**Texture Samplers:**
- `s0` - Base texture (Albedo RGB + Emission mask A)
- `s1` - Normal map (tangent space)
- `s2` - MRAO (Metalness R, Roughness G, AO B)
- `s3` - Environment cubemap
- `s4` - Emission texture (optional)

**Static Combos:**
- `EMISSION_TEXTURE` - Enable/disable emission texture sampling

### 3. Vertex Shader
**File:** `src/materialsystem/stdshaders/pbr_vs20.fxc` (70 lines)

**Features:**
- Model-to-world transformation
- World-space normal and tangent calculation
- Texture coordinate pass-through (base + lightmap)
- Projected position for fog calculation

**Inputs:**
- Position, Normal, Tangent (w/ binormal sign)
- Texture coordinates (base + lightmap)

**Outputs:**
- Clip space position
- World space position, normal, tangent
- Texture coordinates

### 4. Shader Definition
**File:** `src/materialsystem/stdshaders/pbr_dx9.cpp` (180 lines)

**VMT Parameters:**
```
$basetexture        - Albedo (sRGB)
$bumpmap            - Normal map
$mraotexture        - Metalness/Roughness/AO
$envmap             - Environment cubemap (default: env_cubemap)
$emissiontexture    - Emission (optional)
$model              - Is model (1) or brush (0)
$basetexturetransform - UV transform matrix
```

**Shader Features:**
- Automatic env_cubemap fallback
- Hardware skinning support
- Lightmap support flags
- sRGB-correct albedo sampling
- DX9 (SM 2.0b) requirement with fallback to LightmappedGeneric

---

## Technical Details

### BRDF Model

**Specular (Cook-Torrance Microfacet):**
```
f_specular = (D * F * G) / (4 * (N·V) * (N·L))

Where:
D = GGX normal distribution
F = Fresnel-Schlick reflection
G = Smith geometry shadowing-masking
```

**Diffuse (Lambertian with energy conservation):**
```
f_diffuse = (kd * albedo) / π

Where:
kd = (1 - F) * (1 - metalness)
```

**Energy Conservation:**
- Metals: 100% specular (kd = 0)
- Dielectrics: Split between diffuse and specular based on Fresnel

### Material Properties

**Metalness:**
- 0.0 = Dielectric (plastic, wood, concrete)
- 1.0 = Metallic (metals use albedo as F0)

**Roughness:**
- 0.0 = Perfectly smooth (mirror)
- 1.0 = Completely rough (matte)
- Clamped to 0.04 minimum to prevent artifacts

**F0 (Base Reflectivity):**
- Dielectrics: ~0.04 (4% reflection at normal incidence)
- Metals: Use albedo color as F0

### Image-Based Lighting

**Diffuse IBL:**
- Samples environment cubemap with surface normal
- Weighted by diffuse coefficient (kd)
- Modulated by ambient occlusion

**Specular IBL:**
- Samples environment cubemap with reflection vector
- Mip level based on roughness (rougher = blurrier reflection)
- Uses Lazarov 2013 approximation for split-sum integral

---

## Pending Work

### Build System Integration

To compile these shaders, add to shader build system:

**Windows (`buildshaders.bat`):**
```batch
:: PBR shaders
set shaders=%shaders% pbr_ps20b pbr_vs20
```

**Shader List Files:**
- Add `pbr_ps20b` to `..\devtools\bin\std_shader_list.txt`
- Add `pbr_vs20` to `..\devtools\bin\std_shader_list.txt`

**VPC (if applicable):**
- Add to `src/materialsystem/stdshaders/stdshaders_dx9.vpc`

### Testing

**Test VMT:**
```
"PBR"
{
    $basetexture "models/mymodel/albedo"
    $bumpmap "models/mymodel/normal"
    $mraotexture "models/mymodel/mrao"
    $envmap "env_cubemap"
    $model 1
}
```

**Validation:**
- [ ] Shader compiles without errors
- [ ] Material loads in game
- [ ] Albedo displays correctly
- [ ] Normal mapping works
- [ ] MRAO channels control appearance correctly
- [ ] Environment reflections visible
- [ ] Metalness scale (0-1) behaves correctly
- [ ] Roughness scale (0-1) behaves correctly

### Future Enhancements

**Direct Lighting:**
- [ ] Add dynamic light sources
- [ ] Flashlight integration
- [ ] Point light support
- [ ] Spot light support

**Advanced Features:**
- [ ] Pre-filtered specular cubemap support
- [ ] Diffuse irradiance cubemap
- [ ] Parallax mapping ($parallax, $parallaxdepth)
- [ ] Specular F0 override ($speculartexture)
- [ ] Subsurface scattering (thin materials)
- [ ] Clear coat (car paint)
- [ ] Anisotropy (brushed metals)

**Performance:**
- [ ] Quality levels (Low/Medium/High)
- [ ] Half-resolution specular option
- [ ] Shader LOD system

---

## Example Materials

### Metal (Chrome)
```
"PBR"
{
    $basetexture "materials/pbr/metal_chrome/albedo"
    $bumpmap "materials/pbr/metal_chrome/normal"
    $mraotexture "materials/pbr/metal_chrome/mrao"
    $envmap "env_cubemap"
    $model 1
    $surfaceprop "metal"
}
```
- Albedo: Light gray (~200,200,200)
- Metalness: 1.0 (full white in R channel)
- Roughness: 0.2 (smooth, polished)
- AO: Crevices darkened

### Wood (Oak)
```
"PBR"
{
    $basetexture "materials/pbr/wood_oak/albedo"
    $bumpmap "materials/pbr/wood_oak/normal"
    $mraotexture "materials/pbr/wood_oak/mrao"
    $envmap "env_cubemap"
    $model 0
    $surfaceprop "wood"
}
```
- Albedo: Brown wood color
- Metalness: 0.0 (full black in R channel)
- Roughness: 0.6 (semi-rough)
- AO: Wood grain darkened

### Plastic (Matte)
```
"PBR"
{
    $basetexture "materials/pbr/plastic_matte/albedo"
    $bumpmap "materials/pbr/plastic_matte/normal"
    $mraotexture "materials/pbr/plastic_matte/mrao"
    $envmap "env_cubemap"
    $model 1
    $surfaceprop "plastic"
}
```
- Albedo: Any color
- Metalness: 0.0 (dielectric)
- Roughness: 0.5 (matte finish)
- AO: Minimal

---

## References

### Implementation
- **Thexa4 source-pbr:** https://github.com/thexa4/source-pbr
- **Cook-Torrance BRDF:** https://en.wikipedia.org/wiki/Specular_highlight#Cook%E2%80%93Torrance_model
- **PBR Theory:** https://learnopengl.com/PBR/Theory
- **Environment BRDF:** "Getting More Physical in Call of Duty: Black Ops II" (Lazarov 2013)

### Textures
- **Substance Painter:** Standard export for MRAO workflow
- **Unreal Engine:** Compatible texture format (gametextures.com)
- **ambientCG:** Free PBR materials (https://ambientcg.com/)
- **Poly Haven:** Free HDRIs and textures (https://polyhaven.com/)

---

## Integration Checklist

- [x] BRDF helper functions (pbr_helper.h)
- [x] Pixel shader (pbr_ps20b.fxc)
- [x] Vertex shader (pbr_vs20.fxc)
- [x] Shader definition (pbr_dx9.cpp)
- [ ] Add to shader build system
- [ ] Compile shaders
- [ ] Test with sample materials
- [ ] Create 5 CC0 material library
- [ ] Write authoring guide
- [ ] Create Substance Painter preset
- [ ] Record tutorial video

---

*Last Updated: 2025-11-03*
*Status: Foundation complete, build integration pending*
