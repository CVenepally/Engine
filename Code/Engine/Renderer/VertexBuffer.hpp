#pragma once
#include "Game/EngineBuildPreferences.hpp"


#if defined(USING_DX12)
#include <d3d12.h>
#include <mutex>
#include <unordered_map>
#include "Engine/Renderer/DescriptorAllocationResult.hpp"
#endif

//------------------------------------------------------------------------------------------------------------------
struct ID3D11Device;
struct ID3D11Buffer;

//------------------------------------------------------------------------------------------------------------------
class VertexBuffer
{
	friend class Renderer;
	friend class DX12Renderer;
	friend class CommandList;

public:
#if defined USING_DX12

	VertexBuffer();

	ID3D12Resource*					GetBufferResource() const {return m_buffer;}
	void							SetD3DResource(ID3D12Resource* bufferResource);
	void							CreateVertexBufferView(size_t numElements, size_t vertexStride);
	D3D12_GPU_VIRTUAL_ADDRESS		GetGPUVirtualAddress() const;

	int								GetBindlessIndex() const;

	DescriptorAllocationResult		CreateShaderResourceView(D3D12_SHADER_RESOURCE_VIEW_DESC const* srvDesc) const;
	D3D12_CPU_DESCRIPTOR_HANDLE		GetSRV(D3D12_SHADER_RESOURCE_VIEW_DESC const* srvDesc) const;

#else
	VertexBuffer(ID3D11Device* device, unsigned int size, unsigned int stride);
#endif

	VertexBuffer(const VertexBuffer& copy) = delete;
	virtual ~VertexBuffer();

	void Create();
	void Resize(unsigned int size);

	unsigned int GetSize();
	unsigned int GetStride();

private:
#if defined USING_DX12
	ID3D12Device10*				m_device	= nullptr;
	ID3D12Resource*				m_buffer	= nullptr;
	D3D12_VERTEX_BUFFER_VIEW	m_vbView;

	size_t						m_numElements		= 0;
	size_t						m_sizeOfEachElement	= 0;

	uint32_t					m_bindlessIndex = 0;

	mutable std::unordered_map<size_t, DescriptorAllocationResult>	m_shaderResourceViews;
	mutable std::mutex												m_shaderResourceViewsMutex;


#else 
	ID3D11Device* m_device = nullptr;
	ID3D11Buffer* m_buffer = nullptr;

	unsigned int m_size = 0;
	unsigned int m_stride = 0;
#endif
};