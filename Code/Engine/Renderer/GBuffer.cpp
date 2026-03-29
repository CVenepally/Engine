#include "Engine/Renderer/GBuffer.hpp"

#ifdef USING_DX12
#include "Engine/Renderer/Texture.hpp"
#include "Engine/Renderer/DX12Renderer.hpp"
#include "Engine/Renderer/ResourceStateTracker.hpp"
#include "Engine/Renderer/CommandList.hpp"
#include "Engine/Renderer/BindlessDescriptorHeap.hpp"
#include "Engine/Renderer/RayTracingUtils.hpp"

#include "Engine/Window/Window.hpp"
#include "Engine/Core/EngineCommon.hpp"

#include <d3d12.h>
//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
GBuffer::GBuffer(ID3D12Device10* device, CommandList* commandList)
	: m_device(device)
{

	IntVec2 rtTextureDimensons = g_theWindow->GetClientDimensions();

	D3D12_HEAP_PROPERTIES heapProperties = {};
	heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;

	D3D12_RESOURCE_DESC textureDesc = {};
	textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	textureDesc.Alignment = 0;
	textureDesc.Width = rtTextureDimensons.x;
	textureDesc.Height = rtTextureDimensons.y;
	textureDesc.DepthOrArraySize = 1;
	textureDesc.MipLevels = 1;
	textureDesc.SampleDesc.Count = 1;
	textureDesc.SampleDesc.Quality = 0;
	textureDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	textureDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

	for(int i = 0; i < GBUFFER_COUNT; ++i)
	{
		if(i == GBUFFER_BASE_COLOR)
		{
			textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		}
		else
		{
			textureDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
		}

		ID3D12Resource* gBuffer = nullptr;

		HRESULT hr = m_device->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &textureDesc, D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&gBuffer));

		if(FAILED(hr))
		{
			ERROR_AND_DIE("GBuffer resource creation failed");
		}

		ResourceStateTracker::AddGlobalResourceState(gBuffer, D3D12_RESOURCE_STATE_COMMON);
		gBuffer->SetName(L"GBuffer Texture");

		commandList->TransitionBarrier(gBuffer, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES, true);

		m_gBufferTextures[i] = new Texture(m_device, gBuffer, "GBuffer");
		m_gBufferTextures[i]->CreateViews();
	}
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
GBuffer::~GBuffer()
{
	for(int i = 0; i < GBUFFER_COUNT; ++i)
	{
		if(m_gBufferTextures[i])
		{
			delete m_gBufferTextures[i];
			m_gBufferTextures[i] = nullptr;
		}
	}
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
Texture* GBuffer::GetGBuffer(GBufferType bufferType)
{
	return m_gBufferTextures[bufferType];
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void GBuffer::PrepareGBuffersForDispatch(CommandList* commandList, BindlessDescriptorHeap* heap)
{
	
	int startRootParamIndex = static_cast<int>(GlobalRTRootSignatureParameters::GBUFFER_POSITION);

	for(int i = 0; i < GBUFFER_COUNT; ++i)
	{
		commandList->TransitionBarrier(m_gBufferTextures[i]->m_textureResource, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES, true);

		TemporaryDescriptorAllocation tempAlloc = heap->AllocateTemporary(1);

		uint32_t destRanges[1] = {static_cast<uint32_t>(1)};

		static const uint32_t descriptorCopyRanges[] = {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1};

		D3D12_CPU_DESCRIPTOR_HANDLE uavHandle = m_gBufferTextures[i]->GetUAV(nullptr);

		m_device->CopyDescriptors(1, &tempAlloc.m_cpuHandle, destRanges, static_cast<uint32_t>(1), &uavHandle, descriptorCopyRanges, heap->m_heapType);
		commandList->m_d3dCommandList->SetComputeRootDescriptorTable(startRootParamIndex + i, tempAlloc.m_gpuHandle);
	}
}

#endif // USING_DX12

