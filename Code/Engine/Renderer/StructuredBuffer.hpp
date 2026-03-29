#pragma once
#include "Game/EngineBuildPreferences.hpp"
#if defined USING_DX12
#include <cstdint>
#include <d3d12.h>
#include <mutex>
#include <unordered_map>
#include "Engine/Renderer/DescriptorAllocationResult.hpp"

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
struct ID3D12Resource;
struct ID3D12Device10;

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
// For now, this is only for Bindless.
class StructuredBuffer
{
	friend class DX12Renderer;
	friend class CommandList;

private:
				StructuredBuffer(ID3D12Device10* device);

public:
	virtual		~StructuredBuffer();
	unsigned int GetBindlessIndex() const {return m_bindlessIndex;}
	DescriptorAllocationResult CreateUnorderedAccessView(D3D12_UNORDERED_ACCESS_VIEW_DESC const* uavDesc) const;
	D3D12_CPU_DESCRIPTOR_HANDLE GetUAV(D3D12_UNORDERED_ACCESS_VIEW_DESC const* srvDesc) const;

private:
	ID3D12Device10*		m_device			= nullptr;
	ID3D12Resource*		m_buffer			= nullptr;
	
	size_t				m_numElements		= 0;
	size_t				m_sizeOfEachElement = 0;
	
	uint32_t			m_bindlessIndex		= 0;

	mutable std::unordered_map<size_t, DescriptorAllocationResult> m_unorderedAccessViews;
	mutable std::mutex m_unorderedAccessViewsMutex;

};
#endif