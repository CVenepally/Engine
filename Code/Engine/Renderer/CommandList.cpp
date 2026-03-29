#include "Engine/Renderer/CommandList.hpp"
#include "Engine/Renderer/ResourceStateTracker.hpp"
#include "Engine/Renderer/UploadBuffer.hpp"
#include "Engine/Renderer/VertexBuffer.hpp"
#include "Engine/Renderer/IndexBuffer.hpp"
#include "Engine/Renderer/Texture.hpp"
#include "Engine/Renderer/RenderTarget.hpp"
#include "Engine/Renderer/DX12Renderer.hpp"
#include "Engine/Renderer/RootSignature.hpp"
#include "Engine/Renderer/PipelineStateObject.hpp"
#include "Engine/Renderer/DynamicDescriptorHeap.hpp"
#include "Engine/Renderer/RayTracingUtils.hpp"
#include "Engine/Renderer/RendererUtils.hpp"
#include "Engine/Renderer/ShaderTable.hpp"
#include "Engine/Renderer/ShaderRecord.hpp"
#include "Engine/Renderer/MaterialInfo.hpp"

#include "Engine/Core/Image.hpp"
#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Core/Rgba8.hpp"

#include "Engine/Math/IntVec2.hpp"

#include "Game/EngineBuildPreferences.hpp"

#if defined(USING_DX12)

std::vector<Texture*> CommandList::s_textureCache;
std::mutex CommandList::s_textureCacheMutex;

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
CommandList::CommandList(CommandQueue* commandQueue, ID3D12Device10* device, D3D12_COMMAND_LIST_TYPE type)
	: m_device(device)
	, m_commandQueue(commandQueue)
	, m_commandListType(type)
{
	
	HRESULT hr;
	hr = m_device->CreateCommandAllocator(m_commandListType, IID_PPV_ARGS(&m_d3dCommandAllocator));

	if(FAILED(hr))
	{
		ERROR_AND_DIE("Create Command Allocator Failed");
	}

	hr = m_device->CreateCommandList1(0, m_commandListType, D3D12_COMMAND_LIST_FLAG_NONE, IID_PPV_ARGS(&m_d3dCommandList));

	if(FAILED(hr))
	{
		ERROR_AND_DIE("Create Command List Failed");
	}

	switch(m_commandListType)
	{
	case D3D12_COMMAND_LIST_TYPE_COPY:
	{
		m_d3dCommandList->SetName(L"Copy Command List");
		m_d3dCommandAllocator->SetName(L"Copy Command Allocator");
		break;
	}
	case D3D12_COMMAND_LIST_TYPE_DIRECT:
	{
		m_d3dCommandList->SetName(L"Graphics Command List");
		m_d3dCommandAllocator->SetName(L"Graphics Command Allocator");
		break;
	}
	case D3D12_COMMAND_LIST_TYPE_COMPUTE:
	{
		m_d3dCommandList->SetName(L"Compute Command List");
		m_d3dCommandAllocator->SetName(L"Compute Command Allocator");
		break;
	}
	default:
		break;
	}


	m_uploadBuffer			= new UploadBuffer(this);
	m_resourceStateTracker	= new ResourceStateTracker();



	for(int i = 0; i < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES; ++i)
	{
		m_dynamicDescriptorHeap[i] = new DynamicDescriptorHeap(m_device, static_cast<D3D12_DESCRIPTOR_HEAP_TYPE>(i));
		m_boundDescriptorHeaps[i] = nullptr;
	}

	Reset();
	
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
CommandList::~CommandList()
{
	for(ID3D12Object* trackedObject : m_trackedObjects)
	{
		if(trackedObject)
		{
			DX_SAFE_RELEASE(trackedObject);
		}
	}

	for(Texture* texture : s_textureCache)
	{
		delete texture;
//		texture = nullptr;
	}
	s_textureCache.clear();

	for(int i = 0; i < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES; ++i)
	{
		if(m_dynamicDescriptorHeap[i])
		{
			delete m_dynamicDescriptorHeap[i];
		}
		
		if(m_boundDescriptorHeaps[i])
		{
			DX_SAFE_RELEASE(m_boundDescriptorHeaps[i]);
		}
	}

	delete m_resourceStateTracker;
	m_resourceStateTracker = nullptr;

	delete m_uploadBuffer;
	m_uploadBuffer = nullptr;

	DX_SAFE_RELEASE(m_d3dCommandList);
	DX_SAFE_RELEASE(m_d3dCommandAllocator);
}

//------------------------------------------------------------------------------------------------------------------
ID3D12Device10* CommandList::GetDevice() const
{
	return m_device;
}

//------------------------------------------------------------------------------------------------------------------
ID3D12GraphicsCommandList7* CommandList::GetCommandList() const
{
	return m_d3dCommandList;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
D3D12_COMMAND_LIST_TYPE CommandList::GetCommandListType() const
{
	return m_commandListType;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void CommandList::TransitionBarrier(ID3D12Resource* resource, D3D12_RESOURCE_STATES stateAfter, UINT subresource, bool flushBarriers)
{
	if(resource)
	{
		m_resourceStateTracker->TransitionResource(resource, stateAfter, subresource);
	}

	if(flushBarriers)
	{
		FlushResourceBarriers();
	}

}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void CommandList::UAVBarrier(ID3D12Resource* resource, bool flushBarriers)
{
	D3D12_RESOURCE_BARRIER uavBarrier = {};
	uavBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
	uavBarrier.UAV.pResource = resource;

	m_resourceStateTracker->ResourceBarrier(uavBarrier);

	if(flushBarriers)
	{
		FlushResourceBarriers();
	}

}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void CommandList::AliasingBarrier(ID3D12Resource * resourceBefore, ID3D12Resource* resourceAfter, bool flushBarriers)
{
	D3D12_RESOURCE_BARRIER aliasingBarrier		= {};
	aliasingBarrier.Type						= D3D12_RESOURCE_BARRIER_TYPE_ALIASING;
	aliasingBarrier.Aliasing.pResourceBefore	= resourceBefore;
	aliasingBarrier.Aliasing.pResourceAfter		= resourceAfter;

	m_resourceStateTracker->ResourceBarrier(aliasingBarrier);

	if(flushBarriers)
	{
		FlushResourceBarriers();
	}

}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void CommandList::FlushResourceBarriers()
{
	m_resourceStateTracker->FlushResourceBarriers(this);
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void CommandList::CopyResource(ID3D12Resource* destinationResource, ID3D12Resource* sourceResource)
{
	TransitionBarrier(destinationResource, D3D12_RESOURCE_STATE_COPY_DEST);
	TransitionBarrier(sourceResource, D3D12_RESOURCE_STATE_COPY_SOURCE);

	FlushResourceBarriers();

	m_d3dCommandList->CopyResource(destinationResource, sourceResource);

// 	TrackObject(destinationResource);
// 	TrackObject(sourceResource);

}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void CommandList::ResolveSubresources(ID3D12Resource* destinationResource, ID3D12Resource* sourceResource, uint32_t dstSubresource, uint32_t srcSubresource)
{
	TransitionBarrier(destinationResource, D3D12_RESOURCE_STATE_RESOLVE_DEST, dstSubresource);
	TransitionBarrier(sourceResource, D3D12_RESOURCE_STATE_RESOLVE_SOURCE, srcSubresource);

	FlushResourceBarriers();

	m_d3dCommandList->ResolveSubresource(destinationResource, dstSubresource, sourceResource, srcSubresource, destinationResource->GetDesc().Format);

// 	TrackObject(sourceResource);
// 	TrackObject(destinationResource);
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void CommandList::CopyVertexBuffer(VertexBuffer* vertexBuffer, size_t numVertices, size_t vertexStride, const void* vertexBufferData)
{
	ID3D12Resource* vbResource = CreateBufferResource(numVertices, vertexStride, vertexBufferData);

	vbResource->SetName(L"Vertex Buffer Resource");

	vertexBuffer->SetD3DResource(vbResource);
	vertexBuffer->CreateVertexBufferView(numVertices, vertexStride);
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void CommandList::CopyIndexBuffer(IndexBuffer* indexBuffer, size_t numIndexes, DXGI_FORMAT indexFormat, const void* indexBufferData)
{
	size_t indexSizeInBytes = indexFormat == DXGI_FORMAT_R16_UINT ? 2 : 4;
	ID3D12Resource* ibResource = CreateBufferResource(numIndexes, indexSizeInBytes, indexBufferData);
	ibResource->SetName(L"Index Buffer Resource");

	indexBuffer->SetD3DResource(ibResource);
	indexBuffer->CreateIndexBufferView(numIndexes, indexSizeInBytes);
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
ID3D12Resource* CommandList::CreateBufferResource(size_t numElements, size_t elementSize, const void* bufferData, D3D12_RESOURCE_FLAGS flags)
{
	ID3D12Resource* bufferResource = nullptr;

	if(numElements == 0 || !bufferData)
	{
		return bufferResource;
	}

	size_t bufferSize = numElements * elementSize;

	D3D12_HEAP_PROPERTIES heapProperties	= {};
	heapProperties.Type						= D3D12_HEAP_TYPE_DEFAULT;
	heapProperties.CreationNodeMask			= 1;
	heapProperties.VisibleNodeMask			= 1;

	D3D12_RESOURCE_DESC bufferResourceDesc	= {};
	bufferResourceDesc.Dimension			= D3D12_RESOURCE_DIMENSION_BUFFER;
	bufferResourceDesc.Width				= static_cast<UINT64>(bufferSize);
	bufferResourceDesc.Height				= 1;
	bufferResourceDesc.DepthOrArraySize		= 1;
	bufferResourceDesc.MipLevels			= 1;
	bufferResourceDesc.SampleDesc.Count		= 1;
	bufferResourceDesc.SampleDesc.Quality	= 0;
	bufferResourceDesc.Layout				= D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	bufferResourceDesc.Flags				= flags;

	D3D12_RESOURCE_STATES bufferResourceState = D3D12_RESOURCE_STATE_COMMON;

	// Create the buffer resource
	HRESULT hr;
	hr = m_device->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &bufferResourceDesc, bufferResourceState, nullptr, IID_PPV_ARGS(&bufferResource));

	if(FAILED(hr))
	{
		ERROR_AND_DIE("Buffer Resource Creation Failed. CommandList::CreateBufferResource");
	}
	ResourceStateTracker::AddGlobalResourceState(bufferResource, bufferResourceState);

	// Upload resource to copy buffer resource
	ID3D12Resource* uploadResource = nullptr;

	D3D12_HEAP_PROPERTIES uploadHeapProperties	= {};
	uploadHeapProperties.Type					= D3D12_HEAP_TYPE_UPLOAD;
	uploadHeapProperties.CreationNodeMask		= 1;
	uploadHeapProperties.VisibleNodeMask		= 1;

	D3D12_RESOURCE_DESC uploadResourceDesc = {};
	uploadResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	uploadResourceDesc.Width = static_cast<UINT64>(bufferSize);
	uploadResourceDesc.Height = 1;
	uploadResourceDesc.DepthOrArraySize = 1;
	uploadResourceDesc.MipLevels = 1;
	uploadResourceDesc.SampleDesc.Count = 1;
	uploadResourceDesc.SampleDesc.Quality = 0;
	uploadResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	uploadResourceDesc.Flags = flags;

	D3D12_RESOURCE_STATES uploadResourceState = D3D12_RESOURCE_STATE_GENERIC_READ;

	hr = m_device->CreateCommittedResource(&uploadHeapProperties, D3D12_HEAP_FLAG_NONE, &uploadResourceDesc, uploadResourceState, nullptr, IID_PPV_ARGS(&uploadResource));
	
	if(FAILED(hr))
	{
		ERROR_AND_DIE("Upload Resource Creation Failed. CommandList::CreateBufferResource");
	}
	uploadResource->SetName(L"Upload Resource");

	D3D12_SUBRESOURCE_DATA subresourceData	= {};
	subresourceData.pData					= bufferData;
	subresourceData.RowPitch				= bufferSize;
	subresourceData.SlicePitch				= subresourceData.RowPitch;

	m_resourceStateTracker->TransitionResource(bufferResource, D3D12_RESOURCE_STATE_COPY_DEST);
	FlushResourceBarriers();

	UpdateSubresources(bufferResource, uploadResource, 0, 0, 1, &subresourceData);

	TrackObject(uploadResource);

	return bufferResource;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
ID3D12Resource* CommandList::CreateUAVBufferResource(UINT64 bufferSize, D3D12_RESOURCE_STATES initialResourceState)
{
	D3D12_HEAP_PROPERTIES heapProperties = {};
	heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;
	heapProperties.CreationNodeMask = 1;
	heapProperties.VisibleNodeMask = 1;

	D3D12_RESOURCE_DESC bufferResourceDesc = {};
	bufferResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	bufferResourceDesc.Width = static_cast<UINT64>(bufferSize);
	bufferResourceDesc.Height = 1;
	bufferResourceDesc.DepthOrArraySize = 1;
	bufferResourceDesc.MipLevels = 1;
	bufferResourceDesc.SampleDesc.Count = 1;
	bufferResourceDesc.SampleDesc.Quality = 0;
	bufferResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	bufferResourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

	ID3D12Resource* bufferResource = nullptr;


	HRESULT hr = m_device->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &bufferResourceDesc, D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&bufferResource));

	if(FAILED(hr))
	{
		ERROR_AND_DIE("UAV Buffer Resource Creation failed. CommandList::CreateUAVBufferResource");
	}

	ResourceStateTracker::AddGlobalResourceState(bufferResource, D3D12_RESOURCE_STATE_COMMON);


	TransitionBarrier(bufferResource, initialResourceState);
	FlushResourceBarriers();

	bufferResource->SetName(L"UAV BUffer Resource");

	return bufferResource;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
ID3D12Resource* CommandList::CreateAccelerationStructure(UINT64 bufferSize, D3D12_RESOURCE_FLAGS flags, std::wstring const& debugName)
{
	UNUSED(flags)
	//The resource state HAS TO BE D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE and should have the ALLOW_UNORDERED_ACCESS flag

	D3D12_HEAP_PROPERTIES heapProperties = {};
	heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;
	heapProperties.CreationNodeMask = 1;
	heapProperties.VisibleNodeMask = 1;

	D3D12_RESOURCE_DESC bufferResourceDesc = {};
	bufferResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	bufferResourceDesc.Width = static_cast<UINT64>(bufferSize);
	bufferResourceDesc.Height = 1;
	bufferResourceDesc.DepthOrArraySize = 1;
	bufferResourceDesc.MipLevels = 1;
	bufferResourceDesc.SampleDesc.Count = 1;
	bufferResourceDesc.SampleDesc.Quality = 0;
	bufferResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	bufferResourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

	ID3D12Resource* bufferResource = nullptr;


	HRESULT hr = m_device->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &bufferResourceDesc, D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE, nullptr, IID_PPV_ARGS(&bufferResource));

	if(FAILED(hr))
	{
		ERROR_AND_DIE("Acceleration Structure Resource Creation failed. CommandList::CreateAccelerationStructure");
	}

	ResourceStateTracker::AddGlobalResourceState(bufferResource, D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE);

	bufferResource->SetName(debugName.c_str());

	return bufferResource;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
ID3D12Resource* CommandList::CreateUploadResource(void* uploadData, size_t uploadDataSize, std::wstring const& name)
{
	ID3D12Resource* uploadResource = nullptr;

	D3D12_HEAP_PROPERTIES uploadHeapProperties = {};
	uploadHeapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;
	uploadHeapProperties.CreationNodeMask = 1;
	uploadHeapProperties.VisibleNodeMask = 1;

	D3D12_RESOURCE_DESC uploadResourceDesc = {};
	uploadResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	uploadResourceDesc.Width = static_cast<UINT64>(uploadDataSize);
	uploadResourceDesc.Height = 1;
	uploadResourceDesc.DepthOrArraySize = 1;
	uploadResourceDesc.MipLevels = 1;
	uploadResourceDesc.SampleDesc.Count = 1;
	uploadResourceDesc.SampleDesc.Quality = 0;
	uploadResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	uploadResourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

	D3D12_RESOURCE_STATES uploadResourceState = D3D12_RESOURCE_STATE_GENERIC_READ;

	HRESULT hr = m_device->CreateCommittedResource(&uploadHeapProperties, D3D12_HEAP_FLAG_NONE, &uploadResourceDesc, uploadResourceState, nullptr, IID_PPV_ARGS(&uploadResource));

	if(FAILED(hr))
	{
		ERROR_AND_DIE("Upload Resource Creation failed. CommandList::CreateUploadResource");
	}

	uploadResource->SetName(name.c_str());

	ResourceStateTracker::AddGlobalResourceState(uploadResource, uploadResourceState);

	void* mappedData;
	uploadResource->Map(0, nullptr, &mappedData);
	memcpy(mappedData, uploadData, uploadDataSize);
	uploadResource->Unmap(0, nullptr);

	return uploadResource;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void CommandList::SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY primitiveTopology)
{
	m_d3dCommandList->IASetPrimitiveTopology(primitiveTopology);
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void CommandList::SetGraphicsDynamicConstantBuffer(uint32_t rootParameterIndex, size_t sizeInBytes, const void* bufferData)
{
	AllocationResult heapAllocation = m_uploadBuffer->Allocate(sizeInBytes, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);
	memcpy(heapAllocation.m_cpuAddress, bufferData, sizeInBytes);

	m_d3dCommandList->SetGraphicsRootConstantBufferView(rootParameterIndex, heapAllocation.m_gpuAddress);
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void CommandList::SetComputeDynamicConstantBuffer(uint32_t rootParameterIndex, size_t sizeInBytes, const void* bufferData)
{
	AllocationResult heapAllocation = m_uploadBuffer->Allocate(sizeInBytes, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);
	memcpy(heapAllocation.m_cpuAddress, bufferData, sizeInBytes);

	m_d3dCommandList->SetComputeRootConstantBufferView(rootParameterIndex, heapAllocation.m_gpuAddress);
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void CommandList::SetGraphics32BitConstants(uint32_t rootParameterIndex, uint32_t numConstants, const void* constants)
{
	m_d3dCommandList->SetGraphicsRoot32BitConstants(rootParameterIndex, numConstants, constants, 0);
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void CommandList::SetCompute32BitConstants(uint32_t rootParameterIndex, uint32_t numConstants, const void* constants)
{
	m_d3dCommandList->SetComputeRoot32BitConstants(rootParameterIndex, numConstants, constants, 0);
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void CommandList::SetVertexBuffer(uint32_t slot, VertexBuffer const& vertexBuffer)
{
	TransitionBarrier(vertexBuffer.m_buffer, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);

	D3D12_VERTEX_BUFFER_VIEW vertexBufferView = vertexBuffer.m_vbView;

	m_d3dCommandList->IASetVertexBuffers(slot, 1, &vertexBufferView);

	//TrackObject(vertexBuffer.m_buffer);
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void CommandList::SetStructuredVertexBuffer(uint32_t slot, VertexBuffer const& vertexBuffer)
{
	D3D12_RESOURCE_STATES stateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;

	TransitionBarrier(vertexBuffer.m_buffer, stateAfter);

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.ViewDimension					= D3D12_SRV_DIMENSION_BUFFER;
	srvDesc.Format							= DXGI_FORMAT_UNKNOWN;
	srvDesc.Buffer.FirstElement				= 0;
	srvDesc.Buffer.NumElements				= static_cast<UINT>(vertexBuffer.m_numElements);
	srvDesc.Buffer.StructureByteStride		= static_cast<UINT>(vertexBuffer.m_sizeOfEachElement);
	srvDesc.Buffer.Flags					= D3D12_BUFFER_SRV_FLAG_NONE;
	srvDesc.Shader4ComponentMapping			= D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

	m_dynamicDescriptorHeap[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV]->StageDescriptors(slot, 0, 1, vertexBuffer.GetSRV(&srvDesc));
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void CommandList::SetDynamicVertexBuffer(uint32_t slot, size_t numVertices, size_t vertexSize, const void* vertexBufferData)
{
	size_t bufferSize = numVertices * vertexSize;

	AllocationResult heapAllocation = m_uploadBuffer->Allocate(bufferSize, vertexSize);
	memcpy(heapAllocation.m_cpuAddress, vertexBufferData, bufferSize);

	D3D12_VERTEX_BUFFER_VIEW vbView = {};
	vbView.BufferLocation			= heapAllocation.m_gpuAddress;
	vbView.SizeInBytes				= static_cast<UINT>(bufferSize);
	vbView.StrideInBytes			= static_cast<UINT>(vertexSize);
	
	m_d3dCommandList->IASetVertexBuffers(slot, 1, &vbView);
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void CommandList::SetIndexBuffer(IndexBuffer const& indexBuffer)
{
	TransitionBarrier(indexBuffer.m_buffer, D3D12_RESOURCE_STATE_INDEX_BUFFER);

	D3D12_INDEX_BUFFER_VIEW ibView = indexBuffer.m_ibView;

	m_d3dCommandList->IASetIndexBuffer(&ibView);

//	TrackObject(indexBuffer.m_buffer);
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void CommandList::SetStructuredIndexBuffer(uint32_t slot, IndexBuffer const& indexBuffer)
{
	D3D12_RESOURCE_STATES stateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;

	TransitionBarrier(indexBuffer.m_buffer, stateAfter);

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.ViewDimension					= D3D12_SRV_DIMENSION_BUFFER;
	srvDesc.Format							= DXGI_FORMAT_UNKNOWN;
	srvDesc.Buffer.FirstElement				= 0;
	srvDesc.Buffer.NumElements				= static_cast<UINT>(indexBuffer.m_numElements);
	srvDesc.Buffer.StructureByteStride		= static_cast<UINT>(indexBuffer.m_sizeOfEachElement);
	srvDesc.Buffer.Flags					= D3D12_BUFFER_SRV_FLAG_NONE;
	srvDesc.Shader4ComponentMapping			= D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

	m_dynamicDescriptorHeap[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV]->StageDescriptors(slot, 0, 1, indexBuffer.GetSRV(&srvDesc));
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void CommandList::SetDynamicIndexBuffer(size_t numIndices, DXGI_FORMAT indexFormat, const void* indexBufferData)
{
	size_t indexSizeInBytes = indexFormat == DXGI_FORMAT_R16_UINT ? 2 : 4;
	size_t bufferSize		= numIndices * indexSizeInBytes;

	AllocationResult heapAllocation = m_uploadBuffer->Allocate(bufferSize, indexSizeInBytes);
	memcpy(heapAllocation.m_cpuAddress, indexBufferData, bufferSize);

	D3D12_INDEX_BUFFER_VIEW ibView	= {};
	ibView.BufferLocation			= heapAllocation.m_gpuAddress;
	ibView.SizeInBytes				= static_cast<UINT>(bufferSize);
	ibView.Format					= indexFormat;

	m_d3dCommandList->IASetIndexBuffer(&ibView);
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void CommandList::SetViewport(D3D12_VIEWPORT const& viewport)
{
	SetViewports({viewport});
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void CommandList::SetViewports(std::vector<D3D12_VIEWPORT> const& viewports)
{
	GUARANTEE_OR_DIE(viewports.size() < D3D12_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE, "Vector of Viewports has more Viewports than max per pipeline. CommandList::SetViewports")
	m_d3dCommandList->RSSetViewports(static_cast<UINT>(viewports.size()), viewports.data());
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void CommandList::SetScissorRect(D3D12_RECT const& scissorRect)
{
	SetScissorRects({scissorRect});
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void CommandList::SetScissorRects(std::vector<D3D12_RECT> const& scissorRects)
{
	GUARANTEE_OR_DIE(scissorRects.size() < D3D12_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE, "Vector of Scissor Rects has more Scissor Rects than max per pipeline. CommandList::SetScissorRects")
	m_d3dCommandList->RSSetScissorRects(static_cast<UINT>(scissorRects.size()), scissorRects.data());
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void CommandList::SetPipelineState(PipelineStateObject const& pipelineState)
{
	ID3D12PipelineState* d3dPipelineState = pipelineState.GetD3DPSO();
	m_d3dCommandList->SetPipelineState(d3dPipelineState);
//	TrackObject(d3dPipelineState);
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void CommandList::SetGraphicsRootSignature(RootSignature const& rootSignature)
{
	ID3D12RootSignature* d3dRootSignature = rootSignature.GetRootSignature();
	if(m_rootSignature != d3dRootSignature)
	{
		m_rootSignature = d3dRootSignature;

		for(int i = 0; i < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES; ++i)
		{
			m_dynamicDescriptorHeap[i]->ParseRootSignature(rootSignature);
		}

		m_d3dCommandList->SetGraphicsRootSignature(m_rootSignature);
//		TrackObject(m_rootSignature);
	}
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void CommandList::SetComputeRootSignature(RootSignature const& rootSignature)
{
	ID3D12RootSignature* d3dRootSignature = rootSignature.GetRootSignature();
	if(m_rootSignature != d3dRootSignature)
	{
		m_rootSignature = d3dRootSignature;

// 		for(int i = 0; i < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES; ++i)
// 		{
// 			m_dynamicDescriptorHeap[i]->ParseRootSignature(rootSignature);
// 		}

		m_d3dCommandList->SetComputeRootSignature(m_rootSignature);
	//	TrackObject(m_rootSignature);
	}
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
Texture* CommandList::GetTexture(std::string const& textureFilePath)
{
	for(Texture* texture : s_textureCache)
	{
		if(texture->GetImageFilePath() == textureFilePath)
		{
			return texture;
		}
	}

	return nullptr;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
Texture* CommandList::LoadTextureFromFile(std::string const& textureFilePath)
{
	Image textureImage = Image(textureFilePath.c_str());

	Texture* texture = CreateTextureFromImage(textureImage, textureFilePath);

	return texture;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
Texture* CommandList::CreateTextureFromImage(Image const& textureImage, std::string const& name)
{
	Texture* texture = new Texture(m_device, name);
	texture->m_dimensions = textureImage.GetDimensions();

	D3D12_RESOURCE_DESC textureDesc = {};
	textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	textureDesc.Width = textureImage.GetDimensions().x;
	textureDesc.Height = textureImage.GetDimensions().y;
	textureDesc.DepthOrArraySize = 1;
	textureDesc.MipLevels = 1;
	textureDesc.SampleDesc.Count = 1;
	textureDesc.SampleDesc.Quality = 0;
	textureDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	textureDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

	D3D12_HEAP_PROPERTIES heapProp = {};
	heapProp.Type = D3D12_HEAP_TYPE_DEFAULT;
	heapProp.CreationNodeMask = 1;
	heapProp.VisibleNodeMask = 1;

	HRESULT hr;

	hr = m_device->CreateCommittedResource(&heapProp, D3D12_HEAP_FLAG_NONE, &textureDesc, D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&texture->m_textureResource));

	if(FAILED(hr))
	{
		ERROR_AND_DIE("Texture Creation Failed")
	}

	std::wstring wname(name.begin(), name.end());

	texture->m_textureResource->SetName(wname.c_str());

	ResourceStateTracker::AddGlobalResourceState(texture->m_textureResource, D3D12_RESOURCE_STATE_COMMON);

	texture->CreateViews();

	// #TODO: MipMaps
	D3D12_SUBRESOURCE_DATA subresource = {};
	subresource.pData = textureImage.GetRawData();
	subresource.RowPitch = static_cast<LONG_PTR>(texture->GetDimensions().x * sizeof(Rgba8));
	subresource.SlicePitch = static_cast<LONG_PTR>(texture->GetDimensions().y * subresource.RowPitch);

	std::vector<D3D12_SUBRESOURCE_DATA> subresources;
	subresources.push_back(subresource);

	CopyTextureSubresource(&texture->m_textureResource, 0, static_cast<uint32_t>(subresources.size()), subresources.data());

	std::lock_guard<std::mutex> lock(s_textureCacheMutex);
	s_textureCache.push_back(texture);

	return texture;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void CommandList::CopyTextureSubresource(ID3D12Resource** texture, uint32_t firstSubresource, uint32_t numSubresources, D3D12_SUBRESOURCE_DATA* subresourceData)
{
	if(texture)
	{
// 		TransitionBarrier(texture->m_textureResource, D3D12_RESOURCE_STATE_COPY_DEST);
// 		FlushResourceBarriers();

		UINT64 requiredSize = GetRequiredIntermediateSize(*texture, firstSubresource, numSubresources);

		ID3D12Resource* uploadResource = nullptr;

		D3D12_HEAP_PROPERTIES uploadHeapProperties	= {};
		uploadHeapProperties.Type					= D3D12_HEAP_TYPE_UPLOAD;
		uploadHeapProperties.CreationNodeMask		= 1;
		uploadHeapProperties.VisibleNodeMask		= 1;

		D3D12_RESOURCE_DESC uploadResourceDesc		= {};
		uploadResourceDesc.Dimension				= D3D12_RESOURCE_DIMENSION_BUFFER;
		uploadResourceDesc.Width					= static_cast<UINT64>(requiredSize);
		uploadResourceDesc.Height					= static_cast<UINT64>(1);
		uploadResourceDesc.DepthOrArraySize			= static_cast<UINT64>(1);
		uploadResourceDesc.MipLevels				= 1;
		uploadResourceDesc.Format					= DXGI_FORMAT_UNKNOWN;
		uploadResourceDesc.SampleDesc.Count			= 1;
		uploadResourceDesc.SampleDesc.Quality		= 0;
		uploadResourceDesc.Layout					= D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		uploadResourceDesc.Flags					= D3D12_RESOURCE_FLAG_NONE;

		D3D12_RESOURCE_STATES uploadResourceState = D3D12_RESOURCE_STATE_GENERIC_READ;

		HRESULT hr;

		hr = m_device->CreateCommittedResource(&uploadHeapProperties, D3D12_HEAP_FLAG_NONE, &uploadResourceDesc, uploadResourceState, nullptr, IID_PPV_ARGS(&uploadResource));

		if(FAILED(hr))
		{
			ERROR_AND_DIE("Upload Resource Creation Failed. CommandList::CreateBufferResource");
		}
		uploadResource->SetName(L"Upload Resource");

		UpdateSubresources(*texture, uploadResource, 0, firstSubresource, numSubresources, subresourceData);

		TrackObject(uploadResource);
	}
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void CommandList::SetTextureShaderResourceView(uint32_t rootParameterIndex, uint32_t descriptorOffset, Texture* texture, D3D12_RESOURCE_STATES stateAfter,
															UINT firstSubresource, UINT numSubresources, D3D12_SHADER_RESOURCE_VIEW_DESC* srv)
{
	if(numSubresources < D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES)
	{
		for(uint32_t i = 0; i < numSubresources; ++i)
		{
			TransitionBarrier(texture->m_textureResource, stateAfter, firstSubresource + i);
		}
	}
	else
	{
		TransitionBarrier(texture->m_textureResource, stateAfter);
	}

	m_dynamicDescriptorHeap[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV]->StageDescriptors(rootParameterIndex, descriptorOffset, 1, texture->GetSRV(srv));
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void CommandList::SetTextureUAV(uint32_t rootParameterIndex, uint32_t descriptorOffset, Texture* texture, D3D12_UNORDERED_ACCESS_VIEW_DESC* uav)
{
	m_dynamicDescriptorHeap[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV]->StageDescriptors(rootParameterIndex, descriptorOffset, 1, texture->GetUAV(uav));
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void CommandList::SetRenderTarget(RenderTarget const& renderTarget)
{
	std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> renderTargetDescriptors;
	renderTargetDescriptors.reserve(static_cast<int>(AttachmentPoint::COUNT));

	std::vector<Texture*> textures = renderTarget.GetTextures();

	for(int i = 0; i < 8; ++i)
	{
		Texture* texture = textures[i];

		if(texture && texture->IsValid() && i != static_cast<int>(AttachmentPoint::RT_OUTPUT))
		{
			TransitionBarrier(texture->m_textureResource, D3D12_RESOURCE_STATE_RENDER_TARGET);
			renderTargetDescriptors.push_back(texture->GetRTV());
		}
	}

	Texture* depthTexture = renderTarget.GetTexture(AttachmentPoint::DEPTH_STENCIL);

	D3D12_CPU_DESCRIPTOR_HANDLE depthStencilDescriptor = {};
	depthStencilDescriptor.ptr = 0;

	if(depthTexture->GetTextureResource())
	{
		TransitionBarrier(depthTexture->m_textureResource, D3D12_RESOURCE_STATE_DEPTH_WRITE);
		depthStencilDescriptor = depthTexture->GetDSV();
	}

	D3D12_CPU_DESCRIPTOR_HANDLE* pDSV = depthStencilDescriptor.ptr != 0? &depthStencilDescriptor : nullptr;

	m_d3dCommandList->OMSetRenderTargets(static_cast<UINT>(renderTargetDescriptors.size()), renderTargetDescriptors.data(), FALSE, pDSV);
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void CommandList::ClearRenderTargetTexture(Texture* texture, Rgba8 const& clearColor)
{
	float color[4];
	clearColor.GetAsFloat(color);
	TransitionBarrier(texture->m_textureResource, D3D12_RESOURCE_STATE_RENDER_TARGET);
	m_d3dCommandList->ClearRenderTargetView(texture->GetRTV(), color, 0, nullptr);
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void CommandList::ClearDepthStencilTexture(Texture* texture, D3D12_CLEAR_FLAGS flag, float depth, uint8_t stencil)
{
	TransitionBarrier(texture->m_textureResource, D3D12_RESOURCE_STATE_DEPTH_WRITE);
	m_d3dCommandList->ClearDepthStencilView(texture->GetDSV(), flag, depth, stencil, 0, nullptr);
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void CommandList::Draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t startVertex, uint32_t startInstance)
{
	FlushResourceBarriers();
	for(int i = 0; i < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES; ++i)
	{
		m_dynamicDescriptorHeap[i]->CommitStagedDescriptorsForDraw(*this);
	}

	m_d3dCommandList->DrawInstanced(vertexCount, instanceCount, startVertex, startInstance);
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void CommandList::DrawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t startIndex, int32_t baseVertex, uint32_t startInstance)
{
	FlushResourceBarriers();
	for(int i = 0; i < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES; ++i)
	{
		m_dynamicDescriptorHeap[i]->CommitStagedDescriptorsForDraw(*this);
	}

	m_d3dCommandList->DrawIndexedInstanced(indexCount, instanceCount, startIndex, baseVertex, startInstance);
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void CommandList::Dispatch(uint32_t numGroupsX, uint32_t numGroupsY, uint32_t numGroupsZ)
{
	FlushResourceBarriers();

	for(int i = 0; i < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES; ++i)
	{
		m_dynamicDescriptorHeap[i]->CommitStagedDescriptorsForDispatch(*this);
	}

	m_d3dCommandList->Dispatch(numGroupsX, numGroupsY, numGroupsZ);

}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void CommandList::DispatchRays(ID3D12StateObject* rtPSO, ID3D12Resource* tlas, D3D12_DISPATCH_RAYS_DESC const& dispatchRaysDesc)
{
// 	for(int i = 0; i < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES; ++i)
// 	{
// 		m_dynamicDescriptorHeap[i]->CommitStagedDescriptorsForDispatch(*this);
// 	}

	m_d3dCommandList->SetComputeRootShaderResourceView(static_cast<UINT>(GlobalRTRootSignatureParameters::ACCELERATION_STRUCTURE), tlas->GetGPUVirtualAddress());
	m_d3dCommandList->SetPipelineState1(rtPSO);
	m_d3dCommandList->DispatchRays(&dispatchRaysDesc);
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
bool CommandList::Close(CommandList* pendingCommandList)
{
	FlushResourceBarriers();
	
	m_d3dCommandList->Close();
	uint32_t numPendingBarriers = m_resourceStateTracker->FlushPendingResourceBarriers(pendingCommandList);
	m_resourceStateTracker->CommitFinalResourceStates();

	return numPendingBarriers > 0;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void CommandList::Close()
{
	FlushResourceBarriers();
	m_d3dCommandList->Close();
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void CommandList::Reset()
{
	HRESULT hr;

	hr = m_d3dCommandAllocator->Reset();
	if(FAILED(hr))
	{
		ERROR_AND_DIE("Command Allocator Reset Failed");
	}

	hr = m_d3dCommandList->Reset(m_d3dCommandAllocator, nullptr);
	if(FAILED(hr))
	{
		ERROR_AND_DIE("Command List Reset Failed");
	}

	m_resourceStateTracker->Reset();
	m_uploadBuffer->Reset();

	ReleaseTrackedObjects();

	for(int i = 0; i < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES; ++i)
	{
		m_dynamicDescriptorHeap[i]->Reset();
		m_boundDescriptorHeaps[i] = nullptr;
	}

	m_rootSignature = nullptr;
	m_computeCommandList = nullptr;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void CommandList::ReleaseTrackedObjects()
{
	for(size_t objIndex = 0; objIndex < m_trackedObjects.size(); ++objIndex)
	{
		if(m_trackedObjects[objIndex])
		{
			DX_SAFE_RELEASE(m_trackedObjects[objIndex])
		}
	}
}

//------------------------------------------------------------------------------------------------------------------
void CommandList::SetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE heapType, ID3D12DescriptorHeap* heap)
{
	if(m_boundDescriptorHeaps[heapType] != heap)
	{
		m_boundDescriptorHeaps[heapType] = heap;
		BindDescriptorHeaps();
	}
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void CommandList::TrackObject(ID3D12Object* object)
{
	m_trackedObjects.push_back(object);
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void CommandList::BindDescriptorHeaps()
{
	UINT numDescriptHeaps = 0;
	ID3D12DescriptorHeap* descriptorHeaps[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES] = {};

	for(uint32_t i = 0; i < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES; ++i)
	{
		ID3D12DescriptorHeap* descriptorHeap = m_boundDescriptorHeaps[i];
		if(descriptorHeap)
		{
			descriptorHeaps[numDescriptHeaps++] = descriptorHeap;
		}
	}

	m_d3dCommandList->SetDescriptorHeaps(numDescriptHeaps, descriptorHeaps);

}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
// D3DX12 Helpers
size_t CommandList::UpdateSubresources(ID3D12Resource* desitnationResource, ID3D12Resource* intermediateResource, UINT64 intermediateOffSet,
										unsigned int firstSubresource, unsigned int numSubresources, D3D12_SUBRESOURCE_DATA* srcData)
{
	UINT64 requiredSize = 0;
	UINT64 memoryToAllocate = static_cast<UINT64>(sizeof(D3D12_PLACED_SUBRESOURCE_FOOTPRINT) + sizeof(unsigned int) + sizeof(UINT64)) * numSubresources;

	if(memoryToAllocate > SIZE_MAX)
	{
		return 0;
	}
	void* allocatedHeapMemoryStartLocation = HeapAlloc(GetProcessHeap(), 0, static_cast<SIZE_T>(memoryToAllocate));

	if(allocatedHeapMemoryStartLocation == NULL)
	{
		return 0;
	}

	D3D12_PLACED_SUBRESOURCE_FOOTPRINT* pointerToFootprintArrayStart = reinterpret_cast<D3D12_PLACED_SUBRESOURCE_FOOTPRINT*>(allocatedHeapMemoryStartLocation);
	UINT64* pointerToRowSizeArrayStart = reinterpret_cast<UINT64*>(pointerToFootprintArrayStart + numSubresources);
	unsigned int* pointerToNumRowsPerFootprintStart = reinterpret_cast<unsigned int*>(pointerToRowSizeArrayStart + numSubresources);

	D3D12_RESOURCE_DESC desc = desitnationResource->GetDesc();

	m_device->GetCopyableFootprints(&desc, firstSubresource, numSubresources, intermediateOffSet, pointerToFootprintArrayStart, pointerToNumRowsPerFootprintStart,
									  pointerToRowSizeArrayStart, &requiredSize);

	UINT64 result = UpdateSubresources(desitnationResource, intermediateResource, firstSubresource, numSubresources, requiredSize,
													 pointerToFootprintArrayStart, pointerToNumRowsPerFootprintStart, pointerToRowSizeArrayStart, srcData);

	HeapFree(GetProcessHeap(), 0, allocatedHeapMemoryStartLocation);
	return static_cast<size_t>(result);
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
UINT64 CommandList::UpdateSubresources(ID3D12Resource* destinationResource, ID3D12Resource* intermediateResource, unsigned int firstSubresource,
										unsigned int numSubresources, UINT64 requiredSize, D3D12_PLACED_SUBRESOURCE_FOOTPRINT const* layouts, unsigned int const* numRows, UINT64 const* rowSizesInBytes,
										D3D12_SUBRESOURCE_DATA const* srcData)
{
	D3D12_RESOURCE_DESC intermediateDesc = intermediateResource->GetDesc();
	D3D12_RESOURCE_DESC destinationDesc = destinationResource->GetDesc();
	if((intermediateDesc.Dimension != D3D12_RESOURCE_DIMENSION_BUFFER) || (intermediateDesc.Width < requiredSize + layouts[0].Offset) || (requiredSize > (SIZE_T)-1) ||
	   ((destinationDesc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER) && (firstSubresource != 0 || numSubresources != 1)))
	{
		return 0;
	}

	BYTE* data;
	HRESULT hr = intermediateResource->Map(0, NULL, reinterpret_cast<void**>(&data));
	if(FAILED(hr))
	{
		return 0;
	}

	for(unsigned int i = 0; i < numSubresources; ++i)
	{
		if(rowSizesInBytes[i] > (SIZE_T)-1) return 0;
		D3D12_MEMCPY_DEST destinationData = {data + layouts[i].Offset, layouts[i].Footprint.RowPitch, layouts[i].Footprint.RowPitch * numRows[i]};
		MemcpySubresource(&destinationData, &srcData[i], (SIZE_T)rowSizesInBytes[i], numRows[i], layouts[i].Footprint.Depth);
	}
	intermediateResource->Unmap(0, NULL);

	if(destinationDesc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER)
	{
		D3D12_BOX srcBox = {};
		srcBox.left = static_cast<unsigned int>(layouts[0].Offset);
		srcBox.right = static_cast<unsigned int>(layouts[0].Offset + layouts[0].Footprint.Width);
		srcBox.top = 0;
		srcBox.bottom = 1;
		srcBox.front = 0;
		srcBox.back = 1;

		m_d3dCommandList->CopyBufferRegion(destinationResource, 0, intermediateResource, layouts[0].Offset, layouts[0].Footprint.Width);
	}
	else
	{
		for(unsigned int i = 0; i < numSubresources; ++i)
		{
			D3D12_TEXTURE_COPY_LOCATION dst = {};
			dst.pResource = destinationResource;
			dst.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
			dst.SubresourceIndex = i + firstSubresource;

			D3D12_TEXTURE_COPY_LOCATION src = {};
			src.pResource = intermediateResource;
			src.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
			src.PlacedFootprint = layouts[i];
			m_d3dCommandList->CopyTextureRegion(&dst, 0, 0, 0, &src, nullptr);
		}
	}
	return requiredSize;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void CommandList::MemcpySubresource(D3D12_MEMCPY_DEST const* destination, D3D12_SUBRESOURCE_DATA const* subresource, size_t rowSizeInBytes, unsigned int numRows, unsigned int numSlices)
{
	for(unsigned int z = 0; z < numSlices; ++z)
	{
		BYTE* pDestSlice = reinterpret_cast<BYTE*>(destination->pData) + destination->SlicePitch * z;
		const BYTE* pSrcSlice = reinterpret_cast<const BYTE*>(subresource->pData) + subresource->SlicePitch * z;
		for(unsigned int y = 0; y < numRows; ++y)
		{
			memcpy(pDestSlice + destination->RowPitch * y,
				   pSrcSlice + subresource->RowPitch * y,
				   rowSizeInBytes);
		}
	}
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
UINT64 CommandList::GetRequiredIntermediateSize(ID3D12Resource* destinationResource, UINT firstSubresource, UINT numSubresource)
{
	D3D12_RESOURCE_DESC desc = destinationResource->GetDesc();
	UINT64 reqSize = 0;

	m_device->GetCopyableFootprints(&desc, firstSubresource, numSubresource, 0, nullptr, nullptr, nullptr, &reqSize);

	return reqSize;
}
#endif
