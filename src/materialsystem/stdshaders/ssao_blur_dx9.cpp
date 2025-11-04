//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: SSAO Bilateral Blur shader
//
//===========================================================================//

#include "BaseVSShader.h"
#include "screenspaceeffect_vs20.inc"
#include "ssao_blur_ps20b.inc"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

BEGIN_VS_SHADER_FLAGS( SSAO_Blur, "SSAO Bilateral Blur", SHADER_NOT_EDITABLE )

	BEGIN_SHADER_PARAMS
		SHADER_PARAM( SSAOTEXTURE, SHADER_PARAM_TYPE_TEXTURE, "_rt_SSAO", "Raw SSAO texture" )
		SHADER_PARAM( DEPTHTEXTURE, SHADER_PARAM_TYPE_TEXTURE, "_rt_SSAODepth", "Depth buffer" )
		SHADER_PARAM( BLURDIR, SHADER_PARAM_TYPE_VEC2, "[1 0]", "Blur direction (X or Y)" )
		SHADER_PARAM( DEPTHTHRESHOLD, SHADER_PARAM_TYPE_FLOAT, "0.01", "Depth threshold for edge detection" )
	END_SHADER_PARAMS

	SHADER_INIT
	{
		if ( params[SSAOTEXTURE]->IsDefined() )
		{
			LoadTexture( SSAOTEXTURE );
		}
		if ( params[DEPTHTEXTURE]->IsDefined() )
		{
			LoadTexture( DEPTHTEXTURE );
		}
	}

	SHADER_FALLBACK
	{
		return 0;
	}

	SHADER_DRAW
	{
		SHADOW_STATE
		{
			pShaderShadow->EnableDepthWrites( false );
			pShaderShadow->EnableDepthTest( false );
			pShaderShadow->EnableAlphaWrites( false );
			pShaderShadow->EnableBlending( false );

			// Enable samplers
			pShaderShadow->EnableTexture( SHADER_SAMPLER0, true );  // SSAO
			pShaderShadow->EnableTexture( SHADER_SAMPLER1, true );  // Depth

			pShaderShadow->VertexShaderVertexFormat( VERTEX_POSITION, 1, 0, 0 );

			// Set shaders
			DECLARE_STATIC_VERTEX_SHADER( screenspaceeffect_vs20 );
			SET_STATIC_VERTEX_SHADER( screenspaceeffect_vs20 );

			DECLARE_STATIC_PIXEL_SHADER( ssao_blur_ps20b );
			SET_STATIC_PIXEL_SHADER( ssao_blur_ps20b );
		}

		DYNAMIC_STATE
		{
			// Bind textures
			BindTexture( SHADER_SAMPLER0, SSAOTEXTURE );
			BindTexture( SHADER_SAMPLER1, DEPTHTEXTURE );

			// Get screen dimensions
			int width, height;
			pShaderAPI->GetBackBufferDimensions( width, height );

			// Set blur parameters (c0)
			float blurParams[4];

			// Get blur direction from parameter
			if ( params[BLURDIR]->IsDefined() )
			{
				const float *blurDir = params[BLURDIR]->GetVecValue();
				blurParams[0] = blurDir[0] / (float)width;   // Normalize by screen size
				blurParams[1] = blurDir[1] / (float)height;
			}
			else
			{
				// Default to horizontal blur
				blurParams[0] = 1.0f / (float)width;
				blurParams[1] = 0.0f;
			}

			// Depth threshold
			blurParams[2] = params[DEPTHTHRESHOLD]->GetFloatValue();
			blurParams[3] = 0.0f;

			pShaderAPI->SetPixelShaderConstant( 0, blurParams, 1 );

			// Set vertex shader
			DECLARE_DYNAMIC_VERTEX_SHADER( screenspaceeffect_vs20 );
			SET_DYNAMIC_VERTEX_SHADER( screenspaceeffect_vs20 );

			// Set pixel shader
			DECLARE_DYNAMIC_PIXEL_SHADER( ssao_blur_ps20b );
			SET_DYNAMIC_PIXEL_SHADER( ssao_blur_ps20b );
		}

		Draw();
	}

END_SHADER
