#include "Engine/Renderer/BindlessDescriptorHeap.hpp"

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
BindlessDescriptorHeap::BindlessDescriptorHeap(ID3D12Device* device, int numPersistent, int numTemporary, D3D12_DESCRIPTOR_HEAP_TYPE heapType, bool shaderVisible, std::wstring debugName)
{
	uint32_t totalNumDescriptors = numPersistent + numTemporary;
	m_maxPersistentAllocations = numPersistent;
	m_maxTemporaryAllocations = numTemporary;
	m_heapType = heapType;
	m_shaderVisible = shaderVisible;

	m_numHeaps = m_shaderVisible ? 3 : 1;

	for(uint32_t i = 0; i < m_maxPersistentAllocations; ++i)
	{
		m_deadList.push_back(i);
	}

	D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
	heapDesc.NumDescriptors = totalNumDescriptors;
	heapDesc.Type			= m_heapType;
	heapDesc.Flags			= D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	
	HRESULT hr;

	for(uint32_t i = 0; i < m_numHeaps; ++i)
	{
		hr = device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&m_bindlessHeaps[i]));
		
		if(FAILED(hr))
		{
			ERROR_AND_DIE("Failed to create a Bindless Heap");
		}

		std::wstring name = debugName+ L" " + std::to_wstring(i);

		m_bindlessHeaps[i]->SetName(name.c_str());

		m_cpuStart[i] = m_bindlessHeaps[i]->GetCPUDescriptorHandleForHeapStart();

		if(m_shaderVisible)
		{
			m_gpuStart[i] = m_bindlessHeaps[i]->GetGPUDescriptorHandleForHeapStart();
		}
	}

	m_descriptorSize = device->GetDescriptorHandleIncrementSize(heapType);
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
BindlessDescriptorHeap::~BindlessDescriptorHeap()
{
	for(uint32_t i = 0; i < m_numHeaps; ++i)
	{
		if(m_bindlessHeaps[i])
		{
			DX_SAFE_RELEASE(m_bindlessHeaps[i]);
		}
	}
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void BindlessDescriptorHeap::Reset()
{
	m_numTemporaryAllocations = 0;
	m_heapIndex = (m_heapIndex + 1) % m_numHeaps;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void BindlessDescriptorHeap::Reset(D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle, D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle)
{
	UNUSED(cpuHandle)
	UNUSED(gpuHandle)

	m_numTemporaryAllocations = 0;
	m_heapIndex = (m_heapIndex + 1) % m_numHeaps;

}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
PersistentDescriptorAllocation BindlessDescriptorHeap::AllocatePersistent()
{
	std::scoped_lock lock(m_heapMutex);

	if(m_numPersistentAllocations >= m_maxPersistentAllocations)
	{
		return PersistentDescriptorAllocation();
	}

	uint32_t index = m_deadList[m_numPersistentAllocations];
	m_numPersistentAllocations+= 1;

	PersistentDescriptorAllocation allocation;
	allocation.m_index = index;

	for(uint32_t i = 0; i < m_numHeaps; ++i)
	{
		allocation.m_handles[i] = m_cpuStart[i];
		allocation.m_handles[i].ptr += index * m_descriptorSize;
	}

	return allocation;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void BindlessDescriptorHeap::FreePersistent(uint32_t& index)
{
	std::scoped_lock lock(m_heapMutex);

	if(m_numPersistentAllocations < 0)
	{
		return;
	}

	m_deadList[m_numPersistentAllocations - 1] = index;
	m_numPersistentAllocations -= 1;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void BindlessDescriptorHeap::FreePersistent(D3D12_CPU_DESCRIPTOR_HANDLE & cpuHandle)
{
	UNUSED(cpuHandle)
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void BindlessDescriptorHeap::FreePersistent(D3D12_GPU_DESCRIPTOR_HANDLE & gpuHandle)
{
	UNUSED(gpuHandle)
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
TemporaryDescriptorAllocation BindlessDescriptorHeap::AllocateTemporary(uint32_t count)
{
	std::scoped_lock lock(m_heapMutex);

	if(count < 0)
	{
		return TemporaryDescriptorAllocation();
	}

	uint32_t tempIdx = static_cast<uint32_t>(m_numTemporaryAllocations.fetch_add(count, std::memory_order_relaxed));

	if(tempIdx >= m_maxTemporaryAllocations)
	{
		ERROR_AND_DIE("Temporary Allocations Exceed Max");
	}
	
	uint32_t finalIndex = tempIdx + m_maxPersistentAllocations;

	TemporaryDescriptorAllocation allocation;
	allocation.m_cpuHandle		= m_cpuStart[m_heapIndex];
	allocation.m_cpuHandle.ptr += finalIndex * m_descriptorSize;
	allocation.m_gpuHandle		= m_gpuStart[m_heapIndex];
	allocation.m_gpuHandle.ptr += finalIndex * m_descriptorSize;
	allocation.m_index			= finalIndex;

	return allocation;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void BindlessDescriptorHeap::AllocateTemporary(D3D12_CPU_DESCRIPTOR_HANDLE* cpuHandle, D3D12_GPU_DESCRIPTOR_HANDLE* gpuHandle)
{
	std::scoped_lock lock(m_heapMutex);
	uint32_t count = 1;

	uint32_t tempIdx = static_cast<uint32_t>(m_numTemporaryAllocations.fetch_add(count, std::memory_order_relaxed));

	if(tempIdx >= m_maxTemporaryAllocations)
	{
		ERROR_AND_DIE("Temporary Allocations Exceed Max");
	}

	uint32_t finalIndex = tempIdx + m_maxPersistentAllocations;
	
	cpuHandle		= &m_cpuStart[m_heapIndex];
	cpuHandle->ptr	+= finalIndex * m_descriptorSize;
	gpuHandle		= &m_gpuStart[m_heapIndex];
	gpuHandle->ptr += finalIndex * m_descriptorSize;
	
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
D3D12_CPU_DESCRIPTOR_HANDLE BindlessDescriptorHeap::CPUHandleFromIndex(uint32_t descriptorIdx) const
{
	return CPUHandleFromIndex(descriptorIdx, m_heapIndex);
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
D3D12_GPU_DESCRIPTOR_HANDLE BindlessDescriptorHeap::GPUHandleFromIndex(uint32_t descriptorIdx) const
{
	return GPUHandleFromIndex(descriptorIdx, m_heapIndex);
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
D3D12_CPU_DESCRIPTOR_HANDLE BindlessDescriptorHeap::CPUHandleFromIndex(uint32_t descriptorIdx, uint64_t heapIndex) const
{
	if(descriptorIdx >= TotalNumDescriptors())
	{
		ERROR_AND_DIE("Descriptor Index Higher than total num descriptors")
	}

	D3D12_CPU_DESCRIPTOR_HANDLE handle = m_cpuStart[heapIndex];
	handle.ptr += descriptorIdx * m_descriptorSize;

	return handle;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
D3D12_GPU_DESCRIPTOR_HANDLE BindlessDescriptorHeap::GPUHandleFromIndex(uint32_t descriptorIdx, uint64_t heapIndex) const
{
	if(descriptorIdx >= TotalNumDescriptors())
	{
		ERROR_AND_DIE("Descriptor Index Higher than total num descriptors")
	}

	D3D12_GPU_DESCRIPTOR_HANDLE handle = m_gpuStart[heapIndex];
	handle.ptr += descriptorIdx * m_descriptorSize;

	return handle;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
uint32_t BindlessDescriptorHeap::IndexFromHandle(D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle)
{
	if(cpuHandle.ptr < m_cpuStart[m_heapIndex].ptr)
	{
		ERROR_AND_DIE("Invalid Handle - IndexFromHandle (CPU)")
	}

	if(cpuHandle.ptr >= m_cpuStart[m_heapIndex].ptr + m_descriptorSize * TotalNumDescriptors())
	{
		ERROR_AND_DIE("Invalid Handle - IndexFromHandle (CPU)")
	}

	if((cpuHandle.ptr - m_cpuStart[m_heapIndex].ptr) % m_descriptorSize != 0)
	{
		ERROR_AND_DIE("Invalid Handle - IndexFromHandle (CPU)")
	}

	return uint32_t(cpuHandle.ptr - m_cpuStart[m_heapIndex].ptr) / m_descriptorSize;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
uint32_t BindlessDescriptorHeap::IndexFromHandle(D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle)
{
	if(gpuHandle.ptr < m_gpuStart[m_heapIndex].ptr)
	{
		ERROR_AND_DIE("Invalid Handle - IndexFromHandle (GPU)")
	}

	if(gpuHandle.ptr >= m_gpuStart[m_heapIndex].ptr + m_descriptorSize * TotalNumDescriptors())
	{
		ERROR_AND_DIE("Invalid Handle - IndexFromHandle (GPU)")
	}

	if((gpuHandle.ptr - m_gpuStart[m_heapIndex].ptr) % m_descriptorSize != 0)
	{
		ERROR_AND_DIE("Invalid Handle - IndexFromHandle (GPU)")
	}

	return uint32_t(gpuHandle.ptr - m_gpuStart[m_heapIndex].ptr) / m_descriptorSize;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
ID3D12DescriptorHeap* BindlessDescriptorHeap::CurrentHeap() const
{
	return m_bindlessHeaps[m_heapIndex];
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
uint32_t BindlessDescriptorHeap::TotalNumDescriptors() const
{
	return m_maxPersistentAllocations + m_maxTemporaryAllocations;
}
