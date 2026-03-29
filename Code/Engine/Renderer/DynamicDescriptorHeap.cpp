#include "Engine/Renderer/DynamicDescriptorHeap.hpp"
#include "Engine/Renderer/RootSignature.hpp"
#include "Engine/Renderer/CommandList.hpp"
#include <d3d12.h>

#if defined (max)
#undef max
#endif

#include "Engine/Core/EngineCommon.hpp"
//------------------------------------------------------------------------------------------------------------------
DynamicDescriptorHeap::DynamicDescriptorHeap(ID3D12Device10* device, D3D12_DESCRIPTOR_HEAP_TYPE heapType, uint32_t numDescriptorsPerHeap)
    : m_device(device)
    , m_heapType(heapType)
    , m_numDescriptorsPerHeap(numDescriptorsPerHeap)
    , m_descriptorTableBitMask(0)
    , m_staleDescriptorTableBitMask(0)
    , m_currentCPUDescriptorHandle(static_cast<size_t>(-1))
    , m_currentGPUDescriptorHandle(static_cast<size_t>(-1))
    , m_numFreeHandles(0)
{
    m_descriptorHandleIncrementSize = device->GetDescriptorHandleIncrementSize(heapType);
    m_descriptorHandleCache         = new D3D12_CPU_DESCRIPTOR_HANDLE[m_numDescriptorsPerHeap];
}

//------------------------------------------------------------------------------------------------------------------
DynamicDescriptorHeap::~DynamicDescriptorHeap()
{
    delete[] m_descriptorHandleCache;

    while (!m_descriptorHeapPool.empty())
    {
        DX_SAFE_RELEASE(m_descriptorHeapPool.front())
        m_descriptorHeapPool.pop();
    }
}

//------------------------------------------------------------------------------------------------------------------
void DynamicDescriptorHeap::StageDescriptors(uint32_t rootParameterIndex, uint32_t offset, uint32_t numDescriptors,
    D3D12_CPU_DESCRIPTOR_HANDLE const& srcDescriptor)
{
    if (numDescriptors > m_numDescriptorsPerHeap || rootParameterIndex >= MAX_DESCRIPTOR_TABLES)
    {
        DebuggerPrintf("Bad Allocation: DynamicDescriptorHeap::StageDescriptors");
        return;
    }

    DescriptorTableCache& descriptorTableCache = m_descriptorTableCache[rootParameterIndex];
    if ((offset + numDescriptors) > descriptorTableCache.m_numDescriptors)
    {
        DebuggerPrintf("Length Error: Number of descriptors exceeds the number of descriptors in the descriptor table; DynamicDescriptorHeap::StageDescriptors()");
        return;
    }

    D3D12_CPU_DESCRIPTOR_HANDLE* dstDescriptor = descriptorTableCache.m_baseDescriptor + offset;
    for (uint32_t i = 0; i < numDescriptors; i++)
    {
        dstDescriptor[i].ptr = srcDescriptor.ptr + static_cast<size_t>(i * m_descriptorHandleIncrementSize);
    }

    // Nuke if fail
    descriptorTableCache.m_numStagedDescriptors = std::max(descriptorTableCache.m_numStagedDescriptors, offset + numDescriptors);

    m_staleDescriptorTableBitMask |= (1 << rootParameterIndex);
}

//------------------------------------------------------------------------------------------------------------------
void DynamicDescriptorHeap::CommitStagedDescriptors(CommandList& commandList,
    std::function<void(ID3D12GraphicsCommandList7*, UINT, D3D12_GPU_DESCRIPTOR_HANDLE)> setFunc)
{
    uint32_t numDescriptorsToCommit = ComputeStaleDescriptorCount();

    if (numDescriptorsToCommit > 0)
    {
        ID3D12GraphicsCommandList7* d3dCommandList = commandList.GetCommandList();
        if (!d3dCommandList)
        {
            ERROR_AND_DIE("GetCommandList() failed. Returned nullptr; DynamicDescriptorHeap::CommitStagedDescriptors")
        }

        if (!m_currentDescriptorHeap || m_numFreeHandles < numDescriptorsToCommit)
        {
            m_currentDescriptorHeap = RequestDescriptorHeap();
            m_currentCPUDescriptorHandle = static_cast<size_t>(m_currentDescriptorHeap->GetCPUDescriptorHandleForHeapStart().ptr);
            m_currentGPUDescriptorHandle = static_cast<size_t>(m_currentDescriptorHeap->GetGPUDescriptorHandleForHeapStart().ptr);
            m_numFreeHandles = m_numDescriptorsPerHeap;

            commandList.SetDescriptorHeap(m_heapType, m_currentDescriptorHeap);
            m_staleDescriptorTableBitMask = m_descriptorTableBitMask;
        }

        DWORD rootIndex;
        while (_BitScanForward(&rootIndex, m_staleDescriptorTableBitMask))
        {
            unsigned int numSrcDescriptors = m_descriptorTableCache[rootIndex].m_numStagedDescriptors;

			// Skip if nothing was staged
			if(numSrcDescriptors == 0)
			{
				m_staleDescriptorTableBitMask ^= (1 << rootIndex);
				continue;
			}

            D3D12_CPU_DESCRIPTOR_HANDLE* srcDescriptorHandles = m_descriptorTableCache[rootIndex].m_baseDescriptor;

            D3D12_CPU_DESCRIPTOR_HANDLE currentCPUHandle = {};
            currentCPUHandle.ptr = m_currentCPUDescriptorHandle;
            
            D3D12_CPU_DESCRIPTOR_HANDLE destDescriptorRangeStarts[] = {currentCPUHandle};

            unsigned int destDescriptorRangeSizes[] = {numSrcDescriptors};

            m_device->CopyDescriptors(1, destDescriptorRangeStarts, destDescriptorRangeSizes,
                numSrcDescriptors, srcDescriptorHandles, nullptr, m_heapType);

            D3D12_GPU_DESCRIPTOR_HANDLE currentGPUHandle = {};
            currentGPUHandle.ptr = m_currentGPUDescriptorHandle;
            
            setFunc(d3dCommandList, rootIndex, currentGPUHandle);

            m_currentCPUDescriptorHandle += static_cast<size_t>(numSrcDescriptors * m_descriptorHandleIncrementSize);
            m_currentGPUDescriptorHandle += static_cast<size_t>(numSrcDescriptors * m_descriptorHandleIncrementSize);
            m_numFreeHandles -= numSrcDescriptors;
            m_staleDescriptorTableBitMask ^= (1 << rootIndex);

            // Nuke if fail
            m_descriptorTableCache[rootIndex].m_numStagedDescriptors = 0;
        }
    }
}

//------------------------------------------------------------------------------------------------------------------
void DynamicDescriptorHeap::CommitStagedDescriptorsForDraw(CommandList& commandList)
{
    CommitStagedDescriptors(commandList, &ID3D12GraphicsCommandList::SetGraphicsRootDescriptorTable);
}

//------------------------------------------------------------------------------------------------------------------
void DynamicDescriptorHeap::CommitStagedDescriptorsForDispatch(CommandList& commandList)
{
    CommitStagedDescriptors(commandList, &ID3D12GraphicsCommandList::SetComputeRootDescriptorTable);
}

//------------------------------------------------------------------------------------------------------------------
D3D12_GPU_DESCRIPTOR_HANDLE DynamicDescriptorHeap::CopyDescriptor(CommandList& commandList,
    D3D12_CPU_DESCRIPTOR_HANDLE cpuDescriptor)
{
    if (!m_currentDescriptorHeap || m_numFreeHandles < 1)
    {
        m_currentDescriptorHeap = RequestDescriptorHeap();
        m_currentCPUDescriptorHandle = m_currentDescriptorHeap->GetCPUDescriptorHandleForHeapStart().ptr;
        m_currentGPUDescriptorHandle = m_currentDescriptorHeap->GetGPUDescriptorHandleForHeapStart().ptr;
        m_numFreeHandles = m_numDescriptorsPerHeap;

        commandList.SetDescriptorHeap(m_heapType, m_currentDescriptorHeap);
        m_staleDescriptorTableBitMask = m_descriptorTableBitMask;
    }

    D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle = {};
    gpuHandle.ptr = m_currentGPUDescriptorHandle;

    D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = {};
    cpuHandle.ptr = m_currentCPUDescriptorHandle;
    
    m_device->CopyDescriptorsSimple(1, cpuHandle, cpuDescriptor, m_heapType);

    m_currentCPUDescriptorHandle += m_descriptorHandleIncrementSize;
    m_currentGPUDescriptorHandle += m_descriptorHandleIncrementSize;

    m_numFreeHandles -= 1;

    return gpuHandle;
}

//------------------------------------------------------------------------------------------------------------------
void DynamicDescriptorHeap::ParseRootSignature(RootSignature const& rootSignature)
{
    m_staleDescriptorTableBitMask = 0;
    D3D12_ROOT_SIGNATURE_DESC1 rootSignatureDesc = rootSignature.GetRootSignatureDesc();

    m_descriptorTableBitMask = rootSignature.GetDescriptorTableBitMask(m_heapType);
    uint32_t descriptorTableBitMask = m_descriptorTableBitMask;

    uint32_t currentOffset = 0;
    DWORD rootIndex;

    while (_BitScanForward(&rootIndex, descriptorTableBitMask) && rootIndex < rootSignatureDesc.NumParameters)
    {
        uint32_t numDescriptors = rootSignature.GetNumDescriptors(rootIndex);

        DescriptorTableCache& descriptorTableCache = m_descriptorTableCache[rootIndex];
        descriptorTableCache.m_numDescriptors = numDescriptors;
        descriptorTableCache.m_baseDescriptor = m_descriptorHandleCache + currentOffset;

        currentOffset += numDescriptors;

        descriptorTableBitMask ^= (1 << rootIndex);
    }

    GUARANTEE_OR_DIE(currentOffset <= m_numDescriptorsPerHeap, "The root signature requires more than the maximum number of descriptors per descriptor heap. Consider increasing the maximum number of descriptors per descriptor heap.")
}

//------------------------------------------------------------------------------------------------------------------
void DynamicDescriptorHeap::Reset()
{
    m_availableDescriptorHeaps      = m_descriptorHeapPool;
    m_currentDescriptorHeap         = nullptr;
    m_currentCPUDescriptorHandle    = static_cast<size_t>(-1);
    m_currentGPUDescriptorHandle    = static_cast<size_t>(-1);

    m_numFreeHandles = 0;
    m_descriptorTableBitMask = 0;
    m_staleDescriptorTableBitMask = 0;

    for (uint32_t i = 0; i < MAX_DESCRIPTOR_TABLES; ++i)
    {
        m_descriptorTableCache[i].Reset();
    }
    
}

//------------------------------------------------------------------------------------------------------------------
ID3D12DescriptorHeap* DynamicDescriptorHeap::RequestDescriptorHeap()
{
    ID3D12DescriptorHeap* descriptorHeap;

    if (!m_availableDescriptorHeaps.empty())
    {
        descriptorHeap = m_availableDescriptorHeaps.front();
        m_availableDescriptorHeaps.pop();
    }
    else
    {
        descriptorHeap = CreateDescriptorHeap();
        m_descriptorHeapPool.push(descriptorHeap);
    }
    return descriptorHeap;
}

//------------------------------------------------------------------------------------------------------------------
ID3D12DescriptorHeap* DynamicDescriptorHeap::CreateDescriptorHeap()
{
    D3D12_DESCRIPTOR_HEAP_DESC descriptorHeapDesc = {};
    descriptorHeapDesc.Type = m_heapType;
    descriptorHeapDesc.NumDescriptors = m_numDescriptorsPerHeap;
    descriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

    ID3D12DescriptorHeap* descriptorHeap = nullptr;
    
    HRESULT hr;
    hr = m_device->CreateDescriptorHeap(&descriptorHeapDesc, IID_PPV_ARGS(&descriptorHeap));

    if (FAILED(hr))
    {
        ERROR_AND_DIE("Descriptor Heap Creation Failed; DynamicDescriptorHeap::CreateDescriptorHeap()")
    }

    return descriptorHeap;
}

//------------------------------------------------------------------------------------------------------------------
uint32_t DynamicDescriptorHeap::ComputeStaleDescriptorCount() const
{
    uint32_t numStaleDescriptors = 0;

    DWORD i;
    DWORD staleDescriptorBitMask = m_staleDescriptorTableBitMask;

    while (_BitScanForward(&i, staleDescriptorBitMask))
    {
        numStaleDescriptors += m_descriptorTableCache[i].m_numStagedDescriptors;
        staleDescriptorBitMask ^= (1 << i);
    }
    return numStaleDescriptors;
}
