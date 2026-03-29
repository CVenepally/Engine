#include "Engine/Renderer/RootSignature.hpp"
#include "Engine/Core/EngineCommon.hpp"

//------------------------------------------------------------------------------------------------------------------
RootSignature::RootSignature()
	: m_rootSignatureDesc{}
	, m_numDescriptorsPerTable{0}
	, m_samplerTableBitMask(0)
	, m_descriptorTableBitMask(0)
{
	
}

//------------------------------------------------------------------------------------------------------------------
RootSignature::RootSignature(ID3D12Device10* device, std::string const& name, D3D12_ROOT_SIGNATURE_DESC1& rootSigDesc, D3D_ROOT_SIGNATURE_VERSION version)
	: m_name(name)
	, m_device(device)
	, m_rootSignatureDesc{}
	, m_samplerTableBitMask(0)
	, m_descriptorTableBitMask(0)
{
	SetRootSignatureDesc(rootSigDesc, version);
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
RootSignature::~RootSignature()
{
	Destroy();
	DX_SAFE_RELEASE(m_d3dRootSignature)
	m_device = nullptr;
}

//------------------------------------------------------------------------------------------------------------------
ID3D12RootSignature* RootSignature::GetRootSignature() const
{
	return m_d3dRootSignature;
}

//------------------------------------------------------------------------------------------------------------------
D3D12_ROOT_SIGNATURE_DESC1 const& RootSignature::GetRootSignatureDesc() const
{
	return m_rootSignatureDesc;
}

//------------------------------------------------------------------------------------------------------------------
uint32_t RootSignature::GetDescriptorTableBitMask(D3D12_DESCRIPTOR_HEAP_TYPE descriptorHeapType) const
{
	uint32_t bitMask = 0;

	switch (descriptorHeapType)
	{
		case D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV:
		{
			bitMask = m_descriptorTableBitMask;
			break;
		}
		case D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER:
		{
			bitMask = m_samplerTableBitMask;
			break;
		}
	}

	return bitMask;
}

//------------------------------------------------------------------------------------------------------------------
uint32_t RootSignature::GetNumDescriptors(uint32_t rootIndex) const
{
	GUARANTEE_OR_DIE(rootIndex < 32, "RootIndex value > 32; RootSignature::GetNumDescriptors")
	return m_numDescriptorsPerTable[rootIndex];
}

//------------------------------------------------------------------------------------------------------------------
void RootSignature::SetRootSignatureDesc(D3D12_ROOT_SIGNATURE_DESC1& rootSigDesc, D3D_ROOT_SIGNATURE_VERSION version)
{
	Destroy();
	UINT numParameters = rootSigDesc.NumParameters;
	D3D12_ROOT_PARAMETER1* parameters = numParameters > 0 ? new D3D12_ROOT_PARAMETER1[numParameters] : nullptr;

	for (UINT i = 0; i < numParameters; i++)
	{
		D3D12_ROOT_PARAMETER1 const& rootParam = rootSigDesc.pParameters[i];
		parameters[i] = rootParam;

		if (rootParam.ParameterType == D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE)
		{
			UINT numDescriptorRanges = rootParam.DescriptorTable.NumDescriptorRanges;
			D3D12_DESCRIPTOR_RANGE1* descriptorRanges = numDescriptorRanges > 0 ? new D3D12_DESCRIPTOR_RANGE1[numDescriptorRanges] : nullptr;

			memcpy(descriptorRanges, rootParam.DescriptorTable.pDescriptorRanges, sizeof(D3D12_DESCRIPTOR_RANGE1) * numDescriptorRanges);

			parameters[i].DescriptorTable.NumDescriptorRanges	= numDescriptorRanges;
			parameters[i].DescriptorTable.pDescriptorRanges		= descriptorRanges;

			if (numDescriptorRanges > 0)
			{
				switch (descriptorRanges[0].RangeType)
				{
					case D3D12_DESCRIPTOR_RANGE_TYPE_SRV:
					case D3D12_DESCRIPTOR_RANGE_TYPE_CBV:
					case D3D12_DESCRIPTOR_RANGE_TYPE_UAV:
					{
						m_descriptorTableBitMask |= (1 << i);
						break;
					}
					case D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER:
					{
						m_samplerTableBitMask |= (1 << i);
						break;
					}
				}
			}

			for (UINT j = 0; j < numDescriptorRanges; j++)
			{
				m_numDescriptorsPerTable[i] += descriptorRanges[j].NumDescriptors;
			}
		}
	}

	m_rootSignatureDesc.NumParameters	= numParameters;
	m_rootSignatureDesc.pParameters		= parameters;

	UINT numStaticSamplers = rootSigDesc.NumStaticSamplers;
	D3D12_STATIC_SAMPLER_DESC* staticSamplers = numStaticSamplers > 0 ? new D3D12_STATIC_SAMPLER_DESC[numStaticSamplers] : nullptr;

	if (staticSamplers)
	{
		memcpy(staticSamplers, rootSigDesc.pStaticSamplers, sizeof(D3D12_STATIC_SAMPLER_DESC) * numStaticSamplers);
	}

	m_rootSignatureDesc.NumStaticSamplers	= numStaticSamplers;
	m_rootSignatureDesc.pStaticSamplers		= staticSamplers;

	D3D12_ROOT_SIGNATURE_FLAGS flags = rootSigDesc.Flags;
	m_rootSignatureDesc.Flags = flags;

	D3D12_VERSIONED_ROOT_SIGNATURE_DESC versionedRootSegDesc	= {};
	versionedRootSegDesc.Version								= version;
	versionedRootSegDesc.Desc_1_1.NumParameters					= numParameters;
	versionedRootSegDesc.Desc_1_1.pParameters					= parameters;
	versionedRootSegDesc.Desc_1_1.NumStaticSamplers				= numStaticSamplers;
	versionedRootSegDesc.Desc_1_1.pStaticSamplers				= staticSamplers;
	versionedRootSegDesc.Desc_1_1.Flags							= flags;

	ID3DBlob* rootSignatureBlob = nullptr;
	ID3DBlob* errorBlob = nullptr;

	HRESULT hr;
	hr = D3D12SerializeVersionedRootSignature(&versionedRootSegDesc, &rootSignatureBlob, &errorBlob);

	if (FAILED(hr))
	{
		if (errorBlob)
		{
			ERROR_AND_DIE(Stringf("Failed to serialize root signature. Error Blob Message: %s", errorBlob->GetBufferPointer()))
		}
		else
		{
			ERROR_AND_DIE("Failed to serialize root signature. Error Blob is also null")
		}
	}
	else
	{
		if(errorBlob)
		{
			DebuggerPrintf(Stringf("==================Root Sig Error Blob Before Release: %s====================", errorBlob->GetBufferPointer()).c_str());
			errorBlob->Release();
		}
	}

	hr = m_device->CreateRootSignature(0, rootSignatureBlob->GetBufferPointer(), rootSignatureBlob->GetBufferSize(), IID_PPV_ARGS(&m_d3dRootSignature));

	if (FAILED(hr))
	{
		ERROR_AND_DIE("Failed to create root signature.")
	}

	m_d3dRootSignature->SetName(L"Root Signature");
	
	rootSignatureBlob->Release();
}

//------------------------------------------------------------------------------------------------------------------
void RootSignature::Destroy()
{
	for (UINT i = 0; i < m_rootSignatureDesc.NumParameters; i++)
	{
		D3D12_ROOT_PARAMETER1 const& rootParameter = m_rootSignatureDesc.pParameters[i];
		if (rootParameter.ParameterType == D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE)
		{
			delete[] rootParameter.DescriptorTable.pDescriptorRanges;
		}
	}

	delete[] m_rootSignatureDesc.pParameters;
	m_rootSignatureDesc.pParameters = nullptr;
	m_rootSignatureDesc.NumParameters = 0;

	delete[] m_rootSignatureDesc.pStaticSamplers;
	m_rootSignatureDesc.pStaticSamplers = nullptr;
	m_rootSignatureDesc.NumStaticSamplers = 0;

	m_descriptorTableBitMask = 0;
	m_samplerTableBitMask = 0;

	memset(m_numDescriptorsPerTable, 0, sizeof(m_numDescriptorsPerTable));
}

