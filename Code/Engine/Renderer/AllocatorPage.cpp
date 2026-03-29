#include "Engine/Renderer/AllocatorPage.hpp"
#include "Engine/Renderer/UploadBuffer.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Math/MathUtils.hpp"

#include <d3d12.h>
//------------------------------------------------------------------------------------------------------------------
AllocatorPage::AllocatorPage(ID3D12Device10* device, size_t sizeInBytes)
    : m_pageSize(sizeInBytes)
    , m_offset(0)
    , m_gpuAddress(0)
{
    if (!device)
    {
        ERROR_AND_DIE("Invalid Device during Allocator Page Creation")
    }

    D3D12_HEAP_PROPERTIES pageHeapProperties    = {};
    pageHeapProperties.Type	                    = D3D12_HEAP_TYPE_UPLOAD;
    pageHeapProperties.CreationNodeMask         = 1;
    pageHeapProperties.VisibleNodeMask          = 1;

    D3D12_RESOURCE_DESC pageDesc    = {};
    pageDesc.Dimension				= D3D12_RESOURCE_DIMENSION_BUFFER;
    pageDesc.Width				    = m_pageSize;
    pageDesc.Height				    = 1;
    pageDesc.DepthOrArraySize		= 1;
    pageDesc.MipLevels				= 1;
    pageDesc.Format				    = DXGI_FORMAT_UNKNOWN;
    pageDesc.SampleDesc.Count		= 1;
    pageDesc.SampleDesc.Quality		= 0;
    pageDesc.Layout				    = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    pageDesc.Flags                  = D3D12_RESOURCE_FLAG_NONE;
    pageDesc.Alignment              = 0;

    HRESULT hr = device->CreateCommittedResource(&pageHeapProperties, D3D12_HEAP_FLAG_NONE, &pageDesc, D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr, IID_PPV_ARGS(&m_pageResource));
    
    if (FAILED(hr))
    {
        ERROR_AND_DIE("Failed to create page resource")
    }

    m_pageResource->SetName(L"Upload Buffer Allocator Page Resource");

    m_gpuAddress = m_pageResource->GetGPUVirtualAddress();
    m_pageResource->Map(0, nullptr, &m_cpuAddress);
}

//------------------------------------------------------------------------------------------------------------------
AllocatorPage::~AllocatorPage()
{
    m_pageResource->Unmap(0, nullptr);
    m_cpuAddress = nullptr;
    m_gpuAddress = 0;
    
    DX_SAFE_RELEASE(m_pageResource)
}

//------------------------------------------------------------------------------------------------------------------
bool AllocatorPage::HasSpace(size_t sizeInBytes, size_t alignment) const
{
    size_t alignedSize      = AlignUp(sizeInBytes, alignment);
    size_t alignedOffset    = AlignUp(m_offset, alignment);

    return alignedSize + alignedOffset <= m_pageSize;
}

//------------------------------------------------------------------------------------------------------------------
AllocationResult AllocatorPage::Allocate(size_t sizeInBytes, size_t alignment)
{

    // Todo: To make this or entire upload buffers thread safe, implement std::lock_guard here smh
    
    size_t alignedSize = AlignUp(sizeInBytes, alignment);
    m_offset = AlignUp(m_offset, alignment);

    AllocationResult allocationResult;
    allocationResult.m_cpuAddress = static_cast<uint8_t*>(m_cpuAddress) + m_offset;
    allocationResult.m_gpuAddress = m_gpuAddress + m_offset;

    m_offset += alignedSize;

    return allocationResult;
}

//------------------------------------------------------------------------------------------------------------------
void AllocatorPage::Reset()
{
    m_offset = 0;
}


