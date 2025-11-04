//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//
// $Workfile:     $
// $Date:         $
// $NoKeywords: $
//===========================================================================//
#ifndef TF_RENDERTARGETS_H
#define TF_RENDERTARGETS_H
#ifdef _WIN32
#pragma once
#endif

#include "baseclientrendertargets.h" // Base class, with interfaces called by engine and inherited members to init common render targets
#include "item_model_panel.h"

// externs
class IMaterialSystem;
class IMaterialSystemHardwareConfig;

class CTFRenderTargets : public CBaseClientRenderTargets
{
	// no networked vars
	DECLARE_CLASS_GAMEROOT( CTFRenderTargets, CBaseClientRenderTargets );
public:
	virtual void InitClientRenderTargets( IMaterialSystem* pMaterialSystem, IMaterialSystemHardwareConfig* pHardwareConfig );
	virtual void ShutdownClientRenderTargets();

	// SSAO render target accessors
	ITexture* GetSSAODepthTexture() { return m_SSAODepthTexture; }
	ITexture* GetSSAONormalTexture() { return m_SSAONormalTexture; }
	ITexture* GetSSAOTexture() { return m_SSAOTexture; }
	ITexture* GetSSAOBlurTexture() { return m_SSAOBlurTexture; }
	ITexture* GetSSAONoiseTexture() { return m_SSAONoiseTexture; }

private:
	ITexture *CreateItemModelPanelTexture( const char *pszName, IMaterialSystem* pMaterialSystem, int iSize );

	// SSAO render target creation
	ITexture *CreateSSAODepthTexture( IMaterialSystem* pMaterialSystem );
	ITexture *CreateSSAONormalTexture( IMaterialSystem* pMaterialSystem );
	ITexture *CreateSSAOTexture( IMaterialSystem* pMaterialSystem );
	ITexture *CreateSSAOBlurTexture( IMaterialSystem* pMaterialSystem );
	ITexture *CreateSSAONoiseTexture( IMaterialSystem* pMaterialSystem );

private:
	// Used for rendering item model panels.
	CUtlVector< CTextureReference >		m_tfRenderTargets;

	// SSAO render targets
	CTextureReference		m_SSAODepthTexture;
	CTextureReference		m_SSAONormalTexture;
	CTextureReference		m_SSAOTexture;
	CTextureReference		m_SSAOBlurTexture;
	CTextureReference		m_SSAONoiseTexture;
};

extern CTFRenderTargets* g_pTFRenderTargets;


#endif // TF_RENDERTARGETS_H
