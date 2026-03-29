#pragma once
#include <cstdint>
#include <map>
#include <queue>
#include <mutex>
#include <d3d12.h>

//------------------------------------------------------------------------------------------------------------------
class   DescriptorAllocationResult;

struct  D3D12_CPU_DESCRIPTOR_HANDLE;
struct  FreeBlockInfo;
struct  ID3D12DescriptorHeap;
struct  ID3D12Device10;

//------------------------------------------------------------------------------------------------------------------
typedef std::map<uint32_t, FreeBlockInfo> FreeListByOffset;
typedef std::multimap<uint32_t, FreeListByOffset::iterator> FreeListBySize;

struct FreeBlockInfo
{
    friend class DescriptorAllocatorPage;
    
public:
    FreeBlockInfo(uint32_t size)
        :m_size(size)
    {}

    uint32_t m_size;
    FreeListBySize::iterator m_freeListBySizeIter;
};

//------------------------------------------------------------------------------------------------------------------
struct StaleDescriptorInfo
{
    friend class DescriptorAllocatorPage;

public:
    StaleDescriptorInfo(uint32_t offset, uint32_t size, uint64_t frame)
        : m_offset(offset)
        , m_size(size)
        , m_frameNum(frame)
    {}
    
private:
    uint32_t m_offset;
    uint32_t m_size;
    uint64_t m_frameNum;
};

typedef std::queue<StaleDescriptorInfo> StaleDescriptorQueue;

//------------------------------------------------------------------------------------------------------------------
class DescriptorAllocatorPage
{
    friend class DescriptorAllocator;
    friend class DescriptorAllocationResult;

private:
    explicit DescriptorAllocatorPage(ID3D12Device10* device, D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t numDescriptorsInHeap = 256);
    virtual ~DescriptorAllocatorPage();

    D3D12_DESCRIPTOR_HEAP_TYPE  GetHeapType() const;
    DescriptorAllocationResult  Allocate(uint32_t numDescriptors = 1);
    uint32_t                    NumFreeHandles() const;
    bool                        HasSpace(uint32_t numDescriptors) const;
    void                        Free(DescriptorAllocationResult&& allocationResult, uint64_t frameNumber);
    void                        ReleaseStaleDescriptors(uint64_t frameNumber);
    uint32_t                    ComputeOffset(D3D12_CPU_DESCRIPTOR_HANDLE handle);
    void                        AddNewBlock(uint32_t offset, uint32_t numDescriptors);
    void                        FreeBlock(uint32_t offset, uint32_t numDescriptors);
    
private:
    ID3D12DescriptorHeap*       m_descriptorHeap;
    D3D12_DESCRIPTOR_HEAP_TYPE  m_heapType;
    size_t                      m_cpuHandle;
    uint32_t                    m_descriptorHandleIncrementSize;
    uint32_t                    m_numDescriptorsInHeap;
    uint32_t                    m_numFreeHandles;
    
    FreeListBySize              m_freeListBySize;
    FreeListByOffset            m_freeListByOffset;
    StaleDescriptorQueue        m_staleDescriptors;

    std::mutex                  m_allocationMutex;
};