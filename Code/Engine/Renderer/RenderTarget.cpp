#include "Engine/Renderer/RenderTarget.hpp"
#include "Engine/Renderer/Texture.hpp"


#if defined(USING_DX12)

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
RenderTarget::RenderTarget()
	:m_textures(static_cast<int>(AttachmentPoint::COUNT))
{}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
RenderTarget::~RenderTarget()
{

}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void RenderTarget::AttachTexture(AttachmentPoint attachmentPoint, Texture * texture)
{
	m_textures[static_cast<int>(attachmentPoint)] = texture;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
Texture* RenderTarget::GetTexture(AttachmentPoint attachmentPoint) const
{
	return m_textures[static_cast<int>(attachmentPoint)];
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void RenderTarget::Resize(uint32_t width, uint32_t height)
{
	for(Texture* texture : m_textures)
	{
		texture->Resize(width, height);
	}
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
std::vector<Texture*> const& RenderTarget::GetTextures() const
{
	return m_textures;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
D3D12_RT_FORMAT_ARRAY RenderTarget::GetRenderTargetFormats() const
{
	D3D12_RT_FORMAT_ARRAY rtvFormats = {};

	for(int i = static_cast<int>(AttachmentPoint::RASTER_OUTPUT); i <= static_cast<int>(AttachmentPoint::COLOR_7); ++i)
	{
		Texture* texture = m_textures[i];
		if(texture->IsValid())
		{
			rtvFormats.RTFormats[rtvFormats.NumRenderTargets++] = texture->GetResourceDesc().Format;
		}
	}

	return rtvFormats;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
DXGI_FORMAT RenderTarget::GetDepthStencilFormats() const
{
	DXGI_FORMAT dsvFormat = DXGI_FORMAT_UNKNOWN;
	Texture* deptStencilTexture = m_textures[static_cast<int>(AttachmentPoint::DEPTH_STENCIL)];

	if(deptStencilTexture && deptStencilTexture->IsValid())
	{
		dsvFormat = deptStencilTexture->GetResourceDesc().Format;
	}

	return dsvFormat;
}

#endif