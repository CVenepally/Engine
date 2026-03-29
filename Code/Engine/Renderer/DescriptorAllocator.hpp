#pragma once
#include <cstdint>
#include <mutex>
#include <set>
#include <vector>
#include <d3d12.h>

//------------------------------------------------------------------------------------------------------------------
class DescriptorAllocationResult;
class DescriptorAllocatorPage;
class CommandList;

typedef std::vector<DescriptorAllocatorPage*> DescriptorAllocatorPagePool;
//------------------------------------------------------------------------------------------------------------------

/*
    Manages DescriptorAllocatorPages (wrapper for ID3D12DescriptorHeap) and Descriptors in general.
    Managed/Owned by command lists. Allocates descriptors when requested and frees them for reuse after they are used.
 */
class DescriptorAllocator
{
    friend class CommandList;
    friend class DX12Renderer;

private:
    explicit DescriptorAllocator(ID3D12Device10* device, D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t numDescriptorsPerHeap = 256);
    virtual ~DescriptorAllocator();

    /*
     Used to allocate Descriptors in a contiguous block from/in a Descriptor Heap (DescriptorAllocatorPage).
     New page or heap is created or requested if no available page can satisfy the request.
     */
    DescriptorAllocationResult  Allocate(uint32_t numDescriptors = 1);
    void                        ReleaseStaleDescriptors(uint64_t frameNumber);
    DescriptorAllocatorPage*    CreateAllocatorPage();
    
private:
    D3D12_DESCRIPTOR_HEAP_TYPE      m_heapType;
    uint32_t                        m_numDescriptorsPerHeap;
    DescriptorAllocatorPagePool     m_heapPool;
    std::set<size_t>                m_availableHeapsIndices;
    std::mutex                      m_allocationMutex;
    ID3D12Device10*                 m_device = nullptr;
};