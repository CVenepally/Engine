#include <d3d12.h>

#include "Engine/Renderer/CommandQueue.h"
#include "Engine/Renderer/CommandList.hpp"
#include "Engine/Renderer/ResourceStateTracker.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Core/EngineCommon.hpp"

//------------------------------------------------------------------------------------------------------------------
CommandQueue::CommandQueue(ID3D12Device10* device, D3D12_COMMAND_LIST_TYPE type)
    : m_fenceValue(0)
    , m_commandListType(type)
    , m_device(device)
    , m_processInFlightCommandLists(true)
{
    D3D12_COMMAND_QUEUE_DESC desc   = {};
    desc.Type                       = m_commandListType;
    desc.Priority                   = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
    desc.Flags                      = D3D12_COMMAND_QUEUE_FLAG_NONE;
    desc.NodeMask                   = 0;

    HRESULT hr;
    
    hr = device->CreateCommandQueue(&desc, IID_PPV_ARGS(&m_d3dCommandQueue));
    if (FAILED(hr))
    {
        ERROR_AND_DIE("Failed to create command queue")
    }

    switch(m_commandListType)
    {
        case D3D12_COMMAND_LIST_TYPE_COPY:
        {
            m_d3dCommandQueue->SetName(L"Copy Command Queue");
            break;
        }
		case D3D12_COMMAND_LIST_TYPE_DIRECT:
		{
			m_d3dCommandQueue->SetName(L"Graphics Command Queue");
			break;
		}
		case D3D12_COMMAND_LIST_TYPE_COMPUTE:
		{
			m_d3dCommandQueue->SetName(L"Compute Command Queue");
			break;
		}
        default:
            break;
    }
    
    hr = device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence));
    if (FAILED(hr))
    {
        ERROR_AND_DIE("Failed to create fence")
    }
    m_fence->SetName(L"Fence");

    m_processInFlightCommandListsThread = std::thread(&CommandQueue::ProcessInFlightCommandLists, this);

}

//------------------------------------------------------------------------------------------------------------------
CommandQueue::~CommandQueue()
{
    
    Flush();

	while(!m_inFlightCommandLists.Empty())
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}

	m_processInFlightCommandLists = false;
	m_processInFlightCommandListsThread.join();

    while(!m_availableCommandLists.Empty())
    {
        CommandList* commandList = GetCommandList();
        delete commandList;
    }

    DX_SAFE_RELEASE(m_fence)
    DX_SAFE_RELEASE(m_d3dCommandQueue)
	
}

//------------------------------------------------------------------------------------------------------------------
CommandList* CommandQueue::GetCommandList()
{
    CommandList* commandList = nullptr;

    if(!m_availableCommandLists.Empty())
    {
        m_availableCommandLists.TryPop(commandList);
    }
    else
    {
        commandList = new CommandList(this, m_device, m_commandListType);
    }

    return commandList;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
uint64_t CommandQueue::ExecuteCommandList(CommandList* commandList)
{
    return ExecuteCommandLists(std::vector<CommandList*>({commandList}));
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
uint64_t CommandQueue::ExecuteCommandLists(std::vector<CommandList*> commandLists)
{
    ResourceStateTracker::Lock();

    std::vector<CommandList*> toBeQueued;
    toBeQueued.reserve(commandLists.size() * 2);

    //Generate mips command list (compute) here

	std::vector<ID3D12CommandList*> toBeExecuted;
	toBeQueued.reserve(commandLists.size() * 2);

    for(CommandList* commandList : commandLists)
    {
        CommandList*    pendingCommandList = GetCommandList();
        bool            hasPendingBarriers = commandList->Close(pendingCommandList);
        pendingCommandList->Close();

        if(hasPendingBarriers)
        {
            toBeExecuted.push_back(pendingCommandList->GetCommandList());
        }

        toBeExecuted.push_back(commandList->GetCommandList());

        toBeQueued.push_back(pendingCommandList);
        toBeQueued.push_back(commandList);

        // Generate mips command list (compute command list) here
    }

    UINT numCommandLists = static_cast<UINT>(toBeExecuted.size());
    m_d3dCommandQueue->ExecuteCommandLists(numCommandLists, toBeExecuted.data());

    uint64_t fenceValue = Signal();

    ResourceStateTracker::Unlock();

    for(CommandList* commandList : toBeQueued)
    {
        m_inFlightCommandLists.Push({fenceValue, commandList});
    }

    // execute mip generation command lists
    return fenceValue;
}

//------------------------------------------------------------------------------------------------------------------
uint64_t CommandQueue::Signal()
{
    uint64_t fenceValue = ++m_fenceValue;
    
    HRESULT hr;
    hr = m_d3dCommandQueue->Signal(m_fence, fenceValue);

    if (FAILED(hr))
    {
        ERROR_AND_DIE("Fence/Command Queue Signal failed")
    }
    
    return fenceValue;
}

//------------------------------------------------------------------------------------------------------------------
bool CommandQueue::IsFenceComplete(uint64_t fenceValue)
{
    return m_fence->GetCompletedValue() >= fenceValue;
}

//------------------------------------------------------------------------------------------------------------------
void CommandQueue::WaitForFenceValue(uint64_t fenceValue)
{
    if (!IsFenceComplete(fenceValue))
    {
        auto event = CreateEvent(nullptr, FALSE, FALSE, nullptr);
        GUARANTEE_OR_DIE(event, "Failed to create fence event handle")
        m_fence->SetEventOnCompletion(fenceValue, event);
        WaitForSingleObject(event, INFINITE);
        CloseHandle(event);
    }
}

//------------------------------------------------------------------------------------------------------------------
void CommandQueue::Flush()
{
    std::unique_lock<std::mutex> lock(m_processInFlightCommandListsThreradMutex);
    m_processInFlightCommandListsThreadCV.wait(lock, [this]{ return m_inFlightCommandLists.Empty(); });

    WaitForFenceValue(m_fenceValue);
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void CommandQueue::Wait(CommandQueue* other)
{
    m_d3dCommandQueue->Wait(other->m_fence, other->m_fenceValue);
}

//------------------------------------------------------------------------------------------------------------------
ID3D12CommandQueue* CommandQueue::GetCommandQueue() const
{
    return m_d3dCommandQueue;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void CommandQueue::ProcessInFlightCommandLists()
{
    std::unique_lock<std::mutex> lock(m_processInFlightCommandListsThreradMutex, std::defer_lock);

    while(m_processInFlightCommandLists)
    {
        CommandListEntry commandListEntry;
        lock.lock();
        while(m_inFlightCommandLists.TryPop(commandListEntry))
        {
            auto fenceValue = std::get<0>(commandListEntry);
            auto commandList = std::get<1>(commandListEntry);
            
            WaitForFenceValue(fenceValue);
            commandList->Reset();
            m_availableCommandLists.Push(commandList);
        }

        lock.unlock();
        m_processInFlightCommandListsThreadCV.notify_one();

        std::this_thread::yield();
    }
}




