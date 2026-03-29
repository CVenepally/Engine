#pragma once
#include <cstdint>

//------------------------------------------------------------------------------------------------------------------
struct AllocationResult;
struct ID3D12Resource;
struct ID3D12Device10;

//------------------------------------------------------------------------------------------------------------------
struct AllocatorPage
{
	friend class UploadBuffer;

private:
	AllocatorPage(ID3D12Device10* device, size_t sizeInBytes);
	virtual ~AllocatorPage();

	bool				HasSpace(size_t sizeInBytes, size_t alignment) const;
	AllocationResult	Allocate(size_t sizeInBytes, size_t alignment);
	void				Reset();

private:
	ID3D12Resource*			m_pageResource = nullptr;
	void*					m_cpuAddress = nullptr;
	uint64_t				m_gpuAddress;
	size_t					m_pageSize;
	size_t					m_offset;
};