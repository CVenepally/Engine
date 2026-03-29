#include "Engine/Renderer/DescriptorAllocationResult.hpp"
#include "Engine/Renderer/DescriptorAllocatorPage.hpp"
#include "Engine/Renderer/DX12Renderer.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"

#include "Game/EngineBuildPreferences.hpp"

#include <d3d12.h>
//------------------------------------------------------------------------------------------------------------------
DescriptorAllocationResult::DescriptorAllocationResult()
    : m_cpuDescriptorHandle(0)
    , m_numHandles(0)
    , m_descriptorSize(0)
    , m_descriptorAllocatorPage(nullptr)
{
}

//------------------------------------------------------------------------------------------------------------------
DescriptorAllocationResult::DescriptorAllocationResult(D3D12_CPU_DESCRIPTOR_HANDLE descriptorCPUHandle, uint32_t numHandles, uint32_t descriptorSize, DescriptorAllocatorPage* page)
    : m_cpuDescriptorHandle(descriptorCPUHandle.ptr)
    , m_numHandles(numHandles)
    , m_descriptorSize(descriptorSize)
    , m_descriptorAllocatorPage(page)
{
    
}

//------------------------------------------------------------------------------------------------------------------
DescriptorAllocationResult::~DescriptorAllocationResult()
{
    Free();
}

//------------------------------------------------------------------------------------------------------------------
DescriptorAllocationResult::DescriptorAllocationResult(DescriptorAllocationResult&& otherAllocationResult) noexcept
    : m_cpuDescriptorHandle(otherAllocationResult.m_cpuDescriptorHandle)
    , m_numHandles(otherAllocationResult.m_numHandles)
    , m_descriptorSize(otherAllocationResult.m_descriptorSize)
    , m_descriptorAllocatorPage(std::move(otherAllocationResult.m_descriptorAllocatorPage))
{
    otherAllocationResult.m_cpuDescriptorHandle = 0;
    otherAllocationResult.m_numHandles          = 0;
    otherAllocationResult.m_descriptorSize      = 0;
}

//------------------------------------------------------------------------------------------------------------------
DescriptorAllocationResult& DescriptorAllocationResult::operator=(DescriptorAllocationResult&& otherAllocationResult) noexcept
{
    Free();
    m_cpuDescriptorHandle       = otherAllocationResult.m_cpuDescriptorHandle;
    m_numHandles                = otherAllocationResult.m_numHandles;
    m_descriptorSize            = otherAllocationResult.m_descriptorSize;
    m_descriptorAllocatorPage   = std::move(otherAllocationResult.m_descriptorAllocatorPage);
    
    otherAllocationResult.m_cpuDescriptorHandle = 0;
    otherAllocationResult.m_numHandles          = 0;
    otherAllocationResult.m_descriptorSize      = 0;

    return *this;
}

//------------------------------------------------------------------------------------------------------------------
bool DescriptorAllocationResult::IsNull() const
{
    return m_cpuDescriptorHandle == 0;
}

//------------------------------------------------------------------------------------------------------------------
D3D12_CPU_DESCRIPTOR_HANDLE DescriptorAllocationResult::GetDescriptorCPUHandle(uint32_t offset) const
{
    GUARANTEE_OR_DIE(offset < m_numHandles, "GetDescriptorCPUHandle `offset` out of range (offset > numHandles); DescriptorAllocationResult::GetDescriptorCPUHandle")

    return {m_cpuDescriptorHandle + static_cast<size_t>(offset * m_descriptorSize)};

}

//------------------------------------------------------------------------------------------------------------------
uint32_t DescriptorAllocationResult::GetNumHandles() const
{
    return m_numHandles;
}

//------------------------------------------------------------------------------------------------------------------
DescriptorAllocatorPage* DescriptorAllocationResult::GetDescriptorAllocatorPage() const
{
    return m_descriptorAllocatorPage;
}

//------------------------------------------------------------------------------------------------------------------
void DescriptorAllocationResult::Free()
{
#if defined(USING_DX12)
	if(!IsNull() && m_descriptorAllocatorPage)
	{
		m_descriptorAllocatorPage->Free(std::move(*this), DX12Renderer::GetCurrentFrameNumber());
		m_cpuDescriptorHandle = 0;
		m_numHandles = 0;
		m_descriptorSize = 0;

		m_descriptorAllocatorPage = nullptr;
	}
#endif
}


