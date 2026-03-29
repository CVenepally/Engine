#pragma once
#include "Engine/Renderer/DX12Renderer.hpp"
#include "Game/EngineBuildPreferences.hpp"
#include <d3d12.h>

#if defined(USING_DX12)

class Shader;

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
struct PSS_RootSignature
{
	D3D12_PIPELINE_STATE_SUBOBJECT_TYPE m_subObjectType = D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_ROOT_SIGNATURE;
	ID3D12RootSignature* m_rootSignature = nullptr;
};

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
struct PSS_RasterizerState
{
	D3D12_PIPELINE_STATE_SUBOBJECT_TYPE m_subObjectType = D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_RASTERIZER;
	D3D12_RASTERIZER_DESC m_rasterizerDesc;
};

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
struct PSS_BlendState
{
	D3D12_PIPELINE_STATE_SUBOBJECT_TYPE m_subObjectType = D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_BLEND;
	D3D12_BLEND_DESC m_blendDesc;
};

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
struct PSS_DepthStencilState
{
	D3D12_PIPELINE_STATE_SUBOBJECT_TYPE m_subObjectType = D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_DEPTH_STENCIL;
	D3D12_DEPTH_STENCIL_DESC m_desc;
};

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
struct PSS_InputLayout
{
	D3D12_PIPELINE_STATE_SUBOBJECT_TYPE m_subObjectType = D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_INPUT_LAYOUT;
	D3D12_INPUT_LAYOUT_DESC m_inputLayoutDesc;
};

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
struct PSS_PrimitiveTopologyType
{
	D3D12_PIPELINE_STATE_SUBOBJECT_TYPE m_subObjectType = D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_PRIMITIVE_TOPOLOGY;
	D3D12_PRIMITIVE_TOPOLOGY_TYPE m_primitiveTopologyType;
};

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
struct PSS_VertexShaderByteCode
{
	D3D12_PIPELINE_STATE_SUBOBJECT_TYPE m_subObjectType = D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_VS;
	D3D12_SHADER_BYTECODE m_vertexShaderByteCode;
};

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
struct PSS_PixelShaderByteCode
{
	D3D12_PIPELINE_STATE_SUBOBJECT_TYPE m_subObjectType = D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_PS;
	D3D12_SHADER_BYTECODE m_pixelShaderByteCode;
};

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
struct PSS_DepthStencilFormat
{
	D3D12_PIPELINE_STATE_SUBOBJECT_TYPE subObjectType = D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_DEPTH_STENCIL_FORMAT;
	DXGI_FORMAT m_format = DXGI_FORMAT_D32_FLOAT;
};

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
struct PSS_RenderTargetFormats
{
	D3D12_PIPELINE_STATE_SUBOBJECT_TYPE subObjectType = D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_RENDER_TARGET_FORMATS;
	D3D12_RT_FORMAT_ARRAY m_formatArray;
};

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
struct PipelineStateStream
{
public:

	PSS_RootSignature			m_rootSignatureSubObject;
	PSS_InputLayout				m_inputLayoutSubObject;
	PSS_PrimitiveTopologyType	m_primitiveTopologyTypeSubObject;
	PSS_VertexShaderByteCode	m_vsByteCodeSubObject;
	PSS_PixelShaderByteCode		m_psByteCodeSubObject;
	PSS_DepthStencilState		m_depthStencilStateSubObject;
	PSS_DepthStencilFormat		m_dsvFormatSubObject;
	PSS_RenderTargetFormats		m_rtvFormatsSubObject;
	PSS_RasterizerState			m_rasterizerStateSubObject;
	PSS_BlendState				m_blendStateSubObject;
};
#endif