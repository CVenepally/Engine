#pragma once
#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Renderer/RendererUtils.hpp"
#include "Engine/Renderer/RenderTarget.hpp"

#include <d3d12.h>
#include <vector>
#include <map>
#include <string>
#include <mutex>
#include <cstdint>

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
class UploadBuffer;
class ResourceStateTracker;
class DynamicDescriptorHeap;
class VertexBuffer;
class IndexBuffer;
class MaterialBuffer;
class PipelineStateObject;
class RootSignature;
class Texture;
class Image;
class RenderTarget;
class ShaderTable;

struct Vertex_PCU;
struct Vertex_PCUTBN;
struct Rgba8;
struct Viewport;
struct IntVec2;

typedef std::vector<ID3D12Object*> TrackedObjects;

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
class CommandList
{
	friend class DX12Renderer;
	friend class CommandQueue;
	friend class GBuffer;

private:
				CommandList() = delete;
	explicit	CommandList(CommandQueue* commandQueue, ID3D12Device10* device, D3D12_COMMAND_LIST_TYPE type);
	virtual		~CommandList();

public:
	ID3D12Device10*					GetDevice() const;
	ID3D12GraphicsCommandList7*		GetCommandList() const;
	D3D12_COMMAND_LIST_TYPE			GetCommandListType() const;

	//Resource State Tracker-----------------------------------------------
	void							TransitionBarrier(ID3D12Resource* resource, D3D12_RESOURCE_STATES stateAfter, UINT subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
													  bool flushBarriers = false);
	void							UAVBarrier(ID3D12Resource* resource, bool flushBarriers = false);
	void							AliasingBarrier(ID3D12Resource* resourceBefore, ID3D12Resource* resourceAfter, bool flushBarriers = false);
	void							FlushResourceBarriers();
	void							CopyResource(ID3D12Resource* destinationResource, ID3D12Resource* sourceResource);
	void							ResolveSubresources(ID3D12Resource* destinationResource, ID3D12Resource* sourceResource, uint32_t dstSubresource = 0, uint32_t srcSubresource = 0);
	
	//Buffer Operations----------------------------------------------------
	void							CopyVertexBuffer(VertexBuffer* vertexBuffer, size_t numVertices, size_t vertexStride, const void* vertexBufferData);
	template<typename T>
	void							CopyVertexBuffer(VertexBuffer* vertexBuffer, std::vector<T> const& vertexBufferData)
	{
		CopyVertexBuffer(vertexBuffer, vertexBufferData.size(), sizeof(T), vertexBufferData.data());
	}

	void							CopyIndexBuffer(IndexBuffer* indexBuffer, size_t numIndexes, DXGI_FORMAT indexFormat, const void* indexBufferData);
	template<typename T>
	void							CopyIndexBuffer(IndexBuffer* indexBuffer, std::vector<T> const& indexBufferData)
	{
		GUARANTEE_OR_DIE(sizeof(T) == 2 || sizeof(T) == 4, "Index Buffer type size if larger than 4.");

		DXGI_FORMAT indexFormat = (sizeof(T) == 2) ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT;
		CopyIndexBuffer(indexBuffer, indexBufferData.size(), indexFormat, indexBufferData.data());
	}

	ID3D12Resource*					CreateBufferResource(size_t numElements, size_t elementSize, const void* bufferData, D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE);
	ID3D12Resource*					CreateUAVBufferResource(UINT64 bufferSize, D3D12_RESOURCE_STATES initialResourceState = D3D12_RESOURCE_STATE_COMMON);
	ID3D12Resource*					CreateAccelerationStructure(UINT64 bufferSize, D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, std::wstring const& debugName = L"AccelerationStructure");
	ID3D12Resource*					CreateUploadResource(void* uploadData, size_t uploadDataSize, std::wstring const& name = L"UploadResource");

	void							SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY primitiveTopology);

	//Set Buffers On Command Lists-----------------------------------------
	void							SetGraphicsDynamicConstantBuffer(uint32_t rootParameterIndex, size_t sizeInBytes, const void* bufferData);
	template<typename T>
	void							SetGraphicsDynamicConstantBuffer(uint32_t rootParameterIndex, T const& data)
	{
		SetGraphicsDynamicConstantBuffer(rootParameterIndex, sizeof(T), &data);
	}

	void							SetComputeDynamicConstantBuffer(uint32_t rootParameterIndex, size_t sizeInBytes, const void* bufferData);
	template<typename T>
	void							SetComputeDynamicConstantBuffer(uint32_t rootParameterIndex, T const& data)
	{
		SetComputeDynamicConstantBuffer(rootParameterIndex, sizeof(T), &data);
	}


	void							SetGraphics32BitConstants(uint32_t rootParameterIndex, uint32_t numConstants, const void* constants);
	template<typename T>
	void							SetGraphics32BitConstants(uint32_t rootParameterIndex, T const& constants)
	{
		GUARANTEE_OR_DIE(sizeof(T) % sizeof(uint32_t) == 0, "Size of type must be a multiple of 4 Bytes")
		SetGraphics32BitConstants(rootParameterIndex, sizeof(T) / sizeof(uint32_t), &constants);
	}

	void							SetCompute32BitConstants(uint32_t rootParameterIndex, uint32_t numConstants, const void* constants);
	template<typename T>
	void							SetCompute32BitConstants(uint32_t rootParameterIndex, T const& constants)
	{
		GUARANTEE_OR_DIE(sizeof(T) % sizeof(uint32_t) == 0, "Size of type must be a multiple of 4 Bytes")
		SetCompute32BitConstants(rootParameterIndex, sizeof(T) / sizeof(uint32_t), &constants);
	}

	void							SetVertexBuffer(uint32_t slot, VertexBuffer const& vertexBuffer);
	void							SetStructuredVertexBuffer(uint32_t slot, VertexBuffer const& vertexBuffer);

	void							SetDynamicVertexBuffer(uint32_t slot, size_t numVertices, size_t vertexSize, const void* vertexBufferData);
	template<typename T>
	void							SetDynamicVertexBuffer(uint32_t slot, std::vector<T>& vertexBufferData)
	{
		SetDynamicVertexBuffer(slot, vertexBufferData.size(), sizeof(T), vertexBufferData.data());
	}

	void							SetIndexBuffer(IndexBuffer const& indexBuffer);
	void							SetStructuredIndexBuffer(uint32_t slot, IndexBuffer const& IndexBuffer);

	void							SetDynamicIndexBuffer(size_t numIndices, DXGI_FORMAT indexFormat, const void* indexBufferData);
	template<typename T>
	void							SetDynamicIndexBuffer(std::vector<T> const& indexBufferData)
	{
		if constexpr((sizeof(T) != 2) || (sizeof(T) != 4))
		{
	//		ERROR_AND_DIE("Indexes can only be 16-bit numbers or 32-bit numbers; CommandList::SetDynamicIndexBuffer<T>");
		}

		DXGI_FORMAT indexFormat;
		if constexpr(sizeof(T) == 2)
		{
			indexFormat = DXGI_FORMAT_R16_UINT;
		}
		else
		{
			indexFormat = DXGI_FORMAT_R32_UINT;
		}

		SetDynamicIndexBuffer(indexBufferData.size(), indexFormat, indexBufferData.data());
	}

	//Set Viewports and Scissor Rects--------------------------------------
	void							SetViewport(D3D12_VIEWPORT const& viewport);
	void							SetViewports(std::vector<D3D12_VIEWPORT> const& viewports);

	void							SetScissorRect(D3D12_RECT const& scissorRect);
	void							SetScissorRects(std::vector<D3D12_RECT> const& scissorRects);

	void							SetPipelineState(PipelineStateObject const& pipelineState);
	void							SetGraphicsRootSignature(RootSignature const& rootSignature);
	void							SetComputeRootSignature(RootSignature const& rootSignature);

	//Textures-------------------------------------------------------------
	Texture*						GetTexture(std::string const& textureFilePath);
	Texture*						LoadTextureFromFile(std::string const& textureFilePath);
	Texture*						CreateTextureFromImage(Image const& image, std::string const& name = "");
	void							CopyTextureSubresource(ID3D12Resource** texture, uint32_t firstSubresource, uint32_t numSubresources, D3D12_SUBRESOURCE_DATA* subresourceData);

	void							SetTextureShaderResourceView(uint32_t rootParameterIndex, uint32_t descriptorOffset, Texture* texture, 
																 D3D12_RESOURCE_STATES stateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, 
																 UINT firstSubresource = 0, UINT numSubresources = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES, D3D12_SHADER_RESOURCE_VIEW_DESC* srv = nullptr);

	void							SetTextureUAV(uint32_t rootParameterIndex, uint32_t descriptorOffset, Texture* texture, D3D12_UNORDERED_ACCESS_VIEW_DESC* uav = nullptr);


	void							SetRenderTarget(RenderTarget const& renderTarget);

	void							ClearRenderTargetTexture(Texture* texture, Rgba8 const& clearColor);
	void							ClearDepthStencilTexture(Texture* texture, D3D12_CLEAR_FLAGS flag = D3D12_CLEAR_FLAG_DEPTH, float depth = 1.f, uint8_t stencil = 0);

	void							Draw(uint32_t vertexCount, uint32_t instanceCount = 1, uint32_t startVertex = 0, uint32_t startInstance = 0);
	void							DrawIndexed(uint32_t indexCount, uint32_t instanceCount = 1, uint32_t startIndex = 0, int32_t baseVertex = 0, uint32_t startInstance = 0);
	void							Dispatch(uint32_t numGroupsX, uint32_t numGroupsY = 1, uint32_t numGroupsZ = 1);
	void							DispatchRays(ID3D12StateObject* rtPSO, ID3D12Resource* tlas, D3D12_DISPATCH_RAYS_DESC const& dispatchRaysDesc);

	bool							Close(CommandList* pendingCommandList);
	void							Close();
	void							Reset();
	void							ReleaseTrackedObjects();
	void							SetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE heapType, ID3D12DescriptorHeap* heap);
	void							TrackObject(ID3D12Object* object);

private:
	void							BindDescriptorHeaps();
	
	//D3DX12 Helpers-------------------------------------------------------
	size_t							UpdateSubresources(ID3D12Resource* desitnationResource, ID3D12Resource* intermediateResource, UINT64 intermediateOffSet,
												   unsigned int firstSubresource, unsigned int numSubresources, D3D12_SUBRESOURCE_DATA* srcData);

	UINT64							UpdateSubresources(ID3D12Resource* desitnationResource, ID3D12Resource* intermediateResource, unsigned int firstSubresource,
												   unsigned int numSubresources, UINT64 requiredSize, D3D12_PLACED_SUBRESOURCE_FOOTPRINT const* layouts, unsigned int const* numRows,
													   UINT64 const* rowSizesInBytes, D3D12_SUBRESOURCE_DATA const* srcData);

	void							MemcpySubresource(D3D12_MEMCPY_DEST const* destination, D3D12_SUBRESOURCE_DATA const* subresource, size_t rowSizeInBytes, unsigned int numRows, unsigned int numSlices);

	UINT64							GetRequiredIntermediateSize(ID3D12Resource* pDestinationResource, UINT FirstSubresource, UINT NumSubresources);

private:
	ID3D12Device10*					m_device				= nullptr;
	CommandQueue*					m_commandQueue			= nullptr;

	ID3D12GraphicsCommandList7*		m_d3dCommandList		= nullptr;
	D3D12_COMMAND_LIST_TYPE			m_commandListType		= D3D12_COMMAND_LIST_TYPE_DIRECT;
	ID3D12CommandAllocator*			m_d3dCommandAllocator	= nullptr;

	CommandList*					m_computeCommandList	= nullptr;

	ID3D12RootSignature*			m_rootSignature			= nullptr;
	
	UploadBuffer*					m_uploadBuffer			= nullptr;

	ResourceStateTracker*			m_resourceStateTracker	= nullptr;

	DynamicDescriptorHeap*			m_dynamicDescriptorHeap[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES];

	ID3D12DescriptorHeap*			m_boundDescriptorHeaps[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES];

	TrackedObjects					m_trackedObjects;

	static std::vector<Texture*>	s_textureCache;
	static std::mutex				s_textureCacheMutex;

};