#pragma once
#include <cstdint>

//------------------------------------------------------------------------------------------------------------------
struct D3D12_CPU_DESCRIPTOR_HANDLE;

//------------------------------------------------------------------------------------------------------------------
class DescriptorAllocationResult
{
    friend class DescriptorAllocator;
    friend class DescriptorAllocatorPage;

public:
    DescriptorAllocationResult();
    DescriptorAllocationResult(D3D12_CPU_DESCRIPTOR_HANDLE descriptorCPUHandle, uint32_t numHandles, uint32_t descriptorSize, DescriptorAllocatorPage* page);
    ~DescriptorAllocationResult();

    DescriptorAllocationResult(const DescriptorAllocationResult&)               = delete;
    DescriptorAllocationResult& operator=(const DescriptorAllocationResult&)    = delete;
    DescriptorAllocationResult(DescriptorAllocationResult&& otherAllocationResult) noexcept;
    DescriptorAllocationResult& operator=(DescriptorAllocationResult&& otherAllocationResult) noexcept;
    
    bool                        IsNull() const;
    D3D12_CPU_DESCRIPTOR_HANDLE GetDescriptorCPUHandle(uint32_t offset = 0) const;
    uint32_t                    GetNumHandles() const;
    DescriptorAllocatorPage*    GetDescriptorAllocatorPage() const;

    // Free the descriptor back to the heap it came from
    void                        Free();

private:
    size_t                      m_cpuDescriptorHandle;
    uint32_t                    m_numHandles;
    uint32_t                    m_descriptorSize;
    DescriptorAllocatorPage*    m_descriptorAllocatorPage;
};