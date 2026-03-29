#include "Engine/Renderer/UploadBuffer.hpp"

#include "CommandList.hpp"
#include "Engine/Renderer/AllocatorPage.hpp"

//------------------------------------------------------------------------------------------------------------------
UploadBuffer::UploadBuffer(CommandList* commandList, size_t pageSize)
    :m_pageSize(pageSize)
    ,m_commandList(commandList)
{
    
}

//------------------------------------------------------------------------------------------------------------------
UploadBuffer::~UploadBuffer()
{
    m_currentPage = nullptr;
    m_availablePages.clear();

    while (!m_pagePool.empty())
    {
        delete m_pagePool.front();
        m_pagePool.pop_front();
    }
}

//------------------------------------------------------------------------------------------------------------------
size_t UploadBuffer::GetPageSize() const
{
    return m_pageSize;
}

//------------------------------------------------------------------------------------------------------------------
AllocationResult UploadBuffer::Allocate(size_t sizeInBytes, size_t alignment)
{
    if (sizeInBytes > m_pageSize)
    {
        return {};
    }

    if (!m_currentPage || !m_currentPage->HasSpace(sizeInBytes, alignment))
    {
        m_currentPage = RequestPage();
    }

    return m_currentPage->Allocate(sizeInBytes, alignment);
}

//------------------------------------------------------------------------------------------------------------------
void UploadBuffer::Reset()
{
    m_currentPage = nullptr;
    m_availablePages = m_pagePool;

    for (AllocatorPage* page : m_availablePages)
    {
        page->Reset();
    }
}

//------------------------------------------------------------------------------------------------------------------
AllocatorPage* UploadBuffer::RequestPage()
{
    AllocatorPage* newPage = nullptr;

    if (!m_availablePages.empty())
    {
        newPage = m_availablePages.front();
        m_availablePages.pop_front();
    }
    else
    {
        
        newPage = new AllocatorPage(m_commandList->GetDevice(), m_pageSize);
        m_pagePool.push_back(newPage);
    }

    return newPage;
}



