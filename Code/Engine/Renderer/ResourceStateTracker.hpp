#pragma once
#include <d3d12.h>
#include <cstdint>
#include <vector>
#include <map>
#include <unordered_map>
#include <mutex>

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
struct ResourceState;

typedef std::vector<D3D12_RESOURCE_BARRIER>					ResourceBarriers;
typedef std::unordered_map<ID3D12Resource*, ResourceState>	ResourceStateMap;

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Struct to track state of a particular resource. Internal use by Resource State Tracker (and maybe Command List) only
struct ResourceState
{
	friend class ResourceStateTracker;
	friend class CommandList;

public:
	explicit				ResourceState(D3D12_RESOURCE_STATES state = D3D12_RESOURCE_STATE_COMMON);

private:
	void					SetSubresourceState(unsigned int subresource, D3D12_RESOURCE_STATES state);
	D3D12_RESOURCE_STATES	GetSubresourceState(unsigned int subresource) const;
	
private:
	D3D12_RESOURCE_STATES							m_state;
	std::map<unsigned int, D3D12_RESOURCE_STATES>	m_subresourceState;
};

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
class ResourceStateTracker
{
	friend class CommandList;

private:
							ResourceStateTracker();
	virtual					~ResourceStateTracker();

	void					ResourceBarrier(D3D12_RESOURCE_BARRIER const& barrier);

	// Resource Barrier helpers
	void					TransitionResource(ID3D12Resource* resourceToTransition, D3D12_RESOURCE_STATES stateAfter, unsigned int subResource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES);
	void					UAVBarrier(ID3D12Resource* resource = nullptr);
	void					AliasBarrier(ID3D12Resource* resourceBefore = nullptr, ID3D12Resource* resourceAfter = nullptr);

	// Flush and commit
	uint32_t				FlushPendingResourceBarriers(CommandList* commandList);
	void					FlushResourceBarriers(CommandList* commandList);
	void					CommitFinalResourceStates();

	void					Reset();

public:
	//Lock the global state before flush to make sure that the final states of all the resources are consistent across multiple threads.
	// Unlock after commiting the final resources
	static void				Lock();
	static void				Unlock();

	// Add and remove Resources to and from the global state map during the resource creation and destruction process
	static void				AddGlobalResourceState(ID3D12Resource* resource, D3D12_RESOURCE_STATES state);
	static void				RemoveGlobalResourceState(ID3D12Resource* resource);

private:
	ResourceBarriers		m_pendingResourceBarriers;
	ResourceBarriers		m_resourceBarriers;
	ResourceStateMap		m_finalResourceState;

	static ResourceStateMap	s_globalResourceState;
	static std::mutex		s_globalMutex;
	static bool				s_isLocked;
};