#pragma once
#include <cstdint>
#include <d3d12.h>
#include <functional>
#include <queue>

#include "DescriptorAllocator.hpp"

//------------------------------------------------------------------------------------------------------------------
class   RootSignature;
class   CommandList;

struct  D3D12_CPU_DESCRIPTOR_HANDLE;
struct  D3D12_GPU_DESCRIPTOR_HANDLE;
struct  ID3D12GraphicsCommandList7;
struct  ID3D12Device10;

typedef std::queue<ID3D12DescriptorHeap*> DescriptorHeapPool;

//------------------------------------------------------------------------------------------------------------------
struct DescriptorTableCache
{
    friend class DynamicDescriptorHeap;
    friend class CommandList;
    
private:
    DescriptorTableCache()
        : m_numDescriptors(0)
        , m_numStagedDescriptors(0)
        , m_baseDescriptor(nullptr)
    {}

    void Reset()
    {
        m_numDescriptors = 0;
        m_numStagedDescriptors = 0;
        m_baseDescriptor = nullptr;
    }

private:
    uint32_t                        m_numDescriptors;
    uint32_t                        m_numStagedDescriptors;
    D3D12_CPU_DESCRIPTOR_HANDLE*    m_baseDescriptor;
};

//------------------------------------------------------------------------------------------------------------------
class DynamicDescriptorHeap
{
    friend class CommandList;
    friend class DX12Renderer;
    
private:
    DynamicDescriptorHeap(ID3D12Device10* device, D3D12_DESCRIPTOR_HEAP_TYPE heapType, uint32_t numDescriptorsPerHeap = 1024);
    virtual ~DynamicDescriptorHeap();

    /**
        Stages a contiguous range of CPU-visible Descriptors.
        Handles of CPU-visible descriptors are copied to GPU-visible heap.
        CPU-visible descriptors should nopt be reused or overwritten until CommitStagedDescriptors method is called.
     */
    void StageDescriptors(uint32_t rootParameterIndex, uint32_t offset, uint32_t numDescriptors, D3D12_CPU_DESCRIPTOR_HANDLE const& srcDescriptors);

    /**
        Copy all staged descriptors into the GPU visible heap and bind the heap nad descriptor tables to the command list.
        The setFunc param (std::function<>) is used to set the GPU visible descriptors on the command list.
        Two possible functions are:
        * Before a draw    : ID3D12GraphicsCommandList::SetGraphicsRootDescriptorTable
        * Before a dispatch: ID3D12GraphicsCommandList::SetComputeRootDescriptorTable
        Since the DynamicDescriptorHeap can't know which function will be used, it must be passed as an argument to the function.
     */
    void CommitStagedDescriptors(CommandList& commandList, std::function<void(ID3D12GraphicsCommandList7*, UINT, D3D12_GPU_DESCRIPTOR_HANDLE)> setFunc);
    void CommitStagedDescriptorsForDraw(CommandList& commandList);
    void CommitStagedDescriptorsForDispatch(CommandList& commandList);

    D3D12_GPU_DESCRIPTOR_HANDLE CopyDescriptor(CommandList& commandList, D3D12_CPU_DESCRIPTOR_HANDLE cpuDescriptor);

    /**
        * Parse the root signature to determine which root parameters contain
        * descriptor tables and determine the number of descriptors needed for
        * each table.
   */
    void ParseRootSignature(RootSignature const& rootSignature);

    void Reset();

    ID3D12DescriptorHeap*   RequestDescriptorHeap();
    ID3D12DescriptorHeap*   CreateDescriptorHeap();

    uint32_t                ComputeStaleDescriptorCount() const;

private:
    static constexpr uint32_t       MAX_DESCRIPTOR_TABLES = 32;

    D3D12_DESCRIPTOR_HEAP_TYPE      m_heapType;
    uint32_t                        m_numDescriptorsPerHeap;
    uint32_t                        m_descriptorHandleIncrementSize;

    D3D12_CPU_DESCRIPTOR_HANDLE*    m_descriptorHandleCache;
    DescriptorTableCache            m_descriptorTableCache[MAX_DESCRIPTOR_TABLES];

    uint32_t                        m_descriptorTableBitMask;
    uint32_t                        m_staleDescriptorTableBitMask;

    DescriptorHeapPool              m_descriptorHeapPool;
    DescriptorHeapPool              m_availableDescriptorHeaps;

    ID3D12DescriptorHeap*           m_currentDescriptorHeap;
    size_t                          m_currentGPUDescriptorHandle;
    size_t                          m_currentCPUDescriptorHandle;

    uint32_t                        m_numFreeHandles;

    ID3D12Device10*                 m_device;
};