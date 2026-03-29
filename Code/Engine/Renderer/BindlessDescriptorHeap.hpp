#pragma once
#include "Engine/Core/EngineCommon.hpp"
//#if defined(USING_DX12)
#include <d3d12.h>
#include <cstdint>
#include <vector>
#include <mutex>
#include <atomic>
#include <string>

// From DXRPathTracer Sample by TheRealMJP

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
struct PersistentDescriptorAllocation
{
	D3D12_CPU_DESCRIPTOR_HANDLE m_handles[3] = {};
	uint32_t					m_index = 0;
};

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
struct TemporaryDescriptorAllocation
{
	D3D12_CPU_DESCRIPTOR_HANDLE m_cpuHandle = {};
	D3D12_GPU_DESCRIPTOR_HANDLE m_gpuHandle = {};
	uint32_t					m_index = 0;
};

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
class BindlessDescriptorHeap
{
	friend class DX12Renderer;
public:
	
	BindlessDescriptorHeap(ID3D12Device* device, int numPersistent, int numTemporary, D3D12_DESCRIPTOR_HEAP_TYPE heapType, bool shaderVisible, std::wstring debugName);
	virtual ~BindlessDescriptorHeap();

	void							Reset();
	void							Reset(D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle, D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle);

	PersistentDescriptorAllocation	AllocatePersistent();
	void							FreePersistent(uint32_t& index);
	void							FreePersistent(D3D12_CPU_DESCRIPTOR_HANDLE& cpuHandle);
	void							FreePersistent(D3D12_GPU_DESCRIPTOR_HANDLE& gpuHandle);
	
	TemporaryDescriptorAllocation	AllocateTemporary(uint32_t count);
	void							AllocateTemporary(D3D12_CPU_DESCRIPTOR_HANDLE* cpuHandle, D3D12_GPU_DESCRIPTOR_HANDLE* gpuHandle);

	D3D12_CPU_DESCRIPTOR_HANDLE		CPUHandleFromIndex(uint32_t descriptorIdx) const;
	D3D12_GPU_DESCRIPTOR_HANDLE		GPUHandleFromIndex(uint32_t descriptorIdx) const;
	
	D3D12_CPU_DESCRIPTOR_HANDLE		CPUHandleFromIndex(uint32_t descriptorIdx, uint64_t heapIndex) const;
	D3D12_GPU_DESCRIPTOR_HANDLE		GPUHandleFromIndex(uint32_t descriptorIdx, uint64_t heapIndex) const;

	uint32_t						IndexFromHandle(D3D12_CPU_DESCRIPTOR_HANDLE cpuhandle);
	uint32_t						IndexFromHandle(D3D12_GPU_DESCRIPTOR_HANDLE gpuhandle);

	ID3D12DescriptorHeap*			CurrentHeap() const;
	
	uint32_t						TotalNumDescriptors() const;

public:
	ID3D12DescriptorHeap*			m_bindlessHeaps[3] = {};

	uint32_t						m_maxPersistentAllocations	= 0;
	uint32_t						m_numPersistentAllocations	= 0;
	std::vector<uint32_t>			m_deadList;
	uint32_t						m_maxTemporaryAllocations	= 0;
	std::atomic<int64_t>			m_numTemporaryAllocations	= 0;

	uint32_t						m_heapIndex					= 0;
	uint32_t						m_numHeaps					= 0;
	uint32_t						m_descriptorSize			= 0;
	bool							m_shaderVisible				= false;
	D3D12_DESCRIPTOR_HEAP_TYPE		m_heapType					= D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	D3D12_CPU_DESCRIPTOR_HANDLE		m_cpuStart[3]				= {};
	D3D12_GPU_DESCRIPTOR_HANDLE		m_gpuStart[3]				= {};

	std::mutex						m_heapMutex;
};

//#endif