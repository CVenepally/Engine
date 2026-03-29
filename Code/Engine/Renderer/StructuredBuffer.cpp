#include "Engine/Renderer/StructuredBuffer.hpp"
#if defined USING_DX12
#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Renderer/DX12Renderer.hpp"
#include <d3d12.h>

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
StructuredBuffer::StructuredBuffer(ID3D12Device10* device)
	: m_device(device)
{

}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
StructuredBuffer::~StructuredBuffer()
{
	DX_SAFE_RELEASE(m_buffer);
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
DescriptorAllocationResult StructuredBuffer::CreateUnorderedAccessView(D3D12_UNORDERED_ACCESS_VIEW_DESC const* uavDesc) const
{
	DescriptorAllocationResult uav = DX12Renderer::AllocateDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	m_device->CreateUnorderedAccessView(m_buffer, nullptr, uavDesc, uav.GetDescriptorCPUHandle());

	return uav;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
D3D12_CPU_DESCRIPTOR_HANDLE StructuredBuffer::GetUAV(D3D12_UNORDERED_ACCESS_VIEW_DESC const* uavDesc) const
{
	std::size_t hash = 0;

	if(uavDesc)
	{
		hash = std::hash<D3D12_UNORDERED_ACCESS_VIEW_DESC>{}(*uavDesc);
	}

	std::lock_guard<std::mutex> guard(m_unorderedAccessViewsMutex);

	auto iter = m_unorderedAccessViews.find(hash);
	if(iter == m_unorderedAccessViews.end())
	{
		auto uav = CreateUnorderedAccessView(uavDesc);
		iter = m_unorderedAccessViews.insert({hash, std::move(uav)}).first;
	}

	return iter->second.GetDescriptorCPUHandle();
}
#endif