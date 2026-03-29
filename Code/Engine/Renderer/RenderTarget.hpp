#pragma once
#include <cstdint>
#include <vector>
#include <d3d12.h>
#include "Game/EngineBuildPreferences.hpp"

#if defined(USING_DX12)
//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
class Texture;

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
enum class AttachmentPoint
{
	RASTER_OUTPUT,
	RT_OUTPUT,
	COLOR_2,
	COLOR_3,
	COLOR_4,
	COLOR_5,
	COLOR_6,
	COLOR_7,
	DEPTH_STENCIL,

	COUNT
};

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
class RenderTarget
{
	friend class DX12Renderer;
	friend class CommandList;
public:
	RenderTarget();
	virtual ~RenderTarget();

	void							AttachTexture(AttachmentPoint attachmentPoint, Texture* texture);
	Texture*						GetTexture(AttachmentPoint attachmentPoint) const;

	void							Resize(uint32_t width, uint32_t height);
	std::vector<Texture*> const&	GetTextures() const;
	
	D3D12_RT_FORMAT_ARRAY			GetRenderTargetFormats() const;
	DXGI_FORMAT						GetDepthStencilFormats() const;

private:
	std::vector<Texture*>			m_textures;

};

#endif