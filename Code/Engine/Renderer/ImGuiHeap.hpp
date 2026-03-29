#include "Game/EngineBuildPreferences.hpp"
#include <vector>

#ifdef USING_DX12
#include <d3d12.h>

class ImGuiHeap
{
public:
	ImGuiHeap(ID3D12Device* device);
	~ImGuiHeap();
	void Destroy();
	void Alloc(D3D12_CPU_DESCRIPTOR_HANDLE* out_cpuHandle, D3D12_GPU_DESCRIPTOR_HANDLE* out_gpuHandle);
	void Free(D3D12_CPU_DESCRIPTOR_HANDLE out_cpuHandle, D3D12_GPU_DESCRIPTOR_HANDLE out_gpuHandle);

public:
	ID3D12DescriptorHeap* m_imGuiHeap = nullptr;
	D3D12_DESCRIPTOR_HEAP_TYPE  m_heapType = D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES;
	D3D12_CPU_DESCRIPTOR_HANDLE m_heapStartCpu;
	D3D12_GPU_DESCRIPTOR_HANDLE m_heapStartGpu;
	UINT                        m_heapHandleIncrement;
	std::vector<int>            m_freeIndices;

};
#endif