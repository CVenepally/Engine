#pragma once
#include "Engine/Renderer/RendererUtils.hpp"
#include "Game/EngineBuildPreferences.hpp"

#ifdef USING_DX12
//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
class Texture;
class BindlessDescriptorHeap;

struct ID3D12Device10;

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
class GBuffer
{
private:
	friend class DX12Renderer;
	friend class CommandList;

	GBuffer(ID3D12Device10* device, CommandList* commandList);
	virtual ~GBuffer();

	Texture*	GetGBuffer(GBufferType bufferType);
	void		PrepareGBuffersForDispatch(CommandList* commandList, BindlessDescriptorHeap* heap);

private:
	ID3D12Device10* m_device = nullptr;
	Texture* m_gBufferTextures[GBUFFER_COUNT] = {};

};
#endif // USING_DX12

