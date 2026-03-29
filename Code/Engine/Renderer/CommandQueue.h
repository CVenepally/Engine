#pragma once
#include "Engine/Core/ThreadSafeQueue.hpp"

#include <cstdint>
#include <atomic>
#include <condition_variable>
#include <d3d12.h>
#include <queue>


//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
class CommandList;

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
typedef std::tuple<uint64_t, CommandList*> CommandListEntry;

//------------------------------------------------------------------------------------------------------------------
// Custom wrapper for ID3D12CommandQueue interface. Owns CommandAllocators and CommandLists
class CommandQueue
{
    friend class DX12Renderer;
    
//private:
public:
    CommandQueue(ID3D12Device10* device, D3D12_COMMAND_LIST_TYPE type);
    ~CommandQueue();

    // Get an available command list from this command queue
    CommandList*                            GetCommandList();

    uint64_t                                ExecuteCommandList(CommandList* commandList);
    uint64_t                                ExecuteCommandLists(std::vector<CommandList*> commandLists);

    uint64_t                                Signal();
    bool                                    IsFenceComplete(uint64_t fenceValue);
    void                                    WaitForFenceValue(uint64_t fenceValue);
    void                                    Flush();
    void                                    Wait(CommandQueue* other);

    ID3D12CommandQueue*                     GetCommandQueue() const;

    void                                    ProcessInFlightCommandLists();

private:
    D3D12_COMMAND_LIST_TYPE				    m_commandListType;
    ID3D12Device10*                         m_device            = nullptr;
    ID3D12CommandQueue*					    m_d3dCommandQueue   = nullptr;
    ID3D12Fence1*							m_fence             = nullptr;
    std::atomic_uint64_t					m_fenceValue;
    
    ThreadSafeQueue<CommandListEntry>       m_inFlightCommandLists;
    ThreadSafeQueue<CommandList*>           m_availableCommandLists;

    std::thread                             m_processInFlightCommandListsThread;
    std::atomic_bool                        m_processInFlightCommandLists;
    std::mutex                              m_processInFlightCommandListsThreradMutex;
    std::condition_variable                 m_processInFlightCommandListsThreadCV;
    
};
