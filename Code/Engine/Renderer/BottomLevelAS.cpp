#include "Engine/Renderer/BottomLevelAS.hpp"
#include "Engine/Renderer/IndexBuffer.hpp"
#include "Engine/Renderer/VertexBuffer.hpp"
#include "Engine/Renderer/CommandList.hpp"

#include "Engine/Math/MathUtils.hpp"

#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Renderer/TopLevelAS.hpp"

#if defined(USING_DX12)
//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
BottomLevelAS::BottomLevelAS(ID3D12Device10* device, D3D12_RAYTRACING_GEOMETRY_TYPE geometryType, D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS buildFlag)
	: m_geometryType(geometryType)
	, m_buildFlag(buildFlag)
	, m_rtDevice(device)
{}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
BottomLevelAS::~BottomLevelAS()
{
	if(m_scratchResource)
	{
		DX_SAFE_RELEASE(m_scratchResource);
	}

	if(m_blasResource)
	{
		DX_SAFE_RELEASE(m_blasResource);
	}
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void BottomLevelAS::AddGeometry(VertexBuffer* vbo, IndexBuffer* ibo)
{
	D3D12_RAYTRACING_GEOMETRY_DESC geometryDesc = {};
	geometryDesc.Type = m_geometryType;
	geometryDesc.Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;

	if(m_geometryType == D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES)
	{
		geometryDesc.Triangles.Transform3x4					= 0;
		geometryDesc.Triangles.IndexFormat					= ibo->GetIndexFormat();
		geometryDesc.Triangles.IndexBuffer					= ibo->GetBufferResource()->GetGPUVirtualAddress();
		geometryDesc.Triangles.IndexCount					= ibo->GetSize();
		geometryDesc.Triangles.VertexBuffer.StartAddress	= vbo->GetBufferResource()->GetGPUVirtualAddress();
		geometryDesc.Triangles.VertexBuffer.StrideInBytes	= vbo->GetStride();
		geometryDesc.Triangles.VertexCount					= vbo->GetSize();
		geometryDesc.Triangles.VertexFormat					= DXGI_FORMAT_R32G32B32_FLOAT;
	}

	m_geometryDescs.push_back(geometryDesc);
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
D3D12_GPU_VIRTUAL_ADDRESS BottomLevelAS::GetGPUAddress()
{
	return m_blasResource->GetGPUVirtualAddress();
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void BottomLevelAS::Build(CommandList* commandList)
{	
	// Build Inputs
	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS asInputs = {};
	asInputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
	asInputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
	asInputs.NumDescs = static_cast<unsigned int>(m_geometryDescs.size());
	asInputs.pGeometryDescs = m_geometryDescs.data();
	asInputs.Flags = m_buildFlag;

	// Prebuild Info for Sizes
	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO preBuildInfo = {};
	m_rtDevice->GetRaytracingAccelerationStructurePrebuildInfo(&asInputs, &preBuildInfo);

	m_scratchDataSize	= AlignUp(preBuildInfo.ScratchDataSizeInBytes, static_cast<UINT64>(D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT));
	m_resultDataSize	= AlignUp(preBuildInfo.ResultDataMaxSizeInBytes, static_cast<UINT64>(D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT));

	// Scratch and BLAS Resource Creation
	m_scratchResource = commandList->CreateUAVBufferResource(m_scratchDataSize, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	m_scratchResource->SetName(L"Blas Scratch Resource");
	
	m_blasResource = commandList->CreateAccelerationStructure(m_resultDataSize, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, L"BLAS");

	// Accel Struct Build
	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC buildDesc = {};
	buildDesc.Inputs = asInputs;
	buildDesc.ScratchAccelerationStructureData = m_scratchResource->GetGPUVirtualAddress();
	buildDesc.DestAccelerationStructureData = m_blasResource->GetGPUVirtualAddress();
	
	commandList->GetCommandList()->BuildRaytracingAccelerationStructure(&buildDesc, 0, nullptr);

	commandList->UAVBarrier(m_blasResource, true);
}
#endif