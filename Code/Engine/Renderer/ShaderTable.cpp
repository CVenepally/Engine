#include "Engine/Renderer/ShaderTable.hpp"
#include "Engine/Renderer/ShaderRecord.hpp"
#include "Engine/Renderer/ResourceStateTracker.hpp"

#include "Engine/Math/MathUtils.hpp"

#include "Engine/Core/StringUtils.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Core/EngineCommon.hpp"

#include "ThirdParty/d3dx12/d3dx12.h"

#include <d3d12.h>

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
ShaderTable::ShaderTable()
{
	m_resource = nullptr;
	m_mappedShaderRecords = nullptr;
	m_shaderRecordSize = 0;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
ShaderTable::ShaderTable(ID3D12Device10 * device, unsigned int numShaderRecords, int shaderRecordSize, std::wstring const& shaderTableName)
{
	m_shaderRecordSize = AlignUp(shaderRecordSize, D3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT);	
	m_shaderRecords.reserve(numShaderRecords);

	unsigned int bufferSize = numShaderRecords * m_shaderRecordSize;
	
	CD3DX12_HEAP_PROPERTIES heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);

	CD3DX12_RESOURCE_DESC resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(bufferSize);
	
	HRESULT hr = device->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &resourceDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&m_resource));

	if(FAILED(hr))
	{
		ERROR_AND_DIE(Stringf("Failed to create Shader Binding Table, %s", shaderTableName.c_str()));
	}

	m_resource->SetName(shaderTableName.c_str());

	ResourceStateTracker::AddGlobalResourceState(m_resource, D3D12_RESOURCE_STATE_GENERIC_READ);


	CD3DX12_RANGE readRange(0, 0);

	hr = m_resource->Map(0, &readRange, reinterpret_cast<void**>(&m_mappedShaderRecords));

	if(FAILED(hr))
	{
		ERROR_AND_DIE(Stringf("Failed to Map data to resource (SBT), %s", shaderTableName.c_str()));
	}
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
ShaderTable::~ShaderTable()
{
	DX_SAFE_RELEASE(m_resource);
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void ShaderTable::AddShaderRecord(ShaderRecord const& shaderRecord)
{
	if(m_shaderRecords.size() >= m_shaderRecords.capacity())
	{
		return;
	}

	m_shaderRecords.push_back(shaderRecord);
	shaderRecord.CopyTo(m_mappedShaderRecords);
	m_mappedShaderRecords += m_shaderRecordSize;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
unsigned int ShaderTable::GetShaderRecordSize()
{
	return m_shaderRecordSize;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
uint64_t ShaderTable::GetGPUVirtualAddress()
{
	return m_resource->GetGPUVirtualAddress();
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
D3D12_RESOURCE_DESC ShaderTable::GetDesc()
{
	return m_resource->GetDesc();
}
