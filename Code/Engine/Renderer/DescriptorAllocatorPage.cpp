#include "Engine/Renderer/DescriptorAllocatorPage.hpp"
#include "Engine/Renderer/DescriptorAllocationResult.hpp"
#include "Engine/Renderer/DescriptorAllocator.hpp"
#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"
#include <d3d12.h>
//------------------------------------------------------------------------------------------------------------------
DescriptorAllocatorPage::DescriptorAllocatorPage(ID3D12Device10* device, D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t numDescriptorsInHeap)
    : m_heapType(type)
    , m_numDescriptorsInHeap(numDescriptorsInHeap)
{
    D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
    heapDesc.Type                       = m_heapType;
    heapDesc.NumDescriptors             = m_numDescriptorsInHeap;

    HRESULT hr = device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&m_descriptorHeap));

    if (FAILED(hr))
    {
        ERROR_AND_DIE("Failed to create descriptor heap")
    }

    m_descriptorHeap->SetName(L"Descriptor Allocator Page: Descriptor Heap");

    m_cpuHandle = m_descriptorHeap->GetCPUDescriptorHandleForHeapStart().ptr;
    m_descriptorHandleIncrementSize = device->GetDescriptorHandleIncrementSize(m_heapType);
    m_numFreeHandles = m_numDescriptorsInHeap;

    AddNewBlock(0, m_numFreeHandles);
}

//------------------------------------------------------------------------------------------------------------------
DescriptorAllocatorPage::~DescriptorAllocatorPage()
{
    DX_SAFE_RELEASE(m_descriptorHeap)
}

//------------------------------------------------------------------------------------------------------------------
D3D12_DESCRIPTOR_HEAP_TYPE DescriptorAllocatorPage::GetHeapType() const
{
    return m_heapType;
}

//------------------------------------------------------------------------------------------------------------------
DescriptorAllocationResult DescriptorAllocatorPage::Allocate(uint32_t numDescriptors)
{
    std::lock_guard<std::mutex> lock(m_allocationMutex);
    if (numDescriptors > m_numFreeHandles)
    {
        return {};
    }

    // lower_bound takes in a "key". Returns the first value of the key. If the value does not exist, if there is a different key that is greater than the required key,
    // it returns the first value of that greater key. If there is no key greater than the one we need, it returns end()
    auto smallestBlockIterator = m_freeListBySize.lower_bound(numDescriptors);
    if (smallestBlockIterator == m_freeListBySize.end())
    {
        return {};
    }

    auto blockSize = smallestBlockIterator->first;
    auto offsetIter = smallestBlockIterator->second;
    auto offset = offsetIter->first;

    m_freeListBySize.erase(smallestBlockIterator);
    m_freeListByOffset.erase(offsetIter);

    auto newOffset = offset + numDescriptors;
    auto newSize = blockSize - numDescriptors;

    if (newSize > 0)
    {
        AddNewBlock(newOffset, newSize);
    }

    m_numFreeHandles -= numDescriptors;
    D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = {};
    cpuHandle.ptr = static_cast<size_t>(m_cpuHandle + static_cast<size_t>(offset) * m_descriptorHandleIncrementSize);
    return {cpuHandle, numDescriptors, m_descriptorHandleIncrementSize, this};
}

//------------------------------------------------------------------------------------------------------------------
uint32_t DescriptorAllocatorPage::NumFreeHandles() const
{
    return m_numFreeHandles;
}

//------------------------------------------------------------------------------------------------------------------
bool DescriptorAllocatorPage::HasSpace(uint32_t numDescriptors) const
{
    return m_freeListBySize.lower_bound(numDescriptors) != m_freeListBySize.end();
}

//------------------------------------------------------------------------------------------------------------------
void DescriptorAllocatorPage::Free(DescriptorAllocationResult&& allocationResult, uint64_t frameNumber)
{
    auto offset = ComputeOffset(allocationResult.GetDescriptorCPUHandle());

    std::lock_guard<std::mutex> lock(m_allocationMutex);

    m_staleDescriptors.emplace(offset, allocationResult.GetNumHandles(), frameNumber);
}

//------------------------------------------------------------------------------------------------------------------
void DescriptorAllocatorPage::ReleaseStaleDescriptors(uint64_t frameNumber)
{
    std::lock_guard<std::mutex> lock(m_allocationMutex);
    while (!m_staleDescriptors.empty() && m_staleDescriptors.front().m_frameNum <= frameNumber)
    {
        auto& staleDescriptor = m_staleDescriptors.front();
        auto offset = staleDescriptor.m_offset;
        auto numDescriptors = staleDescriptor.m_size;

        FreeBlock(offset, numDescriptors);

        m_staleDescriptors.pop();
    }
}

//------------------------------------------------------------------------------------------------------------------
uint32_t DescriptorAllocatorPage::ComputeOffset(D3D12_CPU_DESCRIPTOR_HANDLE handle)
{
    return static_cast<uint32_t>(handle.ptr - m_cpuHandle);
}

//------------------------------------------------------------------------------------------------------------------
void DescriptorAllocatorPage::AddNewBlock(uint32_t offset, uint32_t numDescriptors)
{
    auto offSetIter = m_freeListByOffset.emplace(offset, numDescriptors);
    auto sizeIter = m_freeListBySize.emplace(numDescriptors, offSetIter.first);
    offSetIter.first->second.m_freeListBySizeIter = sizeIter;
}

//------------------------------------------------------------------------------------------------------------------
void DescriptorAllocatorPage::FreeBlock(uint32_t offset, uint32_t numDescriptors)
{
    // Block that would be freed after the current block being freed
    auto nextBlockIter = m_freeListByOffset.upper_bound(offset);

    auto prevBlockIter = nextBlockIter;

    if (prevBlockIter != m_freeListByOffset.begin())
    {
        --prevBlockIter;
    }
    else
    {
        prevBlockIter = m_freeListByOffset.end();
    }

    m_numFreeHandles += numDescriptors;

    if (prevBlockIter != m_freeListByOffset.end() && offset == prevBlockIter->first + prevBlockIter->second.m_size)
    {
        // The previous block is exactly behind the block that is to be freed.
        //
        // PrevBlock.Offset           Offset
        // |                          |
        // |<-----PrevBlock.Size----->|<------Size-------->|
        //
        // Increase the block size by the size of merging with the previous block.
        
        offset = prevBlockIter->first;
        numDescriptors += prevBlockIter->second.m_size;

        m_freeListBySize.erase(prevBlockIter->second.m_freeListBySizeIter);
        m_freeListByOffset.erase(prevBlockIter);
    }
    //                                                                          offset
    if (nextBlockIter != m_freeListByOffset.end() && offset + numDescriptors == nextBlockIter->first)
    {
        // The next block is exactly in front of the block that is to be freed.
        //
        // Offset               NextBlock.Offset 
        // |                    |
        // |<------Size-------->|<-----NextBlock.Size----->|
        // Increase the block size by the size of merging with the next block.

        numDescriptors += nextBlockIter->second.m_size;
        m_freeListBySize.erase(nextBlockIter->second.m_freeListBySizeIter);
        m_freeListByOffset.erase(nextBlockIter);
    }

    AddNewBlock(offset, numDescriptors);
}
