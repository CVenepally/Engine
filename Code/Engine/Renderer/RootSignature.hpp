#pragma once
#include <d3d12.h>
#include <vector>
#include <string>
#include <map>

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------

class RootSignature
{
	friend class DX12Renderer;

protected:
	RootSignature();
//	explicit RootSignature(ID3D12Device10* device, std::vector<RootParameters> const& rootParameters, std::string debugName);
	explicit RootSignature(ID3D12Device10* device, std::string const& name, D3D12_ROOT_SIGNATURE_DESC1& rootSigDesc, D3D_ROOT_SIGNATURE_VERSION version);
	~RootSignature();

public:
	ID3D12RootSignature*				GetRootSignature() const;
	D3D12_ROOT_SIGNATURE_DESC1 const&	GetRootSignatureDesc() const;
	uint32_t							GetDescriptorTableBitMask(D3D12_DESCRIPTOR_HEAP_TYPE descriptorHeapType) const;
	uint32_t							GetNumDescriptors(uint32_t rootIndex) const;

	void								SetRootSignatureDesc(D3D12_ROOT_SIGNATURE_DESC1& rootSigDesc, D3D_ROOT_SIGNATURE_VERSION version);
	void								Destroy();
private:
	
	ID3D12RootSignature*				m_d3dRootSignature = nullptr;
	D3D12_ROOT_SIGNATURE_DESC1			m_rootSignatureDesc;
	uint32_t							m_numDescriptorsPerTable[32];
	uint32_t							m_samplerTableBitMask;
	uint32_t							m_descriptorTableBitMask;
	
	std::string							m_name;
	ID3D12Device10*						m_device		= nullptr;
};