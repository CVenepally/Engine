#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <dxgi.h>

#include <dxgi1_6.h>
#include <d3d12sdklayers.h>

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "dxguid.lib")

#include "Engine/Renderer/IndexBuffer.hpp"
#include "Engine/Renderer/DX12Renderer.hpp"
#include "Engine/Core/EngineCommon.hpp"

#if defined(USING_DX12)

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
IndexBuffer::IndexBuffer()
{}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
IndexBuffer::~IndexBuffer()
{
	DX_SAFE_RELEASE(m_buffer);
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
unsigned int IndexBuffer::GetSize()
{
	return static_cast<unsigned int>(m_numElements);
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void IndexBuffer::SetD3DResource(ID3D12Resource* bufferResource)
{
	m_buffer = bufferResource;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void IndexBuffer::CreateIndexBufferView(size_t numElements, size_t elementSize)
{
	GUARANTEE_OR_DIE((elementSize == 2 || elementSize == 4),  "Indexes must be 16 or 32 bit integers");
	
	m_numElements = numElements;
	m_sizeOfEachElement = elementSize;
	m_indexFormat = (m_sizeOfEachElement == 2) ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT;

	m_ibView = {};
	m_ibView.BufferLocation = m_buffer->GetGPUVirtualAddress();
	m_ibView.SizeInBytes	= static_cast<UINT>(m_numElements * m_sizeOfEachElement);
	m_ibView.Format			= m_indexFormat;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
D3D12_GPU_VIRTUAL_ADDRESS IndexBuffer::GetGPUVirtualAddress() const
{
	return m_buffer->GetGPUVirtualAddress();
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
DXGI_FORMAT IndexBuffer::GetIndexFormat() const
{
	return m_indexFormat;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
DescriptorAllocationResult IndexBuffer::CreateShaderResourceView(const D3D12_SHADER_RESOURCE_VIEW_DESC* srvDesc) const
{
	DescriptorAllocationResult srv = DX12Renderer::AllocateDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	m_device->CreateShaderResourceView(m_buffer, srvDesc, srv.GetDescriptorCPUHandle());

	return srv;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
D3D12_CPU_DESCRIPTOR_HANDLE IndexBuffer::GetSRV(D3D12_SHADER_RESOURCE_VIEW_DESC const* srvDesc) const
{
	std::size_t hash = 0;
	if(srvDesc)
	{
		hash = std::hash<D3D12_SHADER_RESOURCE_VIEW_DESC>{}(*srvDesc);
	}

	std::lock_guard<std::mutex> lock(m_shaderResourceViewsMutex);

	auto iter = m_shaderResourceViews.find(hash);
	if(iter == m_shaderResourceViews.end())
	{
		auto srv = CreateShaderResourceView(srvDesc);
		iter = m_shaderResourceViews.insert({hash, std::move(srv)}).first;
	}

	return iter->second.GetDescriptorCPUHandle();
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
#else

IndexBuffer::IndexBuffer(ID3D11Device* device, unsigned int size, unsigned int stride)
	: m_size(size)
	, m_stride(stride)
	, m_device(device)
{
	Create();
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
IndexBuffer::~IndexBuffer()
{
	DX_SAFE_RELEASE(m_buffer);
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void IndexBuffer::Create()
{

	HRESULT hr;

	D3D11_BUFFER_DESC bufferDesc = {};
	bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	bufferDesc.ByteWidth = m_size;
	bufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

	hr = m_device->CreateBuffer(&bufferDesc, nullptr, &m_buffer);

	if(!SUCCEEDED(hr))
	{
		ERROR_AND_DIE("Could not create index buffer");
	}

}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------s
void IndexBuffer::Resize(unsigned int size)
{

	DX_SAFE_RELEASE(m_buffer);
	m_size = size;
	Create();
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
unsigned int IndexBuffer::GetSize()
{
	return m_size;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
unsigned int IndexBuffer::GetStride()
{
	return m_stride;
}
#endif