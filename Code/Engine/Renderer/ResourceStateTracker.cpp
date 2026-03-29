#include "Engine/Renderer/ResourceStateTracker.hpp"
#include "Engine/Renderer/CommandList.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Core/EngineCommon.hpp"

//Static Definitions-------------------------------------------------------------------------------------------------------------------------------------------------
std::mutex			ResourceStateTracker::s_globalMutex;
bool				ResourceStateTracker::s_isLocked = false;
ResourceStateMap	ResourceStateTracker::s_globalResourceState;

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
ResourceState::ResourceState(D3D12_RESOURCE_STATES state)
	: m_state(state)
{}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void ResourceState::SetSubresourceState(unsigned int subresource, D3D12_RESOURCE_STATES state)
{
	if(subresource == D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES)
	{
		m_state = state;
		m_subresourceState.clear();
	}
	else
	{
		m_subresourceState[subresource] = state;
	}
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
D3D12_RESOURCE_STATES ResourceState::GetSubresourceState(unsigned int subresource) const
{
	D3D12_RESOURCE_STATES state = m_state;

	const auto iter = m_subresourceState.find(subresource);

	if(iter != m_subresourceState.end())
	{
		state = iter->second;
	}

	return state;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
ResourceStateTracker::ResourceStateTracker()
{}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
ResourceStateTracker::~ResourceStateTracker()
{
	for(auto& [resource, state] : s_globalResourceState)
	{
	//	resource->Release();
	}
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void ResourceStateTracker::ResourceBarrier(D3D12_RESOURCE_BARRIER const& barrier)
{
	if(barrier.Type == D3D12_RESOURCE_BARRIER_TYPE_TRANSITION)
	{
		D3D12_RESOURCE_TRANSITION_BARRIER const& transitionBarrier = barrier.Transition;
		const auto iter = m_finalResourceState.find(transitionBarrier.pResource);

		if(iter != m_finalResourceState.end())
		{
			ResourceState& resourceState = iter->second;
			if(transitionBarrier.Subresource == D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES && !resourceState.m_subresourceState.empty())
			{
				for(auto subresourceState : resourceState.m_subresourceState)
				{
					if(transitionBarrier.StateAfter != subresourceState.second)
					{
						D3D12_RESOURCE_BARRIER newBarrier = barrier;
						newBarrier.Transition.Subresource = subresourceState.first;
						newBarrier.Transition.StateBefore = subresourceState.second;
						m_resourceBarriers.push_back(newBarrier);
					}
				}
			}
			else
			{
				D3D12_RESOURCE_STATES finalState = resourceState.GetSubresourceState(transitionBarrier.Subresource);
				if(transitionBarrier.StateAfter != finalState)
				{
					D3D12_RESOURCE_BARRIER newBarrier = barrier;
					newBarrier.Transition.StateBefore = finalState;
					m_resourceBarriers.push_back(newBarrier);
				}
			}
		}
		else
		{
			m_pendingResourceBarriers.push_back(barrier);
		}

		m_finalResourceState[transitionBarrier.pResource].SetSubresourceState(transitionBarrier.Subresource, transitionBarrier.StateAfter);
	}
	else
	{
		m_resourceBarriers.push_back(barrier);
	}
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void ResourceStateTracker::TransitionResource(ID3D12Resource* resourceToTransition, D3D12_RESOURCE_STATES stateAfter, unsigned int subresource)
{
	if(resourceToTransition)
	{
		D3D12_RESOURCE_BARRIER transitionBarrier	= {};
		transitionBarrier.Type						= D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		transitionBarrier.Transition.pResource		= resourceToTransition;
		transitionBarrier.Transition.StateBefore	= s_globalResourceState[resourceToTransition].m_state;
		transitionBarrier.Transition.StateAfter		= stateAfter;
		transitionBarrier.Transition.Subresource	= static_cast<UINT>(subresource);

		ResourceBarrier(transitionBarrier);
	}
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void ResourceStateTracker::UAVBarrier(ID3D12Resource* resource)
{
	D3D12_RESOURCE_BARRIER uavBarrier	= {};
	uavBarrier.Type						= D3D12_RESOURCE_BARRIER_TYPE_UAV;
	uavBarrier.UAV.pResource			= resource;

	ResourceBarrier(uavBarrier);
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void ResourceStateTracker::AliasBarrier(ID3D12Resource* resourceBefore, ID3D12Resource* resourceAfter)
{
	D3D12_RESOURCE_BARRIER aliasBarrier		= {};
	aliasBarrier.Type						= D3D12_RESOURCE_BARRIER_TYPE_UAV;
	aliasBarrier.Aliasing.pResourceBefore	= resourceBefore;
	aliasBarrier.Aliasing.pResourceAfter	= resourceAfter;

	ResourceBarrier(aliasBarrier);
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
uint32_t ResourceStateTracker::FlushPendingResourceBarriers(CommandList* commandList)
{
	GUARANTEE_OR_DIE(s_isLocked, "GUARANTEE on s_isLocked failed. ResourceStateTracker::FlushPendingResourceBarriers");

	ResourceBarriers resourceBarriers;

	resourceBarriers.reserve(m_pendingResourceBarriers.size());

	for(D3D12_RESOURCE_BARRIER pendingBarrier : m_pendingResourceBarriers)
	{
		if(pendingBarrier.Type == D3D12_RESOURCE_BARRIER_TYPE_TRANSITION)
		{
			D3D12_RESOURCE_TRANSITION_BARRIER pendingTransition = pendingBarrier.Transition;
			auto const& iter = s_globalResourceState.find(pendingTransition.pResource);
			if(iter != s_globalResourceState.end())
			{
				auto& resourceState = iter->second;
				if(pendingTransition.Subresource == D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES && !resourceState.m_subresourceState.empty())
				{
					for(auto subresourceState : resourceState.m_subresourceState)
					{
						if(pendingTransition.StateAfter != subresourceState.second)
						{
							D3D12_RESOURCE_BARRIER newBarrier = pendingBarrier;
							newBarrier.Transition.Subresource = subresourceState.first;
							newBarrier.Transition.StateBefore = subresourceState.second;
							resourceBarriers.push_back(newBarrier);
						}
					}
				}
				else
				{
					auto globalState = (iter->second).GetSubresourceState(pendingTransition.Subresource);
					if(pendingTransition.StateAfter != globalState)
					{
						pendingBarrier.Transition.StateBefore = globalState;
						resourceBarriers.push_back(pendingBarrier);
					}
				}
			}
		}
	}

	UINT numBarriers = static_cast<UINT>(resourceBarriers.size());
	if(numBarriers > 0)
	{
		auto d3dCommandList = commandList->GetCommandList();
		d3dCommandList->ResourceBarrier(numBarriers, resourceBarriers.data());
	}

	m_pendingResourceBarriers.clear();

	return numBarriers;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void ResourceStateTracker::FlushResourceBarriers(CommandList* commandList)
{
	UINT numBarriers = static_cast<UINT>(m_resourceBarriers.size());
	if(numBarriers > 0)
	{
		ID3D12GraphicsCommandList7* d3dCommandList = commandList->GetCommandList();
		d3dCommandList->ResourceBarrier(numBarriers, m_resourceBarriers.data());
		m_resourceBarriers.clear();
	}

}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void ResourceStateTracker::CommitFinalResourceStates()
{
	GUARANTEE_OR_DIE(s_isLocked, "GUARANTEE on s_isLocked failed. ResourceStateTracker::CommitFinalResourceStates");
	for(auto const& resourceState : m_finalResourceState)
	{
		s_globalResourceState[resourceState.first] = resourceState.second;
	}

	m_finalResourceState.clear();
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void ResourceStateTracker::Reset()
{
	m_pendingResourceBarriers.clear();
	m_resourceBarriers.clear();
	m_finalResourceState.clear();
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void ResourceStateTracker::Lock()
{
	s_globalMutex.lock();
	s_isLocked = true;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void ResourceStateTracker::Unlock()
{
	s_globalMutex.unlock();
	s_isLocked = false;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void ResourceStateTracker::AddGlobalResourceState(ID3D12Resource* resource, D3D12_RESOURCE_STATES state)
{
	if(resource)
	{
		std::lock_guard<std::mutex> lock(s_globalMutex);
		s_globalResourceState[resource].SetSubresourceState(D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES, state);
	}
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void ResourceStateTracker::RemoveGlobalResourceState(ID3D12Resource* resource)
{
	if(resource)
	{
		std::lock_guard<std::mutex> lock(s_globalMutex);
		resource->Release();
		s_globalResourceState.erase(resource);
	}
}
