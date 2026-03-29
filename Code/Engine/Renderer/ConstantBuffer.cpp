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

#include "Engine/Renderer/ConstantBuffer.hpp"
#include "Engine/Core/EngineCommon.hpp"

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
#if defined(USING_DX12)
ConstantBuffer::ConstantBuffer(ID3D12Device10* device, unsigned int size)
	: m_device(device)
	, m_size(size)
{
	m_alignedSize = (size + 255) & ~255;
	Create();
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void ConstantBuffer::Create()
{
	HRESULT hr;

	D3D12_HEAP_PROPERTIES heapProps = {};
	heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;

	D3D12_RESOURCE_DESC bufferDesc = {};
	bufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	bufferDesc.Width = m_alignedSize;
	bufferDesc.Height = 1;
	bufferDesc.DepthOrArraySize = 1;
	bufferDesc.MipLevels = 1;
	bufferDesc.Format = DXGI_FORMAT_UNKNOWN;
	bufferDesc.SampleDesc.Count = 1;
	bufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

	hr = m_device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &bufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&m_buffer));

	if(FAILED(hr))
	{
		ERROR_AND_DIE("Failed to create constant buffer");
	}

	m_buffer->SetName(L"Constant Buffer");

	hr = m_buffer->Map(0, nullptr, &m_mappedData);

	if(FAILED(hr))
	{
		ERROR_AND_DIE("Failed to map constant buffer");
	}

	m_gpuAddress = m_buffer->GetGPUVirtualAddress();
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void ConstantBuffer::CopyData(const void* data)
{
	memcpy(m_mappedData, data, m_size);
}

#else

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
ConstantBuffer::ConstantBuffer(ID3D11Device* device, unsigned int size)
	: m_device(device)
	, m_size(size)
{
	Create();
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void ConstantBuffer::Create()
{
	HRESULT hr;

	D3D11_BUFFER_DESC bufferDesc = {};
	bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	bufferDesc.ByteWidth = m_size;
	bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

	hr = m_device->CreateBuffer(&bufferDesc, nullptr, &m_buffer);

	if(!SUCCEEDED(hr))
	{
		ERROR_AND_DIE("Could not create constant buffer");
	}
}

#endif

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
ConstantBuffer::~ConstantBuffer()
{
	DX_SAFE_RELEASE(m_buffer);
}


//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
unsigned int ConstantBuffer::GetSize()
{
	return m_size;
}