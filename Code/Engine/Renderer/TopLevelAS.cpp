#include "Engine/Renderer/TopLevelAS.hpp"
#include "Engine/Renderer/CommandList.hpp"
#include "Engine/Renderer/BottomLevelAS.hpp"

#include "Engine/Core/EngineCommon.hpp"

#include "Engine/Math/Mat44.hpp"
#include "Engine/Math/MathUtils.hpp"

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
TopLevelAS::TopLevelAS(ID3D12Device10* device, D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS buildFlag)
	: m_rtDevice(device)
	, m_buildFlag(buildFlag)
{}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
TopLevelAS::~TopLevelAS()
{
	if(m_scratchResource)
	{
		DX_SAFE_RELEASE(m_scratchResource);
	}

	if(m_instanceDescBuffer)
	{
		DX_SAFE_RELEASE(m_instanceDescBuffer);
	}

	if(m_tlasResource)
	{
		DX_SAFE_RELEASE(m_tlasResource);
	}

}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void TopLevelAS::AddInstance(BottomLevelAS* blas)
{	
	D3D12_RAYTRACING_INSTANCE_DESC instanceDesc = {};
	instanceDesc.InstanceContributionToHitGroupIndex = 0;
	instanceDesc.InstanceMask					= 0xFF;
	instanceDesc.Transform[0][0] = instanceDesc.Transform[1][1] = instanceDesc.Transform[2][2] = 1;
	instanceDesc.AccelerationStructure = blas->GetGPUAddress();
	instanceDesc.Flags = D3D12_RAYTRACING_INSTANCE_FLAG_NONE;

	m_blasInstanceDescs.push_back(instanceDesc);
	m_sceneBLASes.push_back(blas);
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void TopLevelAS::Build(CommandList* commandList)
{
	m_instanceDescBuffer = commandList->CreateUploadResource(m_blasInstanceDescs.data(), m_blasInstanceDescs.size() * sizeof(D3D12_RAYTRACING_INSTANCE_DESC), L"TLAS Instance Desc Buffer");

	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS asInputs = {};
	asInputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
	asInputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
	asInputs.NumDescs = static_cast<unsigned int>(m_blasInstanceDescs.size());
	asInputs.InstanceDescs = m_instanceDescBuffer->GetGPUVirtualAddress();
	asInputs.Flags = m_buildFlag;

	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO preBuildInfo = {};
	m_rtDevice->GetRaytracingAccelerationStructurePrebuildInfo(&asInputs, &preBuildInfo);

	m_scratchDataSize = AlignUp(preBuildInfo.ScratchDataSizeInBytes, static_cast<UINT64>(D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT));
	m_resultDataSize = AlignUp(preBuildInfo.ResultDataMaxSizeInBytes, static_cast<UINT64>(D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT));

	m_scratchResource = commandList->CreateUAVBufferResource(m_scratchDataSize, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	m_scratchResource->SetName(L"TLAS Scratch Resource");

	m_tlasResource = commandList->CreateAccelerationStructure(m_resultDataSize, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, L"TLAS");

	// Accel Struct Build
	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC buildDesc = {};
	buildDesc.Inputs = asInputs;
	buildDesc.ScratchAccelerationStructureData	= m_scratchResource->GetGPUVirtualAddress();
	buildDesc.DestAccelerationStructureData		= m_tlasResource->GetGPUVirtualAddress();

	commandList->GetCommandList()->BuildRaytracingAccelerationStructure(&buildDesc, 0, nullptr);

	commandList->UAVBarrier(m_tlasResource, true);
}
