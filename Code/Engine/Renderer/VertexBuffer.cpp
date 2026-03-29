#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <dxgi.h>

#include <d3d12.h>
#include <dxgi1_6.h>
#include <d3d12sdklayers.h>

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "dxguid.lib")

#include "Engine/Renderer/VertexBuffer.hpp"
#include "Engine/Renderer/DX12Renderer.hpp"
#include "Engine/Core/Vertex_PCU.hpp"
#include "Engine/Core/EngineCommon.hpp"

#if defined USING_DX12

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
VertexBuffer::VertexBuffer()
{
	
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
VertexBuffer::~VertexBuffer()
{
	DX_SAFE_RELEASE(m_buffer);
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void VertexBuffer::Resize(unsigned int size)
{
	// #ToDo: Rewrite to support the new system
	DX_SAFE_RELEASE(m_buffer);
	m_numElements = size;
}

//------------------------------------------------------------------------------------------------------------------
unsigned int VertexBuffer::GetSize()
{
	return static_cast<unsigned int>(m_numElements);
}

//------------------------------------------------------------------------------------------------------------------
unsigned int VertexBuffer::GetStride()
{
	return static_cast<unsigned int>(m_sizeOfEachElement);
}


//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void VertexBuffer::SetD3DResource(ID3D12Resource* bufferRersource)
{
	m_buffer = bufferRersource;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void VertexBuffer::CreateVertexBufferView(size_t numElements, size_t vertexStride)
{
	m_numElements				= numElements;
	m_sizeOfEachElement			= vertexStride;

	m_vbView = {};
	m_vbView.BufferLocation		= m_buffer->GetGPUVirtualAddress();
	m_vbView.SizeInBytes		= static_cast<UINT>(m_numElements * m_sizeOfEachElement);
	m_vbView.StrideInBytes		= static_cast<UINT>(m_sizeOfEachElement);
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
D3D12_GPU_VIRTUAL_ADDRESS VertexBuffer::GetGPUVirtualAddress() const
{
	return m_buffer->GetGPUVirtualAddress();
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
int VertexBuffer::GetBindlessIndex() const
{
	return m_bindlessIndex;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
DescriptorAllocationResult VertexBuffer::CreateShaderResourceView(const D3D12_SHADER_RESOURCE_VIEW_DESC* srvDesc) const
{
	DescriptorAllocationResult srv = DX12Renderer::AllocateDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	m_device->CreateShaderResourceView(m_buffer, srvDesc, srv.GetDescriptorCPUHandle());

	return srv;
}


//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
D3D12_CPU_DESCRIPTOR_HANDLE VertexBuffer::GetSRV(D3D12_SHADER_RESOURCE_VIEW_DESC const* srvDesc) const
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


#else
//------------------------------------------------------------------------------------------------------------------

VertexBuffer::VertexBuffer(ID3D11Device* device, unsigned int size, unsigned int stride)
	: m_device(device)
	, m_size(size)
	, m_stride(stride)
{
	Create();
}

//------------------------------------------------------------------------------------------------------------------
VertexBuffer::~VertexBuffer()
{

	DX_SAFE_RELEASE(m_buffer);

}

//------------------------------------------------------------------------------------------------------------------
void VertexBuffer::Create()
{
	HRESULT hr;

	D3D11_BUFFER_DESC bufferDesc = {};
	bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	bufferDesc.ByteWidth = m_size;
	bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

	hr = m_device->CreateBuffer(&bufferDesc, nullptr, &m_buffer);

	if(!SUCCEEDED(hr))
	{
		ERROR_AND_DIE("Could not create vertex buffer");
	}
}

//------------------------------------------------------------------------------------------------------------------
void VertexBuffer::Resize(unsigned int size)
{

	DX_SAFE_RELEASE(m_buffer);
	m_size = size;
	Create();

}

//------------------------------------------------------------------------------------------------------------------
unsigned int VertexBuffer::GetSize()
{
	return m_size;
}

//------------------------------------------------------------------------------------------------------------------
unsigned int VertexBuffer::GetStride() 
{
	return m_stride;
}
#endif


