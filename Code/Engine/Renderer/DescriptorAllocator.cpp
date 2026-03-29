#include "Engine/Renderer/DescriptorAllocator.hpp"
#include "Engine/Renderer/DescriptorAllocationResult.hpp"
#include "Engine/Renderer/DescriptorAllocatorPage.hpp"
#include "Engine/Renderer/CommandList.hpp"

//------------------------------------------------------------------------------------------------------------------
DescriptorAllocator::DescriptorAllocator(ID3D12Device10* device, D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t numDescriptorsPerHeap)
    : m_heapType(type)
    , m_numDescriptorsPerHeap(numDescriptorsPerHeap)
    , m_device(device)
{
}

//------------------------------------------------------------------------------------------------------------------
DescriptorAllocator::~DescriptorAllocator()
{
    for(DescriptorAllocatorPage* page : m_heapPool)
    {
        delete page;
    }
}

//------------------------------------------------------------------------------------------------------------------
DescriptorAllocationResult DescriptorAllocator::Allocate(uint32_t numDescriptors)
{
    std::lock_guard<std::mutex> lock(m_allocationMutex);
    
    DescriptorAllocationResult allocationResult;

    for (auto iter = m_availableHeapsIndices.begin(); iter != m_availableHeapsIndices.end(); iter++)
    {
        DescriptorAllocatorPage* allocatorPage = m_heapPool[*iter];

        allocationResult = allocatorPage->Allocate(numDescriptors);

        if (allocatorPage->NumFreeHandles() == 0)
        {
            iter = m_availableHeapsIndices.erase(iter);
        }

        if (!allocationResult.IsNull())
        {
            break;
        }
    }

    if(allocationResult.IsNull())
    {
        m_numDescriptorsPerHeap = m_numDescriptorsPerHeap > numDescriptors ? m_numDescriptorsPerHeap : numDescriptors;
        DescriptorAllocatorPage* newPage = CreateAllocatorPage();

        allocationResult = newPage->Allocate(numDescriptors);
    }
    
    return allocationResult;
}

//------------------------------------------------------------------------------------------------------------------
void DescriptorAllocator::ReleaseStaleDescriptors(uint64_t frameNumber)
{
    std::lock_guard<std::mutex> lock(m_allocationMutex);

    for (size_t i = 0; i < m_heapPool.size(); i++)
    {
        DescriptorAllocatorPage* allocatorPage = m_heapPool[i];

        allocatorPage->ReleaseStaleDescriptors(frameNumber);

        if (allocatorPage->NumFreeHandles() > 0)
        {
            m_availableHeapsIndices.insert(i);
        }
    }
}

//------------------------------------------------------------------------------------------------------------------
DescriptorAllocatorPage* DescriptorAllocator::CreateAllocatorPage()
{
    DescriptorAllocatorPage* newPage = new DescriptorAllocatorPage(m_device, m_heapType, m_numDescriptorsPerHeap);

    m_heapPool.emplace_back(newPage);
    m_availableHeapsIndices.insert(m_heapPool.size() - 1);

    return newPage;
}
