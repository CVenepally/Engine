#pragma once
#include <string>
#include <vector>

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
struct ID3D12Device10;
struct ID3D12Resource;
struct D3D12_RESOURCE_DESC;

class ShaderRecord;

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
class ShaderTable
{
	friend class DX12Renderer;
	friend class CommandList;

private:
	ShaderTable();
	explicit ShaderTable(ID3D12Device10* device, unsigned int numShaderRecords, int shaderRecordSize, std::wstring const& shaderTableName = L"Shader Table");
	virtual ~ShaderTable();

	void						AddShaderRecord(ShaderRecord const& shaderRecord);
	unsigned int				GetShaderRecordSize();
	uint64_t					GetGPUVirtualAddress();
	D3D12_RESOURCE_DESC			GetDesc();

private:
	
	ID3D12Resource*				m_resource = nullptr;
	uint8_t*					m_mappedShaderRecords;
	unsigned int				m_shaderRecordSize;
	
	std::vector<ShaderRecord>	m_shaderRecords;
};