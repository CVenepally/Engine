#pragma once
#include "Game/EngineBuildPreferences.hpp"
#include <cstdint>
//------------------------------------------------------------------------------------------------------------------
#if defined (USING_DX12)
struct ID3D12Device10;
struct ID3D12Resource2;
#else
struct ID3D11Buffer;
struct ID3D11Device;

#endif


//------------------------------------------------------------------------------------------------------------------

// For now, Constant Buffer does nothing for DX12. Comeback and expand for improved functionality.
class ConstantBuffer
{
	friend class Renderer;
	friend class DX12Renderer;

public:

#if defined(USING_DX12)
	ConstantBuffer(ID3D12Device10* device, unsigned int size);
#else
	ConstantBuffer(ID3D11Device* device, unsigned int size);
#endif

	ConstantBuffer(const ConstantBuffer& copy) = delete;
	virtual ~ConstantBuffer();

	void Create();
	void CopyData(const void* data);

	unsigned int GetSize();


private:

#if defined(USING_DX12)
	ID3D12Device10*		m_device		= nullptr;
	ID3D12Resource2*	m_buffer		= nullptr;
	void*				m_mappedData	= nullptr;
	uint64_t			m_gpuAddress	= 0;
	unsigned int		m_size			= 0;
	unsigned int		m_alignedSize	= 0;

#else
	ID3D11Buffer* m_buffer = nullptr;
	ID3D11Device* m_device = nullptr;

	unsigned m_size = 0;
#endif
};