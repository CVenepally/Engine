#pragma once
#include <d3d12.h>

#include <vector>
#include <cstdint>

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
class VertexBuffer;
class IndexBuffer;
class CommandList;

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
class BottomLevelAS
{
	friend class DX12Renderer;
	friend class CommandList;

private:
	BottomLevelAS(ID3D12Device10* device, D3D12_RAYTRACING_GEOMETRY_TYPE geometryType = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES, 
				D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS buildFlag = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_NONE);


public:
	~BottomLevelAS();
	void						AddGeometry(VertexBuffer* vbo, IndexBuffer* ibo);
	D3D12_GPU_VIRTUAL_ADDRESS	GetGPUAddress();

private:
	void		Build(CommandList* commandList);

private:
	ID3D12Resource*		m_blasResource		= nullptr;
	ID3D12Resource*		m_scratchResource	= nullptr; 

	ID3D12Device10*		m_rtDevice			= nullptr;

	D3D12_RAYTRACING_GEOMETRY_TYPE						m_geometryType	= D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS	m_buildFlag		= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_NONE;

	// One BLAS only has one geometry type. 
	//But can have multiple geometry descs and it is better to have multiple
	std::vector<D3D12_RAYTRACING_GEOMETRY_DESC> m_geometryDescs;
	
	uint64_t m_resultDataSize = 0; 
	uint64_t m_scratchDataSize = 0; 
};