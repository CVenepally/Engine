#include "Engine/Renderer/Texture.hpp"
#include "Engine/Renderer/DescriptorAllocationResult.hpp"
#include "Engine/Renderer/ResourceStateTracker.hpp"
#include "Engine/Renderer/DX12Renderer.hpp"
#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Core/Image.hpp"
#include <d3d11.h>
#include <d3d12.h>

#if defined (max)
#undef max
#endif

#if defined(USING_DX12)

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
Texture::Texture(ID3D12Device10* device, std::string const& name)
	: m_device(device)
	, m_name(name)
{

}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Texture::Texture(D3D12_RESOURCE_DESC const& resourceDesc, D3D12_CLEAR_VALUE const* clearValue, std::string const& name)
// {}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
Texture::Texture(ID3D12Device10* device, ID3D12Resource* textureResource, std::string const& name)
	: m_device(device)
	, m_textureResource(textureResource)
	, m_name(name)
{

}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
DescriptorAllocationResult Texture::CreateShaderResourceView(const D3D12_SHADER_RESOURCE_VIEW_DESC* srvDesc) const
{
	DescriptorAllocationResult srv = DX12Renderer::AllocateDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	m_device->CreateShaderResourceView(m_textureResource, srvDesc, srv.GetDescriptorCPUHandle());

	return srv;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
DescriptorAllocationResult Texture::CreateUnorderedAccessView(D3D12_UNORDERED_ACCESS_VIEW_DESC const* uavDesc) const
{
	DescriptorAllocationResult uav = DX12Renderer::AllocateDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	m_device->CreateUnorderedAccessView(m_textureResource, nullptr, uavDesc, uav.GetDescriptorCPUHandle());

	return uav;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
bool Texture::IsValid() const
{
	return m_textureResource != nullptr;
}

//------------------------------------------------------------------------------------------------------------------
#else
Texture::Texture()
{

}
#endif

//------------------------------------------------------------------------------------------------------------------
Texture::~Texture()
{

#if defined(USING_DX12)
	DX_SAFE_RELEASE(m_textureResource);
	m_device = nullptr;
#else
	DX_SAFE_RELEASE(m_texture);
	DX_SAFE_RELEASE(m_shaderResourceView);
	DX_SAFE_RELEASE(m_depthStencilView);
#endif
}


//------------------------------------------------------------------------------------------------------------------
IntVec2 Texture::GetDimensions() const
{
	return m_dimensions;
}

//------------------------------------------------------------------------------------------------------------------
std::string const& Texture::GetImageFilePath() const
{
	return m_name;
}

#if defined(USING_DX12)
//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
ID3D12Resource* Texture::GetTextureResource() const
{
	return m_textureResource;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
unsigned int Texture::GetBindlessIndex() const
{
	return m_bindlessIndex;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void Texture::CreateViews()
{
	if(m_textureResource)
	{
		D3D12_RESOURCE_DESC desc = m_textureResource->GetDesc();

		D3D12_FEATURE_DATA_FORMAT_SUPPORT formatSupport = {};
		formatSupport.Format = desc.Format;

		HRESULT hr = m_device->CheckFeatureSupport(D3D12_FEATURE_FORMAT_SUPPORT, &formatSupport, sizeof(D3D12_FEATURE_DATA_FORMAT_SUPPORT));
		if(FAILED(hr))
		{
			ERROR_AND_DIE("Check Feature support failed. Texture::CreateViews");
		}

		if((desc.Flags & D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET) != 0 && CheckRTVSupport(formatSupport.Support1))
		{
			m_renderTargetView = DX12Renderer::AllocateDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
			m_device->CreateRenderTargetView(m_textureResource, nullptr, m_renderTargetView.GetDescriptorCPUHandle());
		}

		if((desc.Flags & D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL) != 0 && CheckDSVSupport(formatSupport.Support1))
		{
			m_depthStencilView = DX12Renderer::AllocateDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
			m_device->CreateDepthStencilView(m_textureResource, nullptr, m_depthStencilView.GetDescriptorCPUHandle());
		}
	}

	std::lock_guard<std::mutex> lock(m_shaderResourceViewsMutex);
	std::lock_guard<std::mutex> guard(m_unorderedAccessViewsMutex);

	m_shaderResourceViews.clear();
	m_unorderedAccessViews.clear();
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
D3D12_CPU_DESCRIPTOR_HANDLE Texture::GetSRV(D3D12_SHADER_RESOURCE_VIEW_DESC const* srvDesc) const
{
	std::size_t hash = 0;
	if(srvDesc)
	{
		hash = std::hash<D3D12_SHADER_RESOURCE_VIEW_DESC>{}(*srvDesc);
	}

	std::lock_guard<std::mutex> lock(m_shaderResourceViewsMutex);

	auto iter = m_shaderResourceViews.find(hash);
	if(iter == m_shaderResourceViews.end())
	{
		auto srv = CreateShaderResourceView(srvDesc);
		iter = m_shaderResourceViews.insert({hash, std::move(srv)}).first;
	}

	return iter->second.GetDescriptorCPUHandle();
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
D3D12_CPU_DESCRIPTOR_HANDLE Texture::GetUAV(D3D12_UNORDERED_ACCESS_VIEW_DESC const* uavDesc) const
{
	std::size_t hash = 0;

	if(uavDesc)
	{
		hash = std::hash<D3D12_UNORDERED_ACCESS_VIEW_DESC>{}(*uavDesc);
	}

	std::lock_guard<std::mutex> guard(m_unorderedAccessViewsMutex);

	auto iter = m_unorderedAccessViews.find(hash);
	if(iter == m_unorderedAccessViews.end())
	{
		auto uav = CreateUnorderedAccessView(uavDesc);
		iter = m_unorderedAccessViews.insert({hash, std::move(uav)}).first;
	}

	return iter->second.GetDescriptorCPUHandle();
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
D3D12_CPU_DESCRIPTOR_HANDLE Texture::GetRTV() const
{
	return m_renderTargetView.GetDescriptorCPUHandle();
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
D3D12_CPU_DESCRIPTOR_HANDLE Texture::GetDSV() const
{
	return m_depthStencilView.GetDescriptorCPUHandle();
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
bool Texture::CheckRTVSupport(D3D12_FORMAT_SUPPORT1 formatSupport)
{
	return ((formatSupport & D3D12_FORMAT_SUPPORT1_RENDER_TARGET) != 0);
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
bool Texture::CheckUAVSupport(D3D12_FORMAT_SUPPORT1 formatSupport)
{
	return ((formatSupport & D3D12_FORMAT_SUPPORT1_TYPED_UNORDERED_ACCESS_VIEW) != 0);
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
bool Texture::CheckSRVSupport(D3D12_FORMAT_SUPPORT1 formatSupport)
{
	return ((formatSupport & D3D12_FORMAT_SUPPORT1_SHADER_SAMPLE) != 0 ||
			(formatSupport & D3D12_FORMAT_SUPPORT1_SHADER_LOAD) != 0);
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
bool Texture::CheckDSVSupport(D3D12_FORMAT_SUPPORT1 formatSupport)
{
	return ((formatSupport & D3D12_FORMAT_SUPPORT1_DEPTH_STENCIL) != 0);
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void Texture::Resize(uint32_t width, uint32_t height, uint32_t depthOrArraySize)
{
	if(!m_textureResource)
	{
		return;
	}

	D3D12_RESOURCE_DESC desc = m_textureResource->GetDesc();
	desc.Width = std::max(width, 1u);
	desc.Height = std::max(height, 1u);
	desc.DepthOrArraySize = static_cast<UINT16>(depthOrArraySize);

	D3D12_HEAP_PROPERTIES heapProp	= {};
	heapProp.Type					= D3D12_HEAP_TYPE_DEFAULT;
	heapProp.CreationNodeMask		= 1;
	heapProp.VisibleNodeMask		= 1;

	ResourceStateTracker::RemoveGlobalResourceState(m_textureResource);

	HRESULT hr = m_device->CreateCommittedResource(&heapProp, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_COMMON, m_clearValue, IID_PPV_ARGS(&m_textureResource));
	if(FAILED(hr))
	{
		ERROR_AND_DIE("Create Committed Resources failed. Texture::Resize");
	}

	m_textureResource->SetName(L"Texture Resource");

	ResourceStateTracker::AddGlobalResourceState(m_textureResource, D3D12_RESOURCE_STATE_COMMON);

	CreateViews();
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
D3D12_RESOURCE_DESC Texture::GetResourceDesc() const
{
	if(!m_textureResource)
	{
		return D3D12_RESOURCE_DESC();
	}

	return m_textureResource->GetDesc();
}

#else
//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
ID3D11Texture2D* Texture::GetTextureResource() const
{
	return m_texture;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
ID3D11ShaderResourceView* Texture::GetSRV() const
{
	return m_shaderResourceView;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
ID3D11DepthStencilView* Texture::GetDSV() const
{
	return m_depthStencilView;
}

#endif

