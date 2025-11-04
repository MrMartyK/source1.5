//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Screen Space Ambient Occlusion (SSAO) shader
//
//===========================================================================//

#include "BaseVSShader.h"
#include "screenspaceeffect_vs20.inc"
#include "ssao_ps20b.inc"
#include "convar.h"

// Include SSAO implementation for kernel generation
#include "../../framework/color_grading.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// ConVars for SSAO
extern ConVar mat_ssao;
extern ConVar mat_ssao_radius;
extern ConVar mat_ssao_intensity;
extern ConVar mat_ssao_bias;
extern ConVar mat_ssao_samples;

BEGIN_VS_SHADER_FLAGS( SSAO, "Screen Space Ambient Occlusion", SHADER_NOT_EDITABLE )

	BEGIN_SHADER_PARAMS
		SHADER_PARAM( DEPTHTEXTURE, SHADER_PARAM_TYPE_TEXTURE, "_rt_SSAODepth", "Depth buffer" )
		SHADER_PARAM( NORMALTEXTURE, SHADER_PARAM_TYPE_TEXTURE, "_rt_SSAONormal", "Normal buffer" )
		SHADER_PARAM( NOISETEXTURE, SHADER_PARAM_TYPE_TEXTURE, "_rt_SSAONoise", "Noise texture" )
	END_SHADER_PARAMS

	SHADER_INIT
	{
		if ( params[DEPTHTEXTURE]->IsDefined() )
		{
			LoadTexture( DEPTHTEXTURE );
		}
		if ( params[NORMALTEXTURE]->IsDefined() )
		{
			LoadTexture( NORMALTEXTURE );
		}
		if ( params[NOISETEXTURE]->IsDefined() )
		{
			LoadTexture( NOISETEXTURE );
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
			pShaderShadow->EnableTexture( SHADER_SAMPLER0, true );  // Depth
			pShaderShadow->EnableTexture( SHADER_SAMPLER1, true );  // Normal
			pShaderShadow->EnableTexture( SHADER_SAMPLER2, true );  // Noise

			pShaderShadow->VertexShaderVertexFormat( VERTEX_POSITION, 1, 0, 0 );

			// Set shaders
			DECLARE_STATIC_VERTEX_SHADER( screenspaceeffect_vs20 );
			SET_STATIC_VERTEX_SHADER( screenspaceeffect_vs20 );

			DECLARE_STATIC_PIXEL_SHADER( ssao_ps20b );
			SET_STATIC_PIXEL_SHADER( ssao_ps20b );
		}

		DYNAMIC_STATE
		{
			// Bind textures
			BindTexture( SHADER_SAMPLER0, DEPTHTEXTURE );
			BindTexture( SHADER_SAMPLER1, NORMALTEXTURE );
			BindTexture( SHADER_SAMPLER2, NOISETEXTURE );

			// Get screen dimensions
			int width, height;
			pShaderAPI->GetBackBufferDimensions( width, height );

			// Set SSAO parameters (c0)
			float ssaoParams[4];
			ssaoParams[0] = mat_ssao_radius.GetFloat();
			ssaoParams[1] = mat_ssao_intensity.GetFloat();
			ssaoParams[2] = mat_ssao_bias.GetFloat();
			ssaoParams[3] = (float)mat_ssao_samples.GetInt();
			pShaderAPI->SetPixelShaderConstant( 0, ssaoParams, 1 );

			// Set screen size (c1)
			float screenSize[4];
			screenSize[0] = (float)width;
			screenSize[1] = (float)height;
			screenSize[2] = 1.0f / (float)width;
			screenSize[3] = 1.0f / (float)height;
			pShaderAPI->SetPixelShaderConstant( 1, screenSize, 1 );

			// Generate and set SSAO kernel (c2 - c65, up to 64 samples)
			int numSamples = mat_ssao_samples.GetInt();
			if ( numSamples > 64 ) numSamples = 64;
			if ( numSamples < 4 ) numSamples = 4;

			// Generate kernel samples
			S15::Vector3 kernel[64];
			S15::GenerateSSAOKernel( numSamples, kernel );

			// Upload kernel to shader constants
			for ( int i = 0; i < numSamples; i++ )
			{
				float kernelData[4];
				kernelData[0] = kernel[i].x;
				kernelData[1] = kernel[i].y;
				kernelData[2] = kernel[i].z;
				kernelData[3] = 0.0f;
				pShaderAPI->SetPixelShaderConstant( 2 + i, kernelData, 1 );
			}

			// Set vertex shader
			DECLARE_DYNAMIC_VERTEX_SHADER( screenspaceeffect_vs20 );
			SET_DYNAMIC_VERTEX_SHADER( screenspaceeffect_vs20 );

			// Set pixel shader
			DECLARE_DYNAMIC_PIXEL_SHADER( ssao_ps20b );
			SET_DYNAMIC_PIXEL_SHADER( ssao_ps20b );
		}

		Draw();
	}

END_SHADER
