//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: L4D mod render targets are specified by and accessable through this singleton
//
// $NoKeywords: $
//===========================================================================//
#include "cbase.h"
#include "tf_rendertargets.h"
#include "materialsystem/imaterialsystem.h"
#include "rendertexture.h"
#if defined( REPLAY_ENABLED )
#include "replay/replay_screenshot.h"
#endif

ConVar tf_water_resolution( "tf_water_resolution", "1024", FCVAR_NONE, "Needs to be set at game launch time to override." );
ConVar tf_monitor_resolution( "tf_monitor_resolution", "1024", FCVAR_NONE, "Needs to be set at game launch time to override." );

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
ITexture *CTFRenderTargets::CreateItemModelPanelTexture( const char *pszName, IMaterialSystem* pMaterialSystem, int iSize )
{
	return pMaterialSystem->CreateNamedRenderTargetTextureEx2(
		pszName,
		iSize, iSize, RT_SIZE_DEFAULT,
		pMaterialSystem->GetBackBufferFormat(),
		MATERIAL_RT_DEPTH_SHARED,
		TEXTUREFLAGS_CLAMPS | TEXTUREFLAGS_CLAMPT,
		0 );
}

//-----------------------------------------------------------------------------
// Purpose: Create SSAO depth render target (full-screen, R32F)
//-----------------------------------------------------------------------------
ITexture *CTFRenderTargets::CreateSSAODepthTexture( IMaterialSystem* pMaterialSystem )
{
	return pMaterialSystem->CreateNamedRenderTargetTextureEx2(
		"_rt_SSAODepth",
		1, 1, RT_SIZE_FULL_FRAME_BUFFER,
		IMAGE_FORMAT_R32F,
		MATERIAL_RT_DEPTH_NONE,
		TEXTUREFLAGS_CLAMPS | TEXTUREFLAGS_CLAMPT | TEXTUREFLAGS_POINTSAMPLE,
		CREATERENDERTARGETFLAGS_HDR );
}

//-----------------------------------------------------------------------------
// Purpose: Create SSAO normal render target (full-screen, RGBA16F)
//-----------------------------------------------------------------------------
ITexture *CTFRenderTargets::CreateSSAONormalTexture( IMaterialSystem* pMaterialSystem )
{
	return pMaterialSystem->CreateNamedRenderTargetTextureEx2(
		"_rt_SSAONormal",
		1, 1, RT_SIZE_FULL_FRAME_BUFFER,
		IMAGE_FORMAT_RGBA16161616F,
		MATERIAL_RT_DEPTH_NONE,
		TEXTUREFLAGS_CLAMPS | TEXTUREFLAGS_CLAMPT | TEXTUREFLAGS_POINTSAMPLE,
		CREATERENDERTARGETFLAGS_HDR );
}

//-----------------------------------------------------------------------------
// Purpose: Create SSAO occlusion render target (full-screen, R32F)
//-----------------------------------------------------------------------------
ITexture *CTFRenderTargets::CreateSSAOTexture( IMaterialSystem* pMaterialSystem )
{
	return pMaterialSystem->CreateNamedRenderTargetTextureEx2(
		"_rt_SSAO",
		1, 1, RT_SIZE_FULL_FRAME_BUFFER,
		IMAGE_FORMAT_R32F,
		MATERIAL_RT_DEPTH_NONE,
		TEXTUREFLAGS_CLAMPS | TEXTUREFLAGS_CLAMPT | TEXTUREFLAGS_POINTSAMPLE,
		CREATERENDERTARGETFLAGS_HDR );
}

//-----------------------------------------------------------------------------
// Purpose: Create SSAO blur render target (full-screen, R32F)
//-----------------------------------------------------------------------------
ITexture *CTFRenderTargets::CreateSSAOBlurTexture( IMaterialSystem* pMaterialSystem )
{
	return pMaterialSystem->CreateNamedRenderTargetTextureEx2(
		"_rt_SSAOBlur",
		1, 1, RT_SIZE_FULL_FRAME_BUFFER,
		IMAGE_FORMAT_R32F,
		MATERIAL_RT_DEPTH_NONE,
		TEXTUREFLAGS_CLAMPS | TEXTUREFLAGS_CLAMPT | TEXTUREFLAGS_POINTSAMPLE,
		CREATERENDERTARGETFLAGS_HDR );
}

//-----------------------------------------------------------------------------
// Purpose: Create SSAO noise texture (4x4, RGBA8)
//-----------------------------------------------------------------------------
ITexture *CTFRenderTargets::CreateSSAONoiseTexture( IMaterialSystem* pMaterialSystem )
{
	return pMaterialSystem->CreateNamedRenderTargetTextureEx2(
		"_rt_SSAONoise",
		4, 4, RT_SIZE_NO_CHANGE,
		IMAGE_FORMAT_RGBA8888,
		MATERIAL_RT_DEPTH_NONE,
		TEXTUREFLAGS_NOMIP | TEXTUREFLAGS_NOLOD | TEXTUREFLAGS_POINTSAMPLE,
		0 );
}

//-----------------------------------------------------------------------------
// Purpose: InitClientRenderTargets, interface called by the engine at material system init in the engine
// Input  : pMaterialSystem - the interface to the material system from the engine (our singleton hasn't been set up yet)
//			pHardwareConfig - the user's hardware config, useful for conditional render targets setup
//-----------------------------------------------------------------------------
extern const char *g_ItemModelPanelRenderTargetNames[];
extern const char *g_pszModelImagePanelRTName;
void CTFRenderTargets::InitClientRenderTargets( IMaterialSystem* pMaterialSystem, IMaterialSystemHardwareConfig* pHardwareConfig )
{
	BaseClass::InitClientRenderTargets( pMaterialSystem, pHardwareConfig, tf_water_resolution.GetInt(), tf_monitor_resolution.GetInt() );

	// rt for item model panels
	for ( int i = 0; i < ITEM_MODEL_IMAGE_CACHE_SIZE; i++ )
	{
		int index = m_tfRenderTargets.AddToTail();
		m_tfRenderTargets[index].Init( CreateItemModelPanelTexture( g_ItemModelPanelRenderTargetNames[i], pMaterialSystem, 256 ) );
	}

	// rt for CModelImagePanel
	int index = m_tfRenderTargets.AddToTail();
	m_tfRenderTargets[index].Init( CreateItemModelPanelTexture( g_pszModelImagePanelRTName, pMaterialSystem, 256 ) );

	CReplayScreenshotTaker::CreateRenderTarget( pMaterialSystem );

	// SSAO render targets
	m_SSAODepthTexture.Init( CreateSSAODepthTexture( pMaterialSystem ) );
	m_SSAONormalTexture.Init( CreateSSAONormalTexture( pMaterialSystem ) );
	m_SSAOTexture.Init( CreateSSAOTexture( pMaterialSystem ) );
	m_SSAOBlurTexture.Init( CreateSSAOBlurTexture( pMaterialSystem ) );
	m_SSAONoiseTexture.Init( CreateSSAONoiseTexture( pMaterialSystem ) );
}

//-----------------------------------------------------------------------------
// Purpose: Shutdown client render targets. This gets called during shutdown in the engine
// Input  :  -
//-----------------------------------------------------------------------------
void CTFRenderTargets::ShutdownClientRenderTargets()
{
	BaseClass::ShutdownClientRenderTargets();

	for ( int i = 0; i < m_tfRenderTargets.Count(); i++ )
	{
		m_tfRenderTargets[i].Shutdown();
	}
	m_tfRenderTargets.Purge();

	// SSAO render targets
	m_SSAODepthTexture.Shutdown();
	m_SSAONormalTexture.Shutdown();
	m_SSAOTexture.Shutdown();
	m_SSAOBlurTexture.Shutdown();
	m_SSAONoiseTexture.Shutdown();
}


static CTFRenderTargets g_TFRenderTargets;
EXPOSE_SINGLE_INTERFACE_GLOBALVAR( CTFRenderTargets, IClientRenderTargets, 
	CLIENTRENDERTARGETS_INTERFACE_VERSION, g_TFRenderTargets );
CTFRenderTargets* g_pTFRenderTargets = &g_TFRenderTargets;