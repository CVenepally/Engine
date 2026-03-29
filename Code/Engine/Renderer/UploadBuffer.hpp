#pragma once

#include <deque>
#include <cstdint>

//------------------------------------------------------------------------------------------------------------------
struct AllocatorPage;

typedef std::deque<AllocatorPage*> AllocatorPagePool;

//------------------------------------------------------------------------------------------------------------------
constexpr size_t k_kiloByte = 1024;
constexpr size_t k_megaByte = k_kiloByte * k_kiloByte;

//------------------------------------------------------------------------------------------------------------------
struct AllocationResult
{
    void*       m_cpuAddress = nullptr;
    uint64_t    m_gpuAddress;
};

//------------------------------------------------------------------------------------------------------------------
class UploadBuffer
{
    friend class DX12Renderer;
    friend class CommandList;
public:
    explicit UploadBuffer(CommandList* commandList, size_t pageSize = 2 * k_megaByte);
    virtual  ~UploadBuffer();

    size_t              GetPageSize() const;
    AllocationResult    Allocate(size_t sizeInBytes, size_t alignment);
    void                Reset();

private:
    AllocatorPage*      RequestPage();
    
private:
    AllocatorPagePool   m_pagePool;
    AllocatorPagePool   m_availablePages;

    AllocatorPage*      m_currentPage;

    size_t              m_pageSize;

    CommandList*        m_commandList;
};
