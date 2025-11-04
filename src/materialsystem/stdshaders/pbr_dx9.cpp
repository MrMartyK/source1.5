//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: PBR (Physically-Based Rendering) Shader
//          VMT-compatible PBR implementation for Source SDK 2013
//          Based on Thexa4's source-pbr
//
//=============================================================================

#include "BaseVSShader.h"
#include "convar.h"

// Auto-generated shader includes
#include "pbr_vs20.inc"
#include "pbr_ps20b.inc"

BEGIN_VS_SHADER( PBR, "PBR - Physically-Based Rendering" )

	BEGIN_SHADER_PARAMS
		// Base textures
		SHADER_PARAM( BASETEXTURE, SHADER_PARAM_TYPE_TEXTURE, "shadertest/BaseTexture", "Albedo texture (RGB)" )
		SHADER_PARAM( BUMPMAP, SHADER_PARAM_TYPE_TEXTURE, "shadertest/BaseTexture_normal", "Normal map" )
		SHADER_PARAM( MRAOTEXTURE, SHADER_PARAM_TYPE_TEXTURE, "", "Metalness (R), Roughness (G), AO (B)" )

		// Environment map
		SHADER_PARAM( ENVMAP, SHADER_PARAM_TYPE_ENVMAP, "env_cubemap", "Environment cubemap" )

		// Optional textures
		SHADER_PARAM( EMISSIONTEXTURE, SHADER_PARAM_TYPE_TEXTURE, "", "Emission texture" )

		// Material properties
		SHADER_PARAM( MODEL, SHADER_PARAM_TYPE_BOOL, "0", "Is this a model (1) or brush (0)?" )

		// UV transform
		SHADER_PARAM( BASETEXTURETRANSFORM, SHADER_PARAM_TYPE_MATRIX, "center .5 .5 scale 1 1 rotate 0 translate 0 0", "Base texture transform" )
	END_SHADER_PARAMS

	// Set up anything that is necessary to make decisions in SHADER_FALLBACK.
	SHADER_INIT_PARAMS()
	{
		// Set default parameters
		if ( !params[MRAOTEXTURE]->IsDefined() )
		{
			Warning( "PBR shader: $mraotexture not defined! Using white texture.\n" );
		}

		if ( !params[ENVMAP]->IsDefined() )
		{
			params[ENVMAP]->SetStringValue( "env_cubemap" );
		}

		// Set shader flags
		SET_FLAGS2( MATERIAL_VAR2_SUPPORTS_HW_SKINNING );
		SET_FLAGS2( MATERIAL_VAR2_LIGHTING_LIGHTMAP );
	}

	// Define shader fallback
	SHADER_FALLBACK
	{
		// Fallback to lightmappedgeneric if shader model is too old
		if ( g_pHardwareConfig->GetDXSupportLevel() < 90 )
			return "LightmappedGeneric";

		return 0;
	}

	SHADER_INIT
	{
		// Load textures
		if ( params[BASETEXTURE]->IsDefined() )
		{
			LoadTexture( BASETEXTURE );
		}

		if ( params[BUMPMAP]->IsDefined() )
		{
			LoadTexture( BUMPMAP );
		}

		if ( params[MRAOTEXTURE]->IsDefined() )
		{
			LoadTexture( MRAOTEXTURE );
		}

		if ( params[ENVMAP]->IsDefined() )
		{
			LoadCubeMap( ENVMAP );
		}

		if ( params[EMISSIONTEXTURE]->IsDefined() )
		{
			LoadTexture( EMISSIONTEXTURE );
		}
	}

	SHADER_DRAW
	{
		// Bind textures
		SHADOW_STATE
		{
			// Set texture stages
			pShaderShadow->EnableTexture( SHADER_SAMPLER0, true );  // Base
			pShaderShadow->EnableTexture( SHADER_SAMPLER1, true );  // Normal
			pShaderShadow->EnableTexture( SHADER_SAMPLER2, true );  // MRAO
			pShaderShadow->EnableTexture( SHADER_SAMPLER3, true );  // Envmap

			if ( params[EMISSIONTEXTURE]->IsDefined() )
			{
				pShaderShadow->EnableTexture( SHADER_SAMPLER4, true );  // Emission
			}

			// Enable sRGB read for albedo
			pShaderShadow->EnableSRGBRead( SHADER_SAMPLER0, true );

			// Set vertex format
			unsigned int flags = VERTEX_POSITION | VERTEX_NORMAL | VERTEX_TANGENT_S | VERTEX_TANGENT_T;
			int userDataSize = 0;
			pShaderShadow->VertexShaderVertexFormat( flags, 2, 0, userDataSize );

			// Set shaders
			DECLARE_STATIC_VERTEX_SHADER( pbr_vs20 );
			SET_STATIC_VERTEX_SHADER( pbr_vs20 );

			DECLARE_STATIC_PIXEL_SHADER( pbr_ps20b );
			SET_STATIC_PIXEL_SHADER_COMBO( EMISSION_TEXTURE, params[EMISSIONTEXTURE]->IsDefined() ? 1 : 0 );
			SET_STATIC_PIXEL_SHADER( pbr_ps20b );

			// Enable fog
			DefaultFog();
		}

		DYNAMIC_STATE
		{
			// Bind textures
			BindTexture( SHADER_SAMPLER0, BASETEXTURE, -1 );
			BindTexture( SHADER_SAMPLER1, BUMPMAP, -1 );
			BindTexture( SHADER_SAMPLER2, MRAOTEXTURE, -1 );
			pShaderAPI->BindStandardTexture( SHADER_SAMPLER3, TEXTURE_LOCAL_ENV_CUBEMAP );

			if ( params[EMISSIONTEXTURE]->IsDefined() )
			{
				BindTexture( SHADER_SAMPLER4, EMISSIONTEXTURE, -1 );
			}

			// Set vertex shader constants
			DECLARE_DYNAMIC_VERTEX_SHADER( pbr_vs20 );
			SET_DYNAMIC_VERTEX_SHADER( pbr_vs20 );

			// Set pixel shader constants
			DECLARE_DYNAMIC_PIXEL_SHADER( pbr_ps20b );
			SET_DYNAMIC_PIXEL_SHADER( pbr_ps20b );

			// Set standard constants
			SetVertexShaderTextureTransform( VERTEX_SHADER_SHADER_SPECIFIC_CONST_0, BASETEXTURETRANSFORM );
			SetPixelShaderFogParams( PSREG_FOG_PARAMS );

			// Eye position for specular
			float vEyePos[4];
			pShaderAPI->GetWorldSpaceCameraPosition( vEyePos );
			pShaderAPI->SetPixelShaderConstant( 0, vEyePos, 1 );
		}

		Draw();
	}

END_SHADER
