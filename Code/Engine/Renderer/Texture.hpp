#pragma once

#include "Engine/Math/IntVec2.hpp"
#include "Engine/Renderer/RendererUtils.hpp"

#include "Game/EngineBuildPreferences.hpp"

#include <string>

#if defined(USING_DX12)
#include <d3d12.h>
#include <mutex>
#include <unordered_map>
#include "Engine/Renderer/DescriptorAllocationResult.hpp"
#endif 
//------------------------------------------------------------------------------------------------------------------
struct ID3D11Texture2D;
struct ID3D11ShaderResourceView;
struct ID3D11DepthStencilView;

struct D3D12_GPU_DESCRIPTOR_HANDLE;
struct ID3D12Resource2;
struct ID3D12DescriptorHeap;
struct ID3D12Device10;

class  Image;
//------------------------------------------------------------------------------------------------------------------

class Texture
{
	friend class Renderer;
	friend class DX12Renderer;
	friend class CommandList;
	friend class GBuffer;

private:

#if defined(USING_DX12)
	Texture(ID3D12Device10* device, std::string const& name);
//	explicit Texture(D3D12_RESOURCE_DESC const& resourceDesc, D3D12_CLEAR_VALUE const* clearValue, std::string const& name);
	explicit Texture(ID3D12Device10* device, ID3D12Resource* textureResource, std::string const& name);

	DescriptorAllocationResult CreateShaderResourceView(D3D12_SHADER_RESOURCE_VIEW_DESC const* srvDesc) const;
	DescriptorAllocationResult CreateUnorderedAccessView(D3D12_UNORDERED_ACCESS_VIEW_DESC const* uavDesc) const;

#else
	Texture();
#endif

	Texture(Texture const& copy) = delete;
	virtual ~Texture();

public:
	
	IntVec2						GetDimensions() const;
	std::string const&			GetImageFilePath() const;
	unsigned int				GetBindlessIndex() const;

#if defined(USING_DX12)
	ID3D12Resource*				GetTextureResource() const;
	void						CreateViews();

	D3D12_CPU_DESCRIPTOR_HANDLE GetSRV(D3D12_SHADER_RESOURCE_VIEW_DESC const* srvDesc) const;
	D3D12_CPU_DESCRIPTOR_HANDLE GetUAV(D3D12_UNORDERED_ACCESS_VIEW_DESC const* srvDesc) const;
	D3D12_CPU_DESCRIPTOR_HANDLE	GetRTV() const;
	D3D12_CPU_DESCRIPTOR_HANDLE	GetDSV() const;
	D3D12_RESOURCE_DESC			GetResourceDesc() const;

	static bool CheckRTVSupport(D3D12_FORMAT_SUPPORT1 formatSupport);
	static bool CheckUAVSupport(D3D12_FORMAT_SUPPORT1 formatSupport);
	static bool CheckSRVSupport(D3D12_FORMAT_SUPPORT1 formatSupport);
	static bool CheckDSVSupport(D3D12_FORMAT_SUPPORT1 formatSupport);

	void		Resize(uint32_t width, uint32_t height, uint32_t depthOrArraySize = 1);
	bool		IsValid() const;

#else
	ID3D11Texture2D*			GetTextureResource() const;
	ID3D11ShaderResourceView*	GetSRV() const;
	ID3D11DepthStencilView*		GetDSV() const;

#endif

protected:

	std::string						m_name;
	IntVec2							m_dimensions;

#if defined(USING_DX12)
	ID3D12Resource*					m_textureResource	= nullptr;
	D3D12_GPU_DESCRIPTOR_HANDLE*	m_srvHandle			= nullptr;
	ID3D12Device10*					m_device			= nullptr;
	D3D12_CLEAR_VALUE*				m_clearValue		= nullptr;

	unsigned int					m_bindlessIndex		= 0;

	DescriptorAllocationResult m_renderTargetView;
	DescriptorAllocationResult m_depthStencilView;

	mutable std::unordered_map<size_t, DescriptorAllocationResult> m_shaderResourceViews;
	mutable std::unordered_map<size_t, DescriptorAllocationResult> m_unorderedAccessViews;

	mutable std::mutex m_shaderResourceViewsMutex;
	mutable std::mutex m_unorderedAccessViewsMutex;
#else
	ID3D11Texture2D*				m_texture				= nullptr;
	ID3D11ShaderResourceView*		m_shaderResourceView	= nullptr;
	ID3D11DepthStencilView*			m_depthStencilView		= nullptr;

#endif
};