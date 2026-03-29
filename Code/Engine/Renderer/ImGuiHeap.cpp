#include "Engine/Renderer/ImGuiHeap.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Core/EngineCommon.hpp"
#ifdef USING_DX12
//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
ImGuiHeap::ImGuiHeap(ID3D12Device* device)
{
	D3D12_DESCRIPTOR_HEAP_DESC desc = {};
	desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	desc.NumDescriptors = 1024;
	desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	if(device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&m_imGuiHeap)) != S_OK)
	{
		ERROR_AND_DIE("Failed to create ImGUIHeap")
	}

	m_imGuiHeap->SetName(L"ImGui Heap");

	m_heapType = desc.Type;
	m_heapStartCpu = m_imGuiHeap->GetCPUDescriptorHandleForHeapStart();
	m_heapStartGpu = m_imGuiHeap->GetGPUDescriptorHandleForHeapStart();
	m_heapHandleIncrement = device->GetDescriptorHandleIncrementSize(m_heapType);
	m_freeIndices.reserve((int)desc.NumDescriptors);
	for(int n = desc.NumDescriptors; n > 0; n--)
		m_freeIndices.push_back(n - 1);
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
ImGuiHeap::~ImGuiHeap()
{
	Destroy();
	DX_SAFE_RELEASE(m_imGuiHeap)
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void ImGuiHeap::Alloc(D3D12_CPU_DESCRIPTOR_HANDLE* out_cpuHandle, D3D12_GPU_DESCRIPTOR_HANDLE* out_gpuHandle)
{
	GUARANTEE_OR_DIE(m_freeIndices.size() > 0, "No free indices");
	int idx = m_freeIndices.back();
	m_freeIndices.pop_back();
	out_cpuHandle->ptr = m_heapStartCpu.ptr + (idx * m_heapHandleIncrement);
	out_gpuHandle->ptr = m_heapStartGpu.ptr + (idx * m_heapHandleIncrement);
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void ImGuiHeap::Free(D3D12_CPU_DESCRIPTOR_HANDLE out_cpuHandle, D3D12_GPU_DESCRIPTOR_HANDLE out_gpuHandle)
{
	int cpu_idx = (int)((out_cpuHandle.ptr - m_heapStartCpu.ptr) / m_heapHandleIncrement);
	int gpu_idx = (int)((out_gpuHandle.ptr - m_heapStartGpu.ptr) / m_heapHandleIncrement);
	GUARANTEE_OR_DIE(cpu_idx == gpu_idx, "Indexes not equal");

	m_freeIndices.push_back(cpu_idx);
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void ImGuiHeap::Destroy()
{
	m_imGuiHeap = nullptr;
	m_freeIndices.clear();
}

#endif // USING_DX12