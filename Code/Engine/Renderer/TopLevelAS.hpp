#pragma once
#include <d3d12.h>

#include <vector>
#include <cstdint>

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
class VertexBuffer;
class IndexBuffer;
class CommandList;
class BottomLevelAS;

struct Mat44;
//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
class TopLevelAS
{
	friend class DX12Renderer;
	friend class CommandList;

private:
	TopLevelAS(ID3D12Device10* device = nullptr, D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS buildFlag = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_NONE);


	void		Build(CommandList* commandList);

public:
	~TopLevelAS();
	void		AddInstance(BottomLevelAS* blas);

private:
	ID3D12Resource* m_tlasResource			= nullptr;
	ID3D12Resource* m_scratchResource		= nullptr;
	ID3D12Resource*	m_instanceDescBuffer	= nullptr;

	ID3D12Device10* m_rtDevice = nullptr;

	std::vector<D3D12_RAYTRACING_INSTANCE_DESC> m_blasInstanceDescs;
	std::vector<BottomLevelAS*> m_sceneBLASes;

	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS	m_buildFlag = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_NONE;

	uint64_t m_resultDataSize = 0;
	uint64_t m_scratchDataSize = 0;
};