#define WIN32_LEAN_AND_MEAN

#include <windows.h>

#include <d3d12.h>
#include <dxgi1_6.h>
#include <d3dcompiler.h>
#include <d3d12sdklayers.h>

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "d3dcompiler.lib")

#include "Engine/Renderer/DX12Renderer.hpp"
#include "Engine/Renderer/CommandList.hpp"
#include "Engine/Renderer/CommandQueue.h"
#include "Engine/Renderer/Camera.hpp" 
#include "Engine/Renderer/VertexBuffer.hpp" 
#include "Engine/Renderer/IndexBuffer.hpp" 
#include "Engine/Renderer/StructuredBuffer.hpp" 
#include "Engine/Renderer/Shader.hpp" 
#include "Engine/Renderer/RootSignature.hpp" 
#include "Engine/Renderer/PipelineStateObject.hpp" 
#include "Engine/Renderer/ConstantBuffer.hpp" 
#include "Engine/Renderer/Texture.hpp"
#include "Engine/Renderer/PipelineStateStream.hpp"
#include "Engine/Renderer/BitmapFont.hpp"
#include "Engine/Renderer/Light.hpp"
#include "Engine/Renderer/ResourceStateTracker.hpp"
#include "Engine/Renderer/DescriptorAllocationResult.hpp"
#include "Engine/Renderer/DescriptorAllocator.hpp"
#include "Engine/Renderer/RenderTarget.hpp"
#include "Engine/Renderer/ShaderCompiler.hpp"
#include "Engine/Renderer/RayTracingUtils.hpp"
#include "Engine/Renderer/ShaderRecord.hpp"
#include "Engine/Renderer/ShaderTable.hpp"
#include "Engine/Renderer/BottomLevelAS.hpp"
#include "Engine/Renderer/TopLevelAS.hpp"
#include "Engine/Renderer/MaterialInfo.hpp"
#include "Engine/Renderer/BindlessDescriptorHeap.hpp"
#include "Engine/Renderer/ImGuiHeap.hpp"
#include "Engine/Renderer/GBuffer.hpp"

#include "Engine/Core/Vertex_PCU.hpp" 
#include "Engine/Core/Vertex_PCUTBN.hpp" 
#include "Engine/Core/VertexUtils.hpp"
#include "Engine/Core/StringUtils.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Core/EngineCommon.hpp"

#include "Engine/Math/Vec4.hpp"
#include "Engine/Window/Window.hpp"

#include "ThirdParty/d3dx12/d3dx12.h"
#include "ThirdParty/PIX/pix3.h"
#include "ThirdParty/imgui/imgui.h"
#include "ThirdParty/imgui/imgui_impl_dx12.h"

#include "Game/EngineBuildPreferences.hpp"

#pragma comment(lib, "ThirdParty/PIX/WinPixEventRuntime.lib")

#if defined(ENGINE_DEBUG_RENDER)
	#include <dxgidebug.h>
	#pragma comment(lib, "dxguid.lib")
#endif


#if defined(USING_DX12)


//Constant Buffers--------------------------------------------------------------------------------------------------------------------------------------------------
struct CameraConstants
{
	Mat44	cb_worldToCameraTransform;
	Mat44	cb_cameraToRenderTransform;
	Mat44	cb_renderToClipTransform;
	Vec3	cb_CameraPosition;
	float	padding0;
};
static const int k_cameraConstantsRegister = 2;
static const int k_cameraConstantsSpace = 0;

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
struct ModelConstants
{
	Mat44	cb_modelTransform;	// 64bytes / aligned
	Vec4	cb_modelTint;		// 16 bytes / aligned
};
static const int k_modelConstantRegister	= 3;
static const int k_modelConstantSpace		= 0;

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
struct LightConstants
{
	Light cb_directionalLight;

	Light cb_allLights[256];

	int	  cb_numLights;
	float cb_ambientIntensity;
	Vec2  padding1;
};
static const int k_lightConstantRegister = 4;
static const int k_lightConstantSpace = 0;


//------------------------------------------------------------------------------------------------------------------
uint64_t				DX12Renderer::s_frameCount		= 0;
DescriptorAllocator*	DX12Renderer::s_descriptorAllocators[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES] = {};
ImGuiHeap*				DX12Renderer::s_imGuiHeap = nullptr;

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
RTCameraConstants		g_rtCameraConsts;
LightConstants			g_rtLightConsts;
SceneConstants			g_rtSceneConsts;
DebugInfo				g_rtDebugInfo;
AppSettings				g_rtAppSettings;

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
DX12Renderer::DX12Renderer(DX12RendererConfig const& config)
	: m_config(config)
{	
	g_rtAppSettings.m_enableJitter				= m_config.m_enableJitter;
	g_rtAppSettings.m_enableFrameAccumulation	= m_config.m_enableFrameAccumulation;
	g_rtAppSettings.m_frameCount				= 0;
	g_rtAppSettings.m_accumCount				= 0;
	g_rtAppSettings.m_maxSamples				= 32;
	g_rtAppSettings.m_minBounces				= m_config.m_minRayBounces;
	g_rtAppSettings.m_doDirect					= true;
	g_rtAppSettings.m_doIndirect				= true;

	g_rtAppSettings.m_maxFramesToAccumulate				= -1;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
DX12Renderer::~DX12Renderer()
{
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void DX12Renderer::BeginStartup()
{
	InitializeDebugInterfaces();
	CreateFactory();
	FetchAdapter();
	CreateDevice();
	CreateCommandQueues();

	m_shaderCompiler = new ShaderCompiler();

	for(int i = 0; i < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES; ++i)
	{
		s_descriptorAllocators[i] = new DescriptorAllocator(m_device, static_cast<D3D12_DESCRIPTOR_HEAP_TYPE>(i));
	}

	CreateSwapChainAndGetBackBufferIndex();
	CreateRenderTargetTextures();
	UpdateRenderTargetViews();

	m_defaultShader		= CreateOrGetShader("Default");
	m_compositeShader	= CreateOrGetShader("Composite");

	CreateDefaultRootSignature();
	CreateDefaultPSO();
	CreateSRVBindlessHeap();

	if(m_config.m_enableRayTracing)
	{
		m_temporalReuseShader	= CreateOrGetShader("Data/Shaders/TemporalReuse", InputLayoutType::VERTEX_PCUTBN, true, SHADER_COMPUTE);
		m_spatialReuseShader	= CreateOrGetShader("Data/Shaders/SpatialReuse", InputLayoutType::VERTEX_PCUTBN, true, SHADER_COMPUTE);
		m_denoiseShader			= CreateOrGetShader("Data/Shaders/Denoiser", InputLayoutType::VERTEX_PCUTBN, true, SHADER_COMPUTE);
		
		CreateRayTracingRootSignatures();
		CreateRaytracingRenderTargets();
		CreateReservoirBuffers();
		CreateRTPSOs();
		m_gBuffers = new GBuffer(m_device, m_graphicsCommandList);
	}

	m_currentPSO = m_defaultPSO;

	Image defaultTextureImage = Image(IntVec2(1, 1), Rgba8::WHITE);

	m_defaultTexture = m_copyCommandList->CreateTextureFromImage(defaultTextureImage);
	ResizeDepthBuffer(g_theWindow->GetClientDimensions().x, g_theWindow->GetClientDimensions().y);

	s_imGuiHeap = new ImGuiHeap(m_device);
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void DX12Renderer::EndStartup()
{
	InitImGui();
	
	uint64_t directFenceValue	= m_directCommandQueue->ExecuteCommandList(m_graphicsCommandList);
	uint64_t copyFenceValue		= m_copyCommandQueue->ExecuteCommandList(m_copyCommandList);
	uint64_t computeFenceValue	= m_computeCommandQueue->ExecuteCommandList(m_computeCommandList);

	m_directCommandQueue->WaitForFenceValue(directFenceValue);
	m_copyCommandQueue->WaitForFenceValue(copyFenceValue);
	m_computeCommandQueue->WaitForFenceValue(computeFenceValue);
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void DX12Renderer::Shutdown()
{
 	ImGui_ImplDX12_Shutdown();
	
	delete s_imGuiHeap;

	for(int index = 0; index < static_cast<int>(m_loadedShaders.size()); ++index)
	{
		delete m_loadedShaders[index];
		m_loadedShaders[index] = nullptr;
	}

	delete m_defaultRootSignature;

	if(m_config.m_enableRayTracing)
	{
		if(m_hitGroupShaderTable)
		{
			delete m_hitGroupShaderTable;
			m_hitGroupShaderTable = nullptr;
		}

		if(m_missShaderTable)
		{
			delete m_missShaderTable;
			m_missShaderTable = nullptr;
		}

		if(m_rayGenShaderTable)
		{
			delete m_rayGenShaderTable;
			m_rayGenShaderTable = nullptr;
		}

		if(m_rayGenShaderTable_gBuffer)
		{
			delete m_rayGenShaderTable_gBuffer;
			m_rayGenShaderTable_gBuffer = nullptr;
		}

		if(m_missShaderTable_gBuffer)
		{
			delete m_missShaderTable_gBuffer;
			m_missShaderTable_gBuffer = nullptr;
		}

		if(m_hitGroupShaderTable_gBuffer)
		{
			delete m_hitGroupShaderTable_gBuffer;
			m_hitGroupShaderTable_gBuffer = nullptr;
		}

		delete m_globalRTRootSignature;
		m_globalRTRootSignature = nullptr;
	}
	
	for(int index = 0; index < static_cast<int>(m_loadedPSOs.size()); ++index)
	{
		delete m_loadedPSOs[index];
		m_loadedPSOs[index] = nullptr;
	}

	if(m_config.m_enableRayTracing)
	{
		m_rtPSO->Release();
		m_rtPSO = nullptr;
	
		m_gBufferPSO->Release();
		m_gBufferPSO = nullptr;

		delete m_temporalReusePSO;
		delete m_spatialReusePSO;
		delete m_denoiserPSO;

		m_temporalReusePSO = nullptr;
		m_spatialReusePSO = nullptr;
		m_denoiserPSO = nullptr;
	}

	for(int index = 0; index < m_numBuffers; index++)
	{
		delete m_backBuffers[index];
	}

	delete m_compositeDepthBuffer;

	if(m_renderTarget)
	{
		for(Texture* rtTexture : m_renderTarget->m_textures)
		{
			if(rtTexture)
			{
				delete rtTexture;
				rtTexture = nullptr;
			}
		}

		m_renderTarget->m_textures.clear();
		delete m_renderTarget;
		m_renderTarget = nullptr;
	}
	
	if(m_noisyRenderTarget)
	{
		delete m_noisyRenderTarget;
		m_noisyRenderTarget = nullptr;
	}

	if(m_temporalReservoirBuffer)
	{
		delete m_temporalReservoirBuffer;
		m_temporalReservoirBuffer = nullptr;
	}

	if(m_finalReservoirBuffer)
	{
		delete m_finalReservoirBuffer;
		m_finalReservoirBuffer = nullptr;
	}

	if(m_prevReservoirBuffer)
	{
		delete m_prevReservoirBuffer;
		m_prevReservoirBuffer = nullptr;
	}

	delete m_gBuffers;

	DX_SAFE_RELEASE(m_swapChain)
	delete m_directCommandQueue;
	delete m_copyCommandQueue;
	delete m_computeCommandQueue;

	for(DescriptorAllocator* descriptorAllocator : s_descriptorAllocators)
	{
		if(descriptorAllocator)
		{
			delete descriptorAllocator;
			descriptorAllocator = nullptr;
		}
	}

	delete m_bindlessSRVHeap;

	DX_SAFE_RELEASE(m_device)
	DX_SAFE_RELEASE(m_adapter)
	DX_SAFE_RELEASE(m_factory)

#if defined(ENGINE_DEBUG_RENDER)
	((IDXGIDebug*)m_dxgiDebug)->ReportLiveObjects(DXGI_DEBUG_ALL, (DXGI_DEBUG_RLO_FLAGS)(DXGI_DEBUG_RLO_DETAIL | DXGI_DEBUG_RLO_IGNORE_INTERNAL));

	((IDXGIDebug*)m_dxgiDebug)->Release();
	m_dxgiDebug = nullptr;

	::FreeLibrary((HMODULE)m_dxgiDebugModule);
	m_dxgiDebugModule = nullptr;

	::FreeLibrary((HMODULE)m_pixLib);
	m_pixLib = nullptr;

#endif

}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void DX12Renderer::WaitForRendererToFinish()
{
	m_directCommandQueue->Flush();
	m_copyCommandQueue->Flush();
	m_computeCommandQueue->Flush();
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void DX12Renderer::BeginFrame()
{
	ImGui_ImplDX12_NewFrame();
	ImGui::NewFrame();

	m_graphicsCommandList	= m_directCommandQueue->GetCommandList();
	m_copyCommandList		= m_copyCommandQueue->GetCommandList();
	m_computeCommandList	= m_computeCommandQueue->GetCommandList();

	m_graphicsCommandList->SetRenderTarget(*m_renderTarget);

	m_graphicsCommandList->SetGraphicsRootSignature(*m_defaultRootSignature);
	m_graphicsCommandList->SetPipelineState(*m_currentPSO);

	if(m_config.m_enableRayTracing)
	{
		m_graphicsCommandList->SetComputeRootSignature(*m_globalRTRootSignature);
	}
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void DX12Renderer::EndFrame()
{
	m_graphicsCommandList->m_d3dCommandList->SetDescriptorHeaps(1, &s_imGuiHeap->m_imGuiHeap);
	ImGui::Render();
	ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), m_graphicsCommandList->m_d3dCommandList);

	m_copyCommandQueue->ExecuteCommandList(m_copyCommandList);
	m_directCommandQueue->ExecuteCommandList(m_graphicsCommandList);
	m_computeCommandQueue->ExecuteCommandList(m_computeCommandList);
	
	m_graphicsCommandList = m_directCommandQueue->GetCommandList();

	CompositeRenderTargetOutputs();
	
	Texture* backBuffer = m_backBuffers[m_currentBackBufferIndex];

	m_graphicsCommandList->TransitionBarrier(backBuffer->m_textureResource, D3D12_RESOURCE_STATE_PRESENT);
	m_directCommandQueue->ExecuteCommandList(m_graphicsCommandList);

	UINT syncInterval = m_config.m_vSync ? 1 : 0;
	UINT presentFlags = m_config.m_tearingSupported && !m_config.m_vSync ? DXGI_PRESENT_ALLOW_TEARING : 0;
	
	HRESULT hr;
	hr = m_swapChain->Present(syncInterval, presentFlags);
	
	if(!SUCCEEDED(hr))
	{
		ERROR_AND_DIE("Present call failed! App:Render()")
	}

	m_frameFenceValues[m_currentBackBufferIndex] = static_cast<unsigned int>(m_directCommandQueue->Signal());
	m_frameValues[m_currentBackBufferIndex] = s_frameCount;

	m_currentBackBufferIndex = m_swapChain->GetCurrentBackBufferIndex();

	m_directCommandQueue->WaitForFenceValue(m_frameFenceValues[m_currentBackBufferIndex]);

	ReleaseStaleDescriptors(m_frameValues[m_currentBackBufferIndex]);

	m_bindlessSRVHeap->Reset();
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
uint64_t DX12Renderer::UpdateFrameCount()
{
	++s_frameCount;
	g_rtAppSettings.m_frameCount = s_frameCount;
	return s_frameCount;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void DX12Renderer::BeginCamera(Camera const& camera)
{
	D3D12_VIEWPORT viewport = {};
	viewport.TopLeftX		= 0.f;
	viewport.TopLeftY		= 0.f;
	viewport.Width			= static_cast<float>(camera.m_viewportBounds.m_maxs.x);
	viewport.Height			= static_cast<float>(camera.m_viewportBounds.m_maxs.y);
	viewport.MinDepth		= 0.f;
	viewport.MaxDepth		= 1.f;

	D3D12_RECT scissorRect	= {};
	scissorRect.top			= 0;
	scissorRect.left		= 0;
	scissorRect.bottom		= LONG_MAX;
	scissorRect.right		= LONG_MAX;
	
	m_graphicsCommandList->SetViewport(viewport);
	m_graphicsCommandList->SetScissorRect(scissorRect);

	CameraConstants cameraConsts;
	cameraConsts.cb_worldToCameraTransform	= camera.GetWorldToCameraTransform();
	cameraConsts.cb_cameraToRenderTransform = camera.GetCameraToRenderTransform();
	cameraConsts.cb_renderToClipTransform	= camera.GetRenderToClipTransform();
	cameraConsts.cb_CameraPosition			= camera.m_position;

	m_graphicsCommandList->SetGraphicsDynamicConstantBuffer(static_cast<int>(RootParameters::CAMERA_CB), cameraConsts);		
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void DX12Renderer::BeginRTCamera(Camera const& camera)
{
	g_rtCameraConsts.m_cameraToWorld = camera.GetCameraToWorldTransform();
	g_rtCameraConsts.m_renderToCamera = camera.GetRenderToCameraTransform();
	g_rtCameraConsts.m_clipToRender = camera.GetClipToRenderTransform();
	g_rtCameraConsts.m_cameraPosition = Vec4(camera.m_position, 0.f);
	g_rtCameraConsts.m_currentFrameWorldToCamera = camera.GetWorldToCameraTransform();
	g_rtCameraConsts.m_cameraToRender = camera.GetCameraToRenderTransform();
	g_rtCameraConsts.m_renderToClip = camera.GetRenderToClipTransform();
	g_rtCameraConsts.m_screenDims = Vec2(m_config.m_window->GetClientDimensions());

// 	m_graphicsCommandList->SetComputeDynamicConstantBuffer(static_cast<int>(GlobalRTRootSignatureParameters::CAMERA_CB), rtCameraConsts);
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void DX12Renderer::EndRTCamera(Camera const& camera)
{
	UNUSED(camera)
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void DX12Renderer::EndCamera(Camera const& camera)
{
	UNUSED(camera)
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
VertexBuffer* DX12Renderer::CreateVertexBuffer(unsigned int numElements, void const* vertexData, InputLayoutType vertexType, bool useBindless)
{
 	VertexBuffer* vbo = new VertexBuffer();
	
	size_t stride = sizeof(Vertex_PCU);

	switch(vertexType)
	{
		case InputLayoutType::VERTEX_P:
		{
			stride = sizeof(Vec3);
			break;
		}
		case InputLayoutType::VERTEX_PCU:
		{
			stride = sizeof(Vertex_PCU);
			break;
		}
		case InputLayoutType::VERTEX_PCUTBN:
		{
			stride = sizeof(Vertex_PCUTBN);
			break;
		}
		case InputLayoutType::COUNT:
			break;
		default:
			break;
	}

	m_graphicsCommandList->CopyVertexBuffer(vbo, numElements, stride, vertexData);

	if(useBindless)
	{
		D3D12_RESOURCE_STATES stateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;

		m_graphicsCommandList->TransitionBarrier(vbo->m_buffer, stateAfter);

		D3D12_SHADER_RESOURCE_VIEW_DESC vbSrvDesc = {};
		vbSrvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
		vbSrvDesc.Format = DXGI_FORMAT_UNKNOWN;
		vbSrvDesc.Buffer.FirstElement = 0;
		vbSrvDesc.Buffer.NumElements = static_cast<UINT>(vbo->m_numElements);
		vbSrvDesc.Buffer.StructureByteStride = static_cast<UINT>(vbo->m_sizeOfEachElement);
		vbSrvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
		vbSrvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

		PersistentDescriptorAllocation vbSrvAlloc = m_bindlessSRVHeap->AllocatePersistent();
		vbo->m_bindlessIndex = vbSrvAlloc.m_index;

		for(uint32_t i = 0; i < m_bindlessSRVHeap->m_numHeaps; ++i)
		{
			m_device->CreateShaderResourceView(vbo->m_buffer, &vbSrvDesc, vbSrvAlloc.m_handles[i]);
		}
	}
	
 	return vbo;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
IndexBuffer* DX12Renderer::CreateIndexBuffer(unsigned int numElements, const void* indexData, bool useBindless)
{
 	IndexBuffer* ibo = new IndexBuffer();

	m_graphicsCommandList->CopyIndexBuffer(ibo, numElements, DXGI_FORMAT_R32_UINT, indexData);

	if(useBindless)
	{
		D3D12_RESOURCE_STATES stateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;

		m_graphicsCommandList->TransitionBarrier(ibo->m_buffer, stateAfter);

		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
		srvDesc.Format = DXGI_FORMAT_UNKNOWN;
		srvDesc.Buffer.FirstElement = 0;
		srvDesc.Buffer.NumElements = static_cast<UINT>(ibo->m_numElements);
		srvDesc.Buffer.StructureByteStride = static_cast<UINT>(ibo->m_sizeOfEachElement);
		srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

		PersistentDescriptorAllocation ibSrvAlloc = m_bindlessSRVHeap->AllocatePersistent();
		ibo->m_bindlessIndex = ibSrvAlloc.m_index;

		for(uint32_t i = 0; i < m_bindlessSRVHeap->m_numHeaps; ++i)
		{
			m_device->CreateShaderResourceView(ibo->m_buffer, &srvDesc, ibSrvAlloc.m_handles[i]);
		}
	}

 	return ibo;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
StructuredBuffer* DX12Renderer::CreateStructuredBuffer(unsigned int numElements, unsigned int elementSize, const void* bufferData)
{
	StructuredBuffer* buffer = new StructuredBuffer(m_device);
	buffer->m_numElements = numElements;
	buffer->m_sizeOfEachElement = elementSize;

	buffer->m_buffer = m_graphicsCommandList->CreateBufferResource(numElements, elementSize, bufferData);
	
	D3D12_RESOURCE_STATES stateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;

	m_graphicsCommandList->TransitionBarrier(buffer->m_buffer, stateAfter);

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
	srvDesc.Format = DXGI_FORMAT_UNKNOWN;
	srvDesc.Buffer.FirstElement = 0;
	srvDesc.Buffer.NumElements = static_cast<UINT>(buffer->m_numElements);
	srvDesc.Buffer.StructureByteStride = static_cast<UINT>(buffer->m_sizeOfEachElement);
	srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

	PersistentDescriptorAllocation ibSrvAlloc = m_bindlessSRVHeap->AllocatePersistent();
	buffer->m_bindlessIndex = ibSrvAlloc.m_index;

	for(uint32_t i = 0; i < m_bindlessSRVHeap->m_numHeaps; ++i)
	{
		m_device->CreateShaderResourceView(buffer->m_buffer, &srvDesc, ibSrvAlloc.m_handles[i]);
	}

	return buffer;
}

//------------------------------------------------------------------------------------------------------------------
uint64_t DX12Renderer::GetCurrentFrameNumber()
{
	return s_frameCount;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
std::string DX12Renderer::MakePSOName(RootSignature* rootSignature, Shader* shader, InputLayoutType inputLayout, RasterizerMode rasterMode, BlendMode blendMode, DepthMode depthMode, bool usingDSV)
{
	std::string psoName;

	if(rootSignature)
	{
		psoName += rootSignature->m_name;
	}
	else
	{
		psoName += "Default";
	}

	if(shader)
	{
		psoName += shader->GetName();
	}
	else
	{
		psoName += "Default";
	}

	std::string inputLayoutName = GetInputLayoutName(inputLayout);
	psoName += inputLayoutName;

	std::string rasterModeName	= GetRasterModeName(rasterMode);
	psoName += rasterModeName;

	std::string blendModeName	= GetBlendModeName(blendMode);
	psoName += blendModeName;

	std::string depthModeName	= GetDepthModeName(depthMode);
	psoName += depthModeName;

	std::string usingDepth = usingDSV ? "UsingDSV" : "NoDSV";
	psoName += usingDepth;

	return psoName;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
std::string DX12Renderer::GetInputLayoutName(InputLayoutType inputLayout)
{
	switch(inputLayout)
	{
		case InputLayoutType::VERTEX_PCU:		return "VPCU";
		case InputLayoutType::VERTEX_PCUTBN:	return "VPCUTBN";
	}

	return "Unknown";
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
std::string DX12Renderer::GetRasterModeName(RasterizerMode rasterMode)
{
	switch(rasterMode)
	{
		case RasterizerMode::SOLID_CULL_BACK:		return "SOLID_CULL_BACK";
		case RasterizerMode::SOLID_CULL_NONE:		return "SOLID_CULL_NONE";
		case RasterizerMode::WIREFRAME_CULL_BACK:	return "WIREFRAME_CULL_BACK";
		case RasterizerMode::WIREFRAME_CULL_NONE:	return "WIREFRAME_CULL_NONE";
	}

	return "Unknown";
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
std::string DX12Renderer::GetBlendModeName(BlendMode blendMode)
{
	switch(blendMode)
	{
		case BlendMode::ALPHA:		return "ALPHA";
		case BlendMode::ADDITIVE:	return "ADDITIVE";
		case BlendMode::OPAQUE:		return "OPAQUE";
	}

	return "Unknown";

}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
std::string DX12Renderer::GetDepthModeName(DepthMode depthMode) 
{
	switch(depthMode)
	{
		case DepthMode::DISABLED:				return "DISABLED";
		case DepthMode::READ_ONLY_ALWAYS:		return "READ_ONLY_ALWAYS";
		case DepthMode::READ_ONLY_LESS_EQUAL:	return "READ_ONLY_LESS_EQUAL";
		case DepthMode::READ_WRITE_LESS_EQUAL:	return "READ_WRITE_LESS_EQUAL";
	}

	return "Unknown";
}

//------------------------------------------------------------------------------------------------------------------
D3D12_RASTERIZER_DESC DX12Renderer::GetRasterizerDesc(RasterizerMode mode)
{
	D3D12_RASTERIZER_DESC rasterDesc	= {};
	rasterDesc.FrontCounterClockwise	= TRUE;
	rasterDesc.DepthBias				= 0;
	rasterDesc.DepthBiasClamp			= 0.f;
	rasterDesc.SlopeScaledDepthBias		= 0.f;
	rasterDesc.DepthClipEnable			= TRUE;
	rasterDesc.MultisampleEnable		= FALSE;
	rasterDesc.AntialiasedLineEnable	= TRUE;
	rasterDesc.ForcedSampleCount		= 0;
	rasterDesc.ConservativeRaster		= D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

	switch (mode)
	{
		case RasterizerMode::SOLID_CULL_BACK:
		{
			rasterDesc.FillMode = D3D12_FILL_MODE_SOLID;
			rasterDesc.CullMode = D3D12_CULL_MODE_BACK;
			break;	
		}
		case RasterizerMode::SOLID_CULL_NONE:
		{
			rasterDesc.FillMode = D3D12_FILL_MODE_SOLID;
			rasterDesc.CullMode = D3D12_CULL_MODE_NONE;
			break;
		}
		case RasterizerMode::WIREFRAME_CULL_BACK:
		{
			rasterDesc.FillMode = D3D12_FILL_MODE_WIREFRAME;
			rasterDesc.CullMode = D3D12_CULL_MODE_BACK;
			break;
		}
		case RasterizerMode::WIREFRAME_CULL_NONE:
		{
			rasterDesc.FillMode = D3D12_FILL_MODE_WIREFRAME;
			rasterDesc.CullMode = D3D12_CULL_MODE_NONE;
			break;
		}
	}

	return rasterDesc;
}

//------------------------------------------------------------------------------------------------------------------
D3D12_INPUT_LAYOUT_DESC DX12Renderer::GetInputLayoutDesc(InputLayoutType inputLayout)
{
	D3D12_INPUT_LAYOUT_DESC inputLayoutDesc = {};
	
	switch (inputLayout)
	{
		case InputLayoutType::VERTEX_PCU:
		{
			static D3D12_INPUT_ELEMENT_DESC inputElementDesc[3] = {};
			inputElementDesc[0] = { "POSITION",	0, DXGI_FORMAT_R32G32B32_FLOAT,	0,		D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };	
			inputElementDesc[1] = { "COLOR",		0, DXGI_FORMAT_R8G8B8A8_UNORM,	0,	D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };	
			inputElementDesc[2] = { "TEXCOORD",	0, DXGI_FORMAT_R32G32_FLOAT,    0,		D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };

			inputLayoutDesc.pInputElementDescs	= inputElementDesc;
			inputLayoutDesc.NumElements			= _countof(inputElementDesc);
			break;
		}
		case InputLayoutType::VERTEX_PCUTBN:
		{
			static D3D12_INPUT_ELEMENT_DESC inputElementDesc[6] = {};
			inputElementDesc[0] = { "POSITION",	0, DXGI_FORMAT_R32G32B32_FLOAT,	0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };
			inputElementDesc[1] = { "COLOR",		0, DXGI_FORMAT_R8G8B8A8_UNORM,	0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };
			inputElementDesc[2] = { "TEXCOORD",	0, DXGI_FORMAT_R32G32_FLOAT,    0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };
			inputElementDesc[3] = { "TANGENT",	0, DXGI_FORMAT_R32G32B32_FLOAT,	0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };
			inputElementDesc[4] = { "BITANGENT",	0, DXGI_FORMAT_R32G32B32_FLOAT,	0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };
			inputElementDesc[5] = { "NORMAL",		0, DXGI_FORMAT_R32G32B32_FLOAT,	0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0};

			inputLayoutDesc.pInputElementDescs	= inputElementDesc;
			inputLayoutDesc.NumElements			= _countof(inputElementDesc);
			break;
		}
	}

	return inputLayoutDesc;
}

//------------------------------------------------------------------------------------------------------------------
D3D12_BLEND_DESC DX12Renderer::GetBlendDesc(BlendMode blendMode)
{
	D3D12_BLEND_DESC blendDesc = {};
	
	switch(blendMode)
	{
		case BlendMode::ALPHA:
		{
			blendDesc.RenderTarget[0].BlendEnable			= TRUE;
			blendDesc.RenderTarget[0].SrcBlend				= D3D12_BLEND_SRC_ALPHA;
			blendDesc.RenderTarget[0].DestBlend				= D3D12_BLEND_INV_SRC_ALPHA;
			blendDesc.RenderTarget[0].BlendOp				= D3D12_BLEND_OP_ADD;
			blendDesc.RenderTarget[0].SrcBlendAlpha			= D3D12_BLEND_SRC_ALPHA;
			blendDesc.RenderTarget[0].DestBlendAlpha		= D3D12_BLEND_INV_SRC_ALPHA;
			blendDesc.RenderTarget[0].BlendOpAlpha			= D3D12_BLEND_OP_ADD;
			blendDesc.RenderTarget[0].RenderTargetWriteMask	= D3D12_COLOR_WRITE_ENABLE_ALL;
			break;
		}
	
		case BlendMode::ADDITIVE:
		{
			blendDesc.RenderTarget[0].BlendEnable			= TRUE;                        
			blendDesc.RenderTarget[0].SrcBlend				= D3D12_BLEND_ONE;             
			blendDesc.RenderTarget[0].DestBlend				= D3D12_BLEND_ONE;             
			blendDesc.RenderTarget[0].BlendOp				= D3D12_BLEND_OP_ADD;          
			blendDesc.RenderTarget[0].SrcBlendAlpha			= D3D12_BLEND_ONE;             
			blendDesc.RenderTarget[0].DestBlendAlpha		= D3D12_BLEND_ONE;         
			blendDesc.RenderTarget[0].BlendOpAlpha			= D3D12_BLEND_OP_ADD;          
			blendDesc.RenderTarget[0].RenderTargetWriteMask	= D3D12_COLOR_WRITE_ENABLE_ALL;
			break;                                                                         		
		}

		case BlendMode::OPAQUE:
		{
			blendDesc.RenderTarget[0].BlendEnable			= TRUE;                        
			blendDesc.RenderTarget[0].SrcBlend				= D3D12_BLEND_ONE;             
			blendDesc.RenderTarget[0].DestBlend				= D3D12_BLEND_ZERO;            
			blendDesc.RenderTarget[0].BlendOp				= D3D12_BLEND_OP_ADD;          
			blendDesc.RenderTarget[0].SrcBlendAlpha			= D3D12_BLEND_ONE;             
			blendDesc.RenderTarget[0].DestBlendAlpha		= D3D12_BLEND_ZERO;        
			blendDesc.RenderTarget[0].BlendOpAlpha			= D3D12_BLEND_OP_ADD;          
			blendDesc.RenderTarget[0].RenderTargetWriteMask	= D3D12_COLOR_WRITE_ENABLE_ALL;
			break;
		}
	}

	return blendDesc;
}

//------------------------------------------------------------------------------------------------------------------
D3D12_DEPTH_STENCIL_DESC DX12Renderer::GetDepthStencilDesc(DepthMode depthMode)
{
	D3D12_DEPTH_STENCIL_DESC depthDesc		= {};
	depthDesc.StencilEnable					= FALSE;
	depthDesc.StencilReadMask				= D3D12_DEFAULT_STENCIL_READ_MASK;
	depthDesc.StencilWriteMask				= D3D12_DEFAULT_STENCIL_WRITE_MASK;
	depthDesc.FrontFace.StencilFailOp		= D3D12_STENCIL_OP_KEEP;
	depthDesc.FrontFace.StencilDepthFailOp	= D3D12_STENCIL_OP_KEEP;
	depthDesc.FrontFace.StencilPassOp		= D3D12_STENCIL_OP_KEEP;
	depthDesc.FrontFace.StencilFunc			= D3D12_COMPARISON_FUNC_ALWAYS;
	depthDesc.BackFace.StencilFailOp		= D3D12_STENCIL_OP_KEEP;
	depthDesc.BackFace.StencilDepthFailOp	= D3D12_STENCIL_OP_KEEP;
	depthDesc.BackFace.StencilPassOp		= D3D12_STENCIL_OP_KEEP;
	depthDesc.BackFace.StencilFunc			= D3D12_COMPARISON_FUNC_ALWAYS;
	
	switch(depthMode)
	{
		case DepthMode::DISABLED:
		{
			depthDesc.DepthEnable = FALSE;
			depthDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
			depthDesc.DepthFunc = D3D12_COMPARISON_FUNC_ALWAYS;
			break;
		}
		case DepthMode::READ_ONLY_ALWAYS:
		{
			depthDesc.DepthEnable = TRUE;
			depthDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
			depthDesc.DepthFunc = D3D12_COMPARISON_FUNC_ALWAYS;
			break;
		}
		case DepthMode::READ_ONLY_LESS_EQUAL:
		{
			depthDesc.DepthEnable = TRUE;
			depthDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
			depthDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
			break;
		}
		case DepthMode::READ_WRITE_LESS_EQUAL:
		{
			depthDesc.DepthEnable = TRUE;
			depthDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
			depthDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
			break;
		}
	}
	return depthDesc;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void DX12Renderer::InitImGui()
{
	ImGui_ImplDX12_InitInfo init_info = {};
	init_info.Device = m_device;
	init_info.CommandQueue = m_directCommandQueue->m_d3dCommandQueue;
	init_info.NumFramesInFlight = m_numBuffers;
	init_info.RTVFormat = DXGI_FORMAT_R8G8B8A8_UNORM;

	init_info.SrvDescriptorHeap = s_imGuiHeap->m_imGuiHeap;
	init_info.SrvDescriptorAllocFn = [](ImGui_ImplDX12_InitInfo*, D3D12_CPU_DESCRIPTOR_HANDLE* out_cpu_handle, D3D12_GPU_DESCRIPTOR_HANDLE* out_gpu_handle) { return s_imGuiHeap->Alloc(out_cpu_handle, out_gpu_handle); };
	init_info.SrvDescriptorFreeFn = [](ImGui_ImplDX12_InitInfo*, D3D12_CPU_DESCRIPTOR_HANDLE cpu_handle, D3D12_GPU_DESCRIPTOR_HANDLE gpu_handle) { return s_imGuiHeap->Free(cpu_handle, gpu_handle); };
	ImGui_ImplDX12_Init(&init_info);
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void DX12Renderer::CompositeRenderTargetOutputs()
{
	PIXBeginEvent(m_graphicsCommandList->m_d3dCommandList, PIX_COLOR_DEFAULT, "Begin Compositing");
	
	Texture* backBuffer = m_backBuffers[m_currentBackBufferIndex];
	Texture* renderTarget	= m_renderTarget->GetTexture(AttachmentPoint::RASTER_OUTPUT);
	Texture* rtRenderTarget = m_renderTarget->GetTexture(AttachmentPoint::RT_OUTPUT);
		
	Texture* depthTexture = m_renderTarget->GetTexture(AttachmentPoint::DEPTH_STENCIL);


	D3D12_CPU_DESCRIPTOR_HANDLE depthStencilDescriptor = {};
	depthStencilDescriptor.ptr = 0;

	if(depthTexture->GetTextureResource())
	{
		m_graphicsCommandList->TransitionBarrier(depthTexture->m_textureResource, D3D12_RESOURCE_STATE_DEPTH_WRITE);
		depthStencilDescriptor = depthTexture->GetDSV();
	}

	D3D12_CPU_DESCRIPTOR_HANDLE* pDSV = depthStencilDescriptor.ptr != 0 ? &depthStencilDescriptor : nullptr;

	m_graphicsCommandList->TransitionBarrier(backBuffer->m_textureResource, D3D12_RESOURCE_STATE_RENDER_TARGET);
	D3D12_CPU_DESCRIPTOR_HANDLE rtv = backBuffer->m_renderTargetView.GetDescriptorCPUHandle();
	m_graphicsCommandList->m_d3dCommandList->OMSetRenderTargets(1, &rtv, FALSE, pDSV);

 	// Set pipeline
	m_graphicsCommandList->SetGraphicsRootSignature(*m_defaultRootSignature);
	PipelineStateObject* compositePSO = CreateOrGetPipelineStateObject(m_defaultRootSignature, m_compositeShader, InputLayoutType::VERTEX_PCU, RasterizerMode::SOLID_CULL_BACK, 
																BlendMode::OPAQUE, DepthMode::READ_WRITE_LESS_EQUAL);
	m_graphicsCommandList->SetPipelineState(*compositePSO);

	BindTexture(renderTarget, RootParameters::DIFFUSE_TEXTURE_DT);
	BindTexture(rtRenderTarget, RootParameters::NORMAL_TEXTURE_DT);
//	BindTexture(m_compositeDepthBuffer, RootParameters::SGE_TEXTURE_DT);

	Camera camera;
	camera.m_viewportBounds = AABB2(Vec2::ZERO, Vec2(m_config.m_window->GetClientDimensions()));
	camera.m_mode = camera.eMode_Orthographic;
	camera.SetOrthographicView(camera.m_viewportBounds);
	AABB2 fullScreenQuad = AABB2(Vec2::ZERO, Vec2(m_config.m_window->GetClientDimensions()));

	std::vector<Vertex_PCU> verts;
	AddVertsForAABB2D(verts, fullScreenQuad, Rgba8::WHITE);

	BeginCamera(camera);
	SetModelConstants();
	DrawVertexArray(verts);
	EndCamera(camera);

	PIXEndEvent(m_graphicsCommandList->m_d3dCommandList);
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
DescriptorAllocationResult DX12Renderer::AllocateDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t numDecriptors)
{
	return DX12Renderer::s_descriptorAllocators[type]->Allocate(numDecriptors); 
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void DX12Renderer::ClearScreen(Rgba8 const& clearColor)
{	
	m_graphicsCommandList->ClearRenderTargetTexture(m_renderTarget->GetTexture(AttachmentPoint::RASTER_OUTPUT), clearColor);
 	m_graphicsCommandList->ClearDepthStencilTexture(m_renderTarget->GetTexture(AttachmentPoint::DEPTH_STENCIL));
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void DX12Renderer::InitializeDebugInterfaces()
{

#if defined(ENGINE_DEBUG_RENDER)
	
	#if defined (ENABLE_PIX_DEBUGGER)
		m_pixLib = (void*)PIXLoadLatestWinPixGpuCapturerLibrary();
	#endif // ENABLE_PIX_DEBUGGER

	HRESULT hr = D3D12GetDebugInterface(IID_PPV_ARGS(&m_debugInterface));
	if(!SUCCEEDED(hr))
	{
		ERROR_AND_DIE("Failed to initialize debug interface; DX12Renderer::InitializeDebugInterface()")
	}

	m_debugInterface->EnableDebugLayer();

	m_dxgiDebugModule = (void*)::LoadLibraryA("dxgidebug.dll");
	if(m_dxgiDebugModule == nullptr)
	{
		ERROR_AND_DIE("Could not load dxgidebug.dll")
	}

	typedef HRESULT(WINAPI* GetDebugModuleCB)(REFIID, void**);
	((GetDebugModuleCB)::GetProcAddress((HMODULE)m_dxgiDebugModule, "DXGIGetDebugInterface"))
		(_uuidof(IDXGIDebug), &m_dxgiDebug);

	if(m_dxgiDebug == nullptr)
	{
		ERROR_AND_DIE("Could not load debug module")
	}


#endif // ENGINE_DEBUG_RENDER

}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void DX12Renderer::CreateDevice()
{
	HRESULT hr;
	
	hr = D3D12CreateDevice(m_adapter, D3D_FEATURE_LEVEL_12_2, IID_PPV_ARGS(&m_device));

	if(FAILED(hr))
	{
		ERROR_AND_DIE("D3D12 Device Creation Failed!")
	}
	else if(hr == S_FALSE && m_device == nullptr)
	{
		ERROR_RECOVERABLE("D3D12 Device is NULL. D3D12CreateDevice returned S_FALSE")
	}

	std::string debugName = Stringf(" DX12 Device");
	m_device->SetPrivateData(WKPDID_D3DDebugObjectName, static_cast<UINT>(strlen(debugName.c_str())), debugName.c_str());

	D3D12_FEATURE_DATA_D3D12_OPTIONS5 options5 = {};
	hr = m_device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5, &options5, sizeof(options5));
	if(SUCCEEDED(hr))
	{
		DebuggerPrintf("==================Raytracing Tier: %d======================\n", options5.RaytracingTier);
	}

#if defined(ENGINE_DEBUG_RENDER)
	ID3D12InfoQueue* infoQueue;

	if(SUCCEEDED(m_device->QueryInterface(&infoQueue)))
	{
		infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE);
		infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE);
		infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, FALSE);

		// Suppress whole categories of messages
		//D3D12_MESSAGE_CATEGORY Categories[] = {};

		D3D12_MESSAGE_SEVERITY severities[] = {D3D12_MESSAGE_SEVERITY_INFO};

		// Suppress individual messages by their ID
		D3D12_MESSAGE_ID denyIDs[] = {
			D3D12_MESSAGE_ID_CLEARRENDERTARGETVIEW_MISMATCHINGCLEARVALUE,   
			D3D12_MESSAGE_ID_MAP_INVALID_NULLRANGE,                         
			D3D12_MESSAGE_ID_UNMAP_INVALID_NULLRANGE,                       
		};

		D3D12_INFO_QUEUE_FILTER newFilter = {};
		//NewFilter.DenyList.NumCategories = _countof(Categories);
		//NewFilter.DenyList.pCategoryList = Categories;
		newFilter.DenyList.NumSeverities = _countof(severities);
		newFilter.DenyList.pSeverityList = severities;
		newFilter.DenyList.NumIDs = _countof(denyIDs);
		newFilter.DenyList.pIDList = denyIDs;

		if(FAILED(infoQueue->PushStorageFilter(&newFilter)))
		{
			ERROR_AND_DIE("InfoQueue new storage filter failed");
		}
	}

	infoQueue->Release();
#endif

}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void DX12Renderer::CreateCommandQueues()
{
	m_directCommandQueue	= new CommandQueue(m_device, D3D12_COMMAND_LIST_TYPE_DIRECT);
	m_copyCommandQueue		= new CommandQueue(m_device, D3D12_COMMAND_LIST_TYPE_COPY);
	m_computeCommandQueue	= new CommandQueue(m_device, D3D12_COMMAND_LIST_TYPE_COMPUTE);

	m_graphicsCommandList	= m_directCommandQueue->GetCommandList();
	m_copyCommandList		= m_copyCommandQueue->GetCommandList();
	m_computeCommandList	= m_computeCommandQueue->GetCommandList();
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void DX12Renderer::CreateSwapChainAndGetBackBufferIndex()
{

	int clientWidth = m_config.m_window->GetClientDimensions().x;
	int clientHeight = m_config.m_window->GetClientDimensions().y;

	DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
	swapChainDesc.Width			= clientWidth;
	swapChainDesc.Height		= clientHeight;
	swapChainDesc.Format		= DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.Stereo		= FALSE;
	swapChainDesc.SampleDesc	= {1, 0};
	swapChainDesc.BufferUsage	= DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.BufferCount	= m_numBuffers;
	swapChainDesc.Scaling		= DXGI_SCALING_STRETCH;
	swapChainDesc.SwapEffect	= DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swapChainDesc.AlphaMode		= DXGI_ALPHA_MODE_UNSPECIFIED;
	swapChainDesc.Flags			= m_config.m_tearingSupported ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0;

	HWND windowHWND = static_cast<HWND>(m_config.m_window->GetHWND());

	IDXGISwapChain1* tempSwapChain;

	HRESULT hr;
	hr = m_factory->CreateSwapChainForHwnd(m_directCommandQueue->GetCommandQueue(), windowHWND, &swapChainDesc, nullptr, nullptr, &tempSwapChain);

	if(FAILED(hr))
	{
		ERROR_AND_DIE("Could not create swap chain!")
	}

	std::string debugName = Stringf("Swap Chain");
	tempSwapChain->SetPrivateData(WKPDID_D3DDebugObjectName, static_cast<UINT>(strlen(debugName.c_str())), debugName.c_str());

	hr = tempSwapChain->QueryInterface(&m_swapChain);
	if(FAILED(hr))
	{
		ERROR_AND_DIE("Query Interface for Swap Chain 1 to 4 failed!")
	}

	debugName = Stringf("Swap Chain");
	tempSwapChain->SetPrivateData(WKPDID_D3DDebugObjectName, static_cast<UINT>(strlen(debugName.c_str())), debugName.c_str());

	DX_SAFE_RELEASE(tempSwapChain)

	m_currentBackBufferIndex = m_swapChain->GetCurrentBackBufferIndex();
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void DX12Renderer::CreateRenderTargetTextures()
{
	m_renderTarget = new RenderTarget();

	IntVec2 rtTextureDimensons = g_theWindow->GetClientDimensions();

	D3D12_HEAP_PROPERTIES heapProperties	= {};
	heapProperties.Type						= D3D12_HEAP_TYPE_DEFAULT;

	D3D12_RESOURCE_DESC textureDesc = {};
	textureDesc.Dimension			= D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	textureDesc.Alignment			= 0;
	textureDesc.Width				= rtTextureDimensons.x;
	textureDesc.Height				= rtTextureDimensons.y;
	textureDesc.DepthOrArraySize	= 1;
	textureDesc.MipLevels			= 1;
	textureDesc.Format				= DXGI_FORMAT_R8G8B8A8_UNORM;
	textureDesc.SampleDesc.Count	= 1;
	textureDesc.SampleDesc.Quality	= 0;
	textureDesc.Layout				= D3D12_TEXTURE_LAYOUT_UNKNOWN;
	textureDesc.Flags				= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

	ID3D12Resource* renderTarget = nullptr;

	HRESULT hr;
	hr = m_device->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &textureDesc, D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&renderTarget));

	if(FAILED(hr))
	{
		ERROR_AND_DIE("Depth Buffer resource creation failed");
	}

	ResourceStateTracker::AddGlobalResourceState(renderTarget, D3D12_RESOURCE_STATE_COMMON);
	renderTarget->SetName(L"Render Target Texture");
	Texture* renderTargetTexture = new Texture(m_device, renderTarget, "Render Target 0");
	renderTargetTexture->CreateViews();

	m_renderTarget->AttachTexture(AttachmentPoint::RASTER_OUTPUT, renderTargetTexture);
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void DX12Renderer::UpdateRenderTargetViews()
{
	HRESULT hr;

	for(int i = 0; i < m_numBuffers; ++i)
	{
		ID3D12Resource* backBufferResource = nullptr;

		hr = m_swapChain->GetBuffer(i, IID_PPV_ARGS(&backBufferResource));

		if(!SUCCEEDED(hr))
		{
			ERROR_AND_DIE("GetBuffer failed; UpdateRenderTargetViews()")
		}

		backBufferResource->SetName(L"Back Buffer");

		ResourceStateTracker::AddGlobalResourceState(backBufferResource, D3D12_RESOURCE_STATE_COMMON);

		m_backBuffers[i] = new Texture(m_device, backBufferResource, "BackBuffer");
		m_backBuffers[i]->m_device = m_device;
		m_backBuffers[i]->CreateViews();
	}
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void DX12Renderer::CreateRTPSOs()
{
	CreateDefaultRayTracingPSO();
	CreateGBufferRayTracingPSO();

	m_temporalReusePSO	= CreateComputePSO(m_globalRTRootSignature, m_temporalReuseShader);
	m_spatialReusePSO	= CreateComputePSO(m_globalRTRootSignature, m_spatialReuseShader);
	m_denoiserPSO		= CreateComputePSO(m_globalRTRootSignature, m_denoiseShader);
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void DX12Renderer::CreateDefaultRayTracingPSO()
{
	// Contains full set of shaders reachable by a DispatchRays() call
	// 7 Subobjects into a single RTPSO
	// 1 - DXIL library
	// 2 - Triangle hit groups 
	//		1 - Primary Hit Group
	//		1 - Shadow Hit Group
	// 1 - Shader configs
	// 1 - Global root signature
	// 1 - Pipeline config
	
	CD3DX12_STATE_OBJECT_DESC rtPSO {D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE};

	Shader* shader = CreateOrGetShader("Data/Shaders/DefaultRT", InputLayoutType::VERTEX_PCUTBN, true, SHADER_RAYTRACE);

	// DXIL Library
	D3D12_SHADER_BYTECODE dxilShaderByteCode = {};
	dxilShaderByteCode.pShaderBytecode = shader->m_rtBlob->GetBufferPointer();
	dxilShaderByteCode.BytecodeLength = shader->m_rtBlob->GetBufferSize();

	CD3DX12_DXIL_LIBRARY_SUBOBJECT* dxilLib = rtPSO.CreateSubobject<CD3DX12_DXIL_LIBRARY_SUBOBJECT>();
	dxilLib->SetDXILLibrary(&dxilShaderByteCode);

	// Primary Ray Shader Exports
	dxilLib->DefineExport(k_rayGenShaderName.c_str());
	dxilLib->DefineExport(k_closestHitShaderName.c_str());
	dxilLib->DefineExport(k_missShaderName.c_str());
	
	// Shadow Ray Exports
	dxilLib->DefineExport(k_shadowClosestHitShaderName.c_str());
	dxilLib->DefineExport(k_shadowMissShaderName.c_str());

	// Triangle Hit Group 
	// Hit Groups in general specifies what hit shaders to be executed (Closest Hit, Any Hit, Intersection) when a ray hits any geometry (Triangle/AABB)
	CD3DX12_HIT_GROUP_SUBOBJECT* primaryRayHitGroup = rtPSO.CreateSubobject<CD3DX12_HIT_GROUP_SUBOBJECT>();
	primaryRayHitGroup->SetClosestHitShaderImport(k_closestHitShaderName.c_str());
	primaryRayHitGroup->SetHitGroupExport(k_hitGroupName.c_str());
	primaryRayHitGroup->SetHitGroupType(D3D12_HIT_GROUP_TYPE_TRIANGLES);
	
	CD3DX12_HIT_GROUP_SUBOBJECT* shadowRayHitGroup = rtPSO.CreateSubobject<CD3DX12_HIT_GROUP_SUBOBJECT>();
	shadowRayHitGroup->SetClosestHitShaderImport(k_shadowClosestHitShaderName.c_str());
	shadowRayHitGroup->SetHitGroupExport(k_shadowHitGroupName.c_str());
	shadowRayHitGroup->SetHitGroupType(D3D12_HIT_GROUP_TYPE_TRIANGLES);

	// Shader Config -  there can be multiple and in that case there can be associations as well.
	CD3DX12_RAYTRACING_SHADER_CONFIG_SUBOBJECT* shaderConfig = rtPSO.CreateSubobject<CD3DX12_RAYTRACING_SHADER_CONFIG_SUBOBJECT>();
	UINT maxPayloadSize = sizeof(RayPayload); // Replace float or the entire thing with sizeof(payload)
	UINT maxAttrSize = 2 * sizeof(float); // Attributes struct maybe?
	shaderConfig->Config(maxPayloadSize, maxAttrSize);
 
	CD3DX12_GLOBAL_ROOT_SIGNATURE_SUBOBJECT* globalRootSigSubObject = rtPSO.CreateSubobject<CD3DX12_GLOBAL_ROOT_SIGNATURE_SUBOBJECT>();
	globalRootSigSubObject->SetRootSignature(m_globalRTRootSignature->m_d3dRootSignature);

	// PSO Config
	// Define max TraceRay recursion depth. There can only be ONE PSO Config for PSO
	CD3DX12_RAYTRACING_PIPELINE_CONFIG_SUBOBJECT* psoConfig = rtPSO.CreateSubobject<CD3DX12_RAYTRACING_PIPELINE_CONFIG_SUBOBJECT>();
	psoConfig->Config(RAY_COUNT);

	// Create PSO	
	HRESULT hr = m_device->CreateStateObject(rtPSO, IID_PPV_ARGS(&m_rtPSO));
	if(FAILED(hr))
	{
		ERROR_AND_DIE("Failed to create Default RT PSO");
	}
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void DX12Renderer::CreateGBufferRayTracingPSO()
{
	CD3DX12_STATE_OBJECT_DESC gBufferPSO{D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE};

	Shader* shader = CreateOrGetShader("Data/Shaders/GBuffers", InputLayoutType::VERTEX_PCUTBN, true, SHADER_RAYTRACE);

	// DXIL Library
	D3D12_SHADER_BYTECODE dxilShaderByteCode = {};
	dxilShaderByteCode.pShaderBytecode = shader->m_rtBlob->GetBufferPointer();
	dxilShaderByteCode.BytecodeLength = shader->m_rtBlob->GetBufferSize();

	CD3DX12_DXIL_LIBRARY_SUBOBJECT* dxilLib = gBufferPSO.CreateSubobject<CD3DX12_DXIL_LIBRARY_SUBOBJECT>();
	dxilLib->SetDXILLibrary(&dxilShaderByteCode);

	// Primary Ray Shader Exports
	dxilLib->DefineExport(k_rayGenShaderName.c_str());
	dxilLib->DefineExport(k_closestHitShaderName.c_str());
	dxilLib->DefineExport(k_missShaderName.c_str());

	// Shadow Ray Exports
	dxilLib->DefineExport(k_shadowClosestHitShaderName.c_str());
	dxilLib->DefineExport(k_shadowMissShaderName.c_str());

	// Triangle Hit Group 
	// Hit Groups in general specifies what hit shaders to be executed (Closest Hit, Any Hit, Intersection) when a ray hits any geometry (Triangle/AABB)
	CD3DX12_HIT_GROUP_SUBOBJECT* primaryRayHitGroup = gBufferPSO.CreateSubobject<CD3DX12_HIT_GROUP_SUBOBJECT>();
	primaryRayHitGroup->SetClosestHitShaderImport(k_closestHitShaderName.c_str());
	primaryRayHitGroup->SetHitGroupExport(k_hitGroupName.c_str());
	primaryRayHitGroup->SetHitGroupType(D3D12_HIT_GROUP_TYPE_TRIANGLES);

	CD3DX12_HIT_GROUP_SUBOBJECT* shadowRayHitGroup = gBufferPSO.CreateSubobject<CD3DX12_HIT_GROUP_SUBOBJECT>();
	shadowRayHitGroup->SetClosestHitShaderImport(k_shadowClosestHitShaderName.c_str());
	shadowRayHitGroup->SetHitGroupExport(k_shadowHitGroupName.c_str());
	shadowRayHitGroup->SetHitGroupType(D3D12_HIT_GROUP_TYPE_TRIANGLES);

	// Shader Config -  there can be multiple and in that case there can be associations as well.
	CD3DX12_RAYTRACING_SHADER_CONFIG_SUBOBJECT* shaderConfig = gBufferPSO.CreateSubobject<CD3DX12_RAYTRACING_SHADER_CONFIG_SUBOBJECT>();
	UINT maxPayloadSize = sizeof(RayPayload); // Replace float or the entire thing with sizeof(payload)
	UINT maxAttrSize = 2 * sizeof(float); // Attributes struct maybe?
	shaderConfig->Config(maxPayloadSize, maxAttrSize);

	CD3DX12_GLOBAL_ROOT_SIGNATURE_SUBOBJECT* globalRootSigSubObject = gBufferPSO.CreateSubobject<CD3DX12_GLOBAL_ROOT_SIGNATURE_SUBOBJECT>();
	globalRootSigSubObject->SetRootSignature(m_globalRTRootSignature->m_d3dRootSignature);

	// PSO Config
	// Define max TraceRay recursion depth. There can only be ONE PSO Config for PSO
	unsigned int rayRecursionDepth = 2;
	CD3DX12_RAYTRACING_PIPELINE_CONFIG_SUBOBJECT* psoConfig = gBufferPSO.CreateSubobject<CD3DX12_RAYTRACING_PIPELINE_CONFIG_SUBOBJECT>();
	psoConfig->Config(rayRecursionDepth);

	// Create PSO	
	HRESULT hr = m_device->CreateStateObject(gBufferPSO, IID_PPV_ARGS(&m_gBufferPSO));
	if(FAILED(hr))
	{
		ERROR_AND_DIE("Failed to create GBuffer RT PSO");
	}
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
// void DX12Renderer::CreateTemporalReuseRayTracingPSO()
// {
// 	CD3DX12_STATE_OBJECT_DESC temporalReusePSO{D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE};
// 
// 	Shader* shader = CreateOrGetShader("Data/Shaders/TemporalReuse", InputLayoutType::VERTEX_PCUTBN, true, SHADER_RAYTRACE);
// 
// 	// DXIL Library
// 	D3D12_SHADER_BYTECODE dxilShaderByteCode = {};
// 	dxilShaderByteCode.pShaderBytecode = shader->m_rtBlob->GetBufferPointer();
// 	dxilShaderByteCode.BytecodeLength = shader->m_rtBlob->GetBufferSize();
// 
// 	CD3DX12_DXIL_LIBRARY_SUBOBJECT* dxilLib = temporalReusePSO.CreateSubobject<CD3DX12_DXIL_LIBRARY_SUBOBJECT>();
// 	dxilLib->SetDXILLibrary(&dxilShaderByteCode);
// 
// 	// Primary Ray Shader Exports
// 	dxilLib->DefineExport(k_rayGenShaderName.c_str());
// 	dxilLib->DefineExport(k_closestHitShaderName.c_str());
// 	dxilLib->DefineExport(k_missShaderName.c_str());
// 
// 	// Shadow Ray Exports
// 	dxilLib->DefineExport(k_shadowClosestHitShaderName.c_str());
// 	dxilLib->DefineExport(k_shadowMissShaderName.c_str());
// 
// 	// Triangle Hit Group 
// 	// Hit Groups in general specifies what hit shaders to be executed (Closest Hit, Any Hit, Intersection) when a ray hits any geometry (Triangle/AABB)
// 	CD3DX12_HIT_GROUP_SUBOBJECT* primaryRayHitGroup = temporalReusePSO.CreateSubobject<CD3DX12_HIT_GROUP_SUBOBJECT>();
// 	primaryRayHitGroup->SetClosestHitShaderImport(k_closestHitShaderName.c_str());
// 	primaryRayHitGroup->SetHitGroupExport(k_hitGroupName.c_str());
// 	primaryRayHitGroup->SetHitGroupType(D3D12_HIT_GROUP_TYPE_TRIANGLES);
// 
// 	CD3DX12_HIT_GROUP_SUBOBJECT* shadowRayHitGroup = temporalReusePSO.CreateSubobject<CD3DX12_HIT_GROUP_SUBOBJECT>();
// 	shadowRayHitGroup->SetClosestHitShaderImport(k_shadowClosestHitShaderName.c_str());
// 	shadowRayHitGroup->SetHitGroupExport(k_shadowHitGroupName.c_str());
// 	shadowRayHitGroup->SetHitGroupType(D3D12_HIT_GROUP_TYPE_TRIANGLES);
// 
// 	// Shader Config -  there can be multiple and in that case there can be associations as well.
// 	CD3DX12_RAYTRACING_SHADER_CONFIG_SUBOBJECT* shaderConfig = temporalReusePSO.CreateSubobject<CD3DX12_RAYTRACING_SHADER_CONFIG_SUBOBJECT>();
// 	UINT maxPayloadSize = sizeof(RayPayload); // Replace float or the entire thing with sizeof(payload)
// 	UINT maxAttrSize = 2 * sizeof(float); // Attributes struct maybe?
// 	shaderConfig->Config(maxPayloadSize, maxAttrSize);
// 
// 	CD3DX12_GLOBAL_ROOT_SIGNATURE_SUBOBJECT* globalRootSigSubObject = temporalReusePSO.CreateSubobject<CD3DX12_GLOBAL_ROOT_SIGNATURE_SUBOBJECT>();
// 	globalRootSigSubObject->SetRootSignature(m_globalRTRootSignature->m_d3dRootSignature);
// 
// 	// PSO Config
// 	// Define max TraceRay recursion depth. There can only be ONE PSO Config for PSO
// 	unsigned int rayRecursionDepth = 1;
// 	CD3DX12_RAYTRACING_PIPELINE_CONFIG_SUBOBJECT* psoConfig = temporalReusePSO.CreateSubobject<CD3DX12_RAYTRACING_PIPELINE_CONFIG_SUBOBJECT>();
// 	psoConfig->Config(rayRecursionDepth);
// 
// 	// Create PSO	
// 	HRESULT hr = m_device->CreateStateObject(temporalReusePSO, IID_PPV_ARGS(&m_temporalReusePSO));
// 	if(FAILED(hr))
// 	{
// 		ERROR_AND_DIE("Failed to create Temporal Reuse RT PSO");
// 	}
// 
// }

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void DX12Renderer::BuildGBufferShaderTables(int numRayGenRecordsPerRayType, int numMissShaderRecordsPerRayType, int numHitShaderRecordsPerRayType)
{
	// A Hit group is Any Hit(s) + Intersection(s) + Closest Hit(s)
	void* rayGenShaderIdentifier = nullptr;
	void* primaryHitGroupIdentifier = nullptr;
	void* primaryMissShaderIdentifier = nullptr;

 	void* shadowHitGroupIdentifier = nullptr;
 	void* shadowMissShaderIdentifier = nullptr;


	ID3D12StateObjectProperties* stateObjProperties = nullptr;
	HRESULT hr = m_gBufferPSO->QueryInterface(IID_PPV_ARGS(&stateObjProperties));

	if(FAILED(hr))
	{
		ERROR_AND_DIE("Query Interface from rtPSO (ID3D12StateObject) to stateObjProperties (ID3D12StateObjectProperties) FAILED.");
	}

	rayGenShaderIdentifier = stateObjProperties->GetShaderIdentifier(k_rayGenShaderName.c_str());
	primaryHitGroupIdentifier = stateObjProperties->GetShaderIdentifier(k_hitGroupName.c_str());
	primaryMissShaderIdentifier = stateObjProperties->GetShaderIdentifier(k_missShaderName.c_str());

 	shadowHitGroupIdentifier = stateObjProperties->GetShaderIdentifier(k_shadowHitGroupName.c_str());
 	shadowMissShaderIdentifier = stateObjProperties->GetShaderIdentifier(k_shadowMissShaderName.c_str());
 
	unsigned int shaderIdentifierSize = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;

	unsigned int numRayGenShaderRecords = numRayGenRecordsPerRayType;
	unsigned int rayGenShaderRecordSize = shaderIdentifierSize;

	m_rayGenShaderTable_gBuffer = new ShaderTable(m_device, numRayGenShaderRecords, rayGenShaderRecordSize, L"RayGenShaderTable_GBuffer");
	m_rayGenShaderTable_gBuffer->AddShaderRecord(ShaderRecord(rayGenShaderIdentifier, shaderIdentifierSize));

	// Miss Shader Table
	unsigned int numMissShaderRecords = RAY_COUNT * numMissShaderRecordsPerRayType;
	unsigned int missShaderRecordSize = shaderIdentifierSize;

	m_missShaderTable_gBuffer = new ShaderTable(m_device, numMissShaderRecords, missShaderRecordSize, L"MissShaderTable_GBuffer");
	m_missShaderTable_gBuffer->AddShaderRecord(ShaderRecord(primaryMissShaderIdentifier, shaderIdentifierSize));
	m_missShaderTable_gBuffer->AddShaderRecord(ShaderRecord(shadowMissShaderIdentifier, shaderIdentifierSize));

	// Hit Group Shader Table
	unsigned int numHitGroupShaderRecords = RAY_COUNT * numHitShaderRecordsPerRayType;
	unsigned int hitGroupShaderRecordSize = shaderIdentifierSize;
	m_hitGroupShaderTable_gBuffer = new ShaderTable(m_device, numHitGroupShaderRecords, hitGroupShaderRecordSize, L"HitGroupShaderTable_GBuffer");

	for(int i = 0; i < numHitShaderRecordsPerRayType; i++)
	{
		m_hitGroupShaderTable_gBuffer->AddShaderRecord(ShaderRecord(primaryHitGroupIdentifier, shaderIdentifierSize));
		m_hitGroupShaderTable_gBuffer->AddShaderRecord(ShaderRecord(shadowHitGroupIdentifier, shaderIdentifierSize));
	}

	stateObjProperties->Release();

}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void DX12Renderer::BuildDefaultShaderBindingTables(int numRayGenRecordsPerRayType, int numMissShaderRecordsPerRayType, int numHitShaderRecordsPerRayType)
{
	// A Hit group is Any Hit(s) + Intersection(s) + Closest Hit(s)
	void* rayGenShaderIdentifier		= nullptr;
	void* primaryHitGroupIdentifier		= nullptr;
	void* primaryMissShaderIdentifier	= nullptr;

	void* shadowHitGroupIdentifier		= nullptr;
	void* shadowMissShaderIdentifier	= nullptr;

	ID3D12StateObjectProperties* stateObjProperties = nullptr;
	HRESULT hr = m_rtPSO->QueryInterface(IID_PPV_ARGS(&stateObjProperties));

	if(FAILED(hr))
	{
		ERROR_AND_DIE("Query Interface from rtPSO (ID3D12StateObject) to stateObjProperties (ID3D12StateObjectProperties) FAILED.");
	}

	rayGenShaderIdentifier		= stateObjProperties->GetShaderIdentifier(k_rayGenShaderName.c_str());
	primaryHitGroupIdentifier	= stateObjProperties->GetShaderIdentifier(k_hitGroupName.c_str());
	primaryMissShaderIdentifier	= stateObjProperties->GetShaderIdentifier(k_missShaderName.c_str());
	
	shadowHitGroupIdentifier	= stateObjProperties->GetShaderIdentifier(k_shadowHitGroupName.c_str());
	shadowMissShaderIdentifier	= stateObjProperties->GetShaderIdentifier(k_shadowMissShaderName.c_str());

	unsigned int shaderIdentifierSize = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;

	unsigned int numRayGenShaderRecords = numRayGenRecordsPerRayType;
	unsigned int rayGenShaderRecordSize = shaderIdentifierSize;

	m_rayGenShaderTable = new ShaderTable(m_device, numRayGenShaderRecords, rayGenShaderRecordSize, L"RayGenShaderTable");
	m_rayGenShaderTable->AddShaderRecord(ShaderRecord(rayGenShaderIdentifier, shaderIdentifierSize));

	// Miss Shader Table
	unsigned int numMissShaderRecords = RAY_COUNT * numMissShaderRecordsPerRayType;
	unsigned int missShaderRecordSize = shaderIdentifierSize;

	m_missShaderTable = new ShaderTable(m_device, numMissShaderRecords, missShaderRecordSize, L"MissShaderTable");
	m_missShaderTable->AddShaderRecord(ShaderRecord(primaryMissShaderIdentifier, shaderIdentifierSize));
	m_missShaderTable->AddShaderRecord(ShaderRecord(shadowMissShaderIdentifier, shaderIdentifierSize));

	// Hit Group Shader Table
	unsigned int numHitGroupShaderRecords = RAY_COUNT * numHitShaderRecordsPerRayType;
	unsigned int hitGroupShaderRecordSize = shaderIdentifierSize;
	m_hitGroupShaderTable = new ShaderTable(m_device, numHitGroupShaderRecords, hitGroupShaderRecordSize, L"HitGroupShaderTable");

	for(int i = 0; i < numHitShaderRecordsPerRayType; i++)
	{
		m_hitGroupShaderTable->AddShaderRecord(ShaderRecord(primaryHitGroupIdentifier, shaderIdentifierSize));
		m_hitGroupShaderTable->AddShaderRecord(ShaderRecord(shadowHitGroupIdentifier, shaderIdentifierSize));
	}

	stateObjProperties->Release();
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void DX12Renderer::CreateRaytracingRenderTargets()
{
// 	m_rtRenderTarget = new RenderTarget();

	IntVec2 rtTextureDimensons = g_theWindow->GetClientDimensions();

	D3D12_HEAP_PROPERTIES heapProperties = {};
	heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;

	D3D12_RESOURCE_DESC textureDesc		= {};
	textureDesc.Dimension				= D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	textureDesc.Alignment				= 0;
	textureDesc.Width					= rtTextureDimensons.x;
	textureDesc.Height					= rtTextureDimensons.y;
	textureDesc.DepthOrArraySize		= 1;
	textureDesc.MipLevels				= 1;
	textureDesc.Format					= DXGI_FORMAT_R8G8B8A8_UNORM;
	textureDesc.SampleDesc.Count		= 1;
	textureDesc.SampleDesc.Quality		= 0;
	textureDesc.Layout					= D3D12_TEXTURE_LAYOUT_UNKNOWN;
	textureDesc.Flags					= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

	ID3D12Resource* renderTarget = nullptr;
	ID3D12Resource*	previousFrame = nullptr;

	HRESULT hr;
	hr = m_device->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &textureDesc, D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&renderTarget));

	if(FAILED(hr))
	{
		ERROR_AND_DIE("RT Render Target resource creation failed");
	}

	ResourceStateTracker::AddGlobalResourceState(renderTarget, D3D12_RESOURCE_STATE_COMMON);
	renderTarget->SetName(L"RT Render Target Texture");

	m_graphicsCommandList->TransitionBarrier(renderTarget, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES, true);

	Texture* renderTargetTexture = new Texture(m_device, renderTarget, "Render Target RT");
	renderTargetTexture->CreateViews();

	m_renderTarget->AttachTexture(AttachmentPoint::RT_OUTPUT, renderTargetTexture);

	// Previous Frame Texture
	hr = m_device->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &textureDesc, D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&previousFrame));

	if(FAILED(hr))
	{
		ERROR_AND_DIE("Previous Frame RT Render Target resource creation failed");
	}

	ResourceStateTracker::AddGlobalResourceState(previousFrame, D3D12_RESOURCE_STATE_COMMON);
	previousFrame->SetName(L"RT Previous Frame Render Target Texture");

	m_graphicsCommandList->TransitionBarrier(previousFrame, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES, true);
	m_noisyRenderTarget = new Texture(m_device, previousFrame, "Render Target Previous Frame RT");
	m_noisyRenderTarget->CreateViews();
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void DX12Renderer::CreateReservoirBuffers()
{
	m_temporalReservoirBuffer = new StructuredBuffer(m_device);
	m_prevReservoirBuffer = new StructuredBuffer(m_device);
	m_finalReservoirBuffer = new StructuredBuffer(m_device);

	size_t numElements = g_theWindow->GetClientDimensions().x * g_theWindow->GetClientDimensions().y;
	size_t elementSize = 4 * sizeof(float);
	size_t bufferWidth = numElements * elementSize;

	m_temporalReservoirBuffer->m_numElements = numElements;
	m_temporalReservoirBuffer->m_sizeOfEachElement = elementSize;

	m_prevReservoirBuffer->m_numElements = numElements;
	m_prevReservoirBuffer->m_sizeOfEachElement = elementSize;

	m_finalReservoirBuffer->m_numElements = numElements;
	m_finalReservoirBuffer->m_sizeOfEachElement = elementSize;

	D3D12_HEAP_PROPERTIES heapProperties = {};
	heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;

	D3D12_RESOURCE_DESC textureDesc = {};
	textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	textureDesc.Alignment = 0;
	textureDesc.Width = bufferWidth;
	textureDesc.Height = 1;
	textureDesc.DepthOrArraySize = 1;
	textureDesc.MipLevels = 1;
	textureDesc.Format = DXGI_FORMAT_UNKNOWN;
	textureDesc.SampleDesc.Count = 1;
	textureDesc.SampleDesc.Quality = 0;
	textureDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	textureDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

	// Temporal
	HRESULT hr;
	hr = m_device->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &textureDesc, D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&m_temporalReservoirBuffer->m_buffer));

	if(FAILED(hr))
	{
		ERROR_AND_DIE("Temporal Reservoir Buffer resource creation failed");
	}

	ResourceStateTracker::AddGlobalResourceState(m_temporalReservoirBuffer->m_buffer, D3D12_RESOURCE_STATE_COMMON);
	m_temporalReservoirBuffer->m_buffer->SetName(L"Reservoir Buffer Texture");

	m_graphicsCommandList->TransitionBarrier(m_temporalReservoirBuffer->m_buffer, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES, true);

	// Prev
	hr = m_device->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &textureDesc, D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&m_prevReservoirBuffer->m_buffer));

	if(FAILED(hr))
	{
		ERROR_AND_DIE("Previous Frame Reservoir Buffer resource creation failed");
	}

	ResourceStateTracker::AddGlobalResourceState(m_prevReservoirBuffer->m_buffer, D3D12_RESOURCE_STATE_COMMON);
	m_prevReservoirBuffer->m_buffer->SetName(L"Previous Frame Reservoir Buffer");

	m_graphicsCommandList->TransitionBarrier(m_prevReservoirBuffer->m_buffer, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES, true);

	// Final
	hr = m_device->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &textureDesc, D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&m_finalReservoirBuffer->m_buffer));

	if(FAILED(hr))
	{
		ERROR_AND_DIE("Final Reservoir Buffer resource creation failed");
	}

	ResourceStateTracker::AddGlobalResourceState(m_finalReservoirBuffer->m_buffer, D3D12_RESOURCE_STATE_COMMON);
	m_finalReservoirBuffer->m_buffer->SetName(L"Final Reservoir Buffer");

	m_graphicsCommandList->TransitionBarrier(m_finalReservoirBuffer->m_buffer, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES, true);
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
BottomLevelAS* DX12Renderer::CreateBLAS(std::vector<VertexBuffer*> const& VBOs, std::vector<IndexBuffer*> const& IBOs)
{
	BottomLevelAS* blas = new BottomLevelAS(m_device, D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES, D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE);
	
	for(size_t index = 0; index < VBOs.size(); ++index)
	{
		m_graphicsCommandList->TransitionBarrier(VBOs[index]->m_buffer, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
		m_graphicsCommandList->TransitionBarrier(IBOs[index]->m_buffer, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
		m_graphicsCommandList->FlushResourceBarriers();

		blas->AddGeometry(VBOs[index], IBOs[index]);
	}

	blas->Build(m_graphicsCommandList);

	return blas;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
BottomLevelAS* DX12Renderer::CreateBLAS(VertexBuffer* VBO, IndexBuffer* IBO)
{
	BottomLevelAS* blas = new BottomLevelAS(m_device, D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES, D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE);

	m_graphicsCommandList->TransitionBarrier(VBO->m_buffer, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
	m_graphicsCommandList->TransitionBarrier(IBO->m_buffer, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
	m_graphicsCommandList->FlushResourceBarriers();

	blas->AddGeometry(VBO, IBO);

	blas->Build(m_graphicsCommandList);

	return blas;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
TopLevelAS* DX12Renderer::CreateTLAS(std::vector<BottomLevelAS*> sceneBLASes)
{
	TopLevelAS* tlas = new TopLevelAS(m_device, D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE);

	for(BottomLevelAS* blas : sceneBLASes)
	{
		if(blas)
		{
			tlas->AddInstance(blas);
		}
	}

	tlas->Build(m_graphicsCommandList);

	return tlas;

}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void DX12Renderer::BuildShaderBindingTables(int numRayGenRecordsPerRayType, int numMissShaderRecordsPerRayType, int numHitShaderRecordsPerRayType)
{
	BuildGBufferShaderTables(numRayGenRecordsPerRayType, numMissShaderRecordsPerRayType, numHitShaderRecordsPerRayType);
	BuildDefaultShaderBindingTables(numRayGenRecordsPerRayType, numMissShaderRecordsPerRayType, numHitShaderRecordsPerRayType);
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void DX12Renderer::DispatchRays(TopLevelAS* tlas)
{
	if(g_rtCameraConsts.m_prevViewMatrix != g_rtCameraConsts.m_cameraToWorld)
	{
		g_rtAppSettings.m_accumCount = 0;
	}

	if(g_rtAppSettings.m_maxFramesToAccumulate != -1 && static_cast<int>(g_rtAppSettings.m_accumCount) >= g_rtAppSettings.m_maxFramesToAccumulate)
	{
		return;
	}

	ComputeGBuffers(tlas); 

	m_graphicsCommandList->UAVBarrier(m_prevReservoirBuffer->m_buffer);
	m_graphicsCommandList->UAVBarrier(m_temporalReservoirBuffer->m_buffer);
	m_graphicsCommandList->UAVBarrier(m_finalReservoirBuffer->m_buffer, true);
	
	if(s_frameCount != 0)
	{
		if(g_rtAppSettings.m_doTemporalReuse)
		{
			PerformTemporalReuse();
			m_graphicsCommandList->UAVBarrier(m_temporalReservoirBuffer->m_buffer, true);
		}

		if(g_rtAppSettings.m_doSpatialReuse)
		{
			PerformSpatialReuse();
			m_graphicsCommandList->UAVBarrier(m_finalReservoirBuffer->m_buffer);
			m_graphicsCommandList->UAVBarrier(m_temporalReservoirBuffer->m_buffer, true);
		}
	}

	PerformDirectLighting(tlas);

	if(g_rtAppSettings.m_doDenoise == 1)
	{
		m_graphicsCommandList->UAVBarrier(m_noisyRenderTarget->m_textureResource, true);

		for(int i = 0; i < g_rtAppSettings.m_denoisePasses; i++)
		{
			Denoise();
			m_graphicsCommandList->CopyResource(m_noisyRenderTarget->m_textureResource, m_renderTarget->GetTexture(AttachmentPoint::RT_OUTPUT)->m_textureResource);
		}
		
	}
	
	g_rtCameraConsts.m_prevViewMatrix = g_rtCameraConsts.m_cameraToWorld;
	g_rtCameraConsts.m_prevFrameWorldToCamera = g_rtCameraConsts.m_currentFrameWorldToCamera;

	if(g_rtAppSettings.m_enableFrameAccumulation)
	{
		g_rtAppSettings.m_accumCount += 1;
	}

}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void DX12Renderer::PerformDirectLighting(TopLevelAS* tlas)
{
	PIXBeginEvent(m_graphicsCommandList->m_d3dCommandList, PIX_COLOR_DEFAULT, "Lighting Pass");

	ID3D12DescriptorHeap* heaps[] = {m_bindlessSRVHeap->CurrentHeap()};
	m_graphicsCommandList->m_d3dCommandList->SetDescriptorHeaps(1, heaps);
	{
		Texture* texture = m_renderTarget->GetTexture(AttachmentPoint::RT_OUTPUT);
		m_graphicsCommandList->TransitionBarrier(texture->m_textureResource, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES, true);

		TemporaryDescriptorAllocation tempAlloc = m_bindlessSRVHeap->AllocateTemporary(1);

		uint32_t destRanges[1] = {static_cast<uint32_t>(1)};

		static const uint32_t descriptorCopyRanges[] = {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1};

		D3D12_CPU_DESCRIPTOR_HANDLE uavHandle = texture->GetUAV(nullptr);

		m_device->CopyDescriptors(1, &tempAlloc.m_cpuHandle, destRanges, static_cast<uint32_t>(1), &uavHandle, descriptorCopyRanges, m_bindlessSRVHeap->m_heapType);
		m_graphicsCommandList->m_d3dCommandList->SetComputeRootDescriptorTable(static_cast<int>(GlobalRTRootSignatureParameters::OUPUT_VIEW), tempAlloc.m_gpuHandle);
	}

	{
		m_graphicsCommandList->TransitionBarrier(m_noisyRenderTarget->m_textureResource, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES, true);

		TemporaryDescriptorAllocation tempAlloc = m_bindlessSRVHeap->AllocateTemporary(1);

		uint32_t destRanges[1] = {static_cast<uint32_t>(1)};

		static const uint32_t descriptorCopyRanges[] = {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1};

		D3D12_CPU_DESCRIPTOR_HANDLE uavHandle = m_noisyRenderTarget->GetUAV(nullptr);

		m_device->CopyDescriptors(1, &tempAlloc.m_cpuHandle, destRanges, static_cast<uint32_t>(1), &uavHandle, descriptorCopyRanges, m_bindlessSRVHeap->m_heapType);
		m_graphicsCommandList->m_d3dCommandList->SetComputeRootDescriptorTable(static_cast<int>(GlobalRTRootSignatureParameters::PREVIOUS_FRAME), tempAlloc.m_gpuHandle);
	}

	m_graphicsCommandList->m_d3dCommandList->SetComputeRootDescriptorTable(static_cast<int>(GlobalRTRootSignatureParameters::SRV_BINDLESS), m_bindlessSRVHeap->m_gpuStart[m_bindlessSRVHeap->m_heapIndex]);

	D3D12_DISPATCH_RAYS_DESC dispatchRaysDesc = {};
	dispatchRaysDesc.HitGroupTable.StartAddress = m_hitGroupShaderTable->GetGPUVirtualAddress();
	dispatchRaysDesc.HitGroupTable.SizeInBytes = m_hitGroupShaderTable->GetDesc().Width;
	dispatchRaysDesc.HitGroupTable.StrideInBytes = m_hitGroupShaderTable->GetShaderRecordSize();

	dispatchRaysDesc.MissShaderTable.StartAddress = m_missShaderTable->GetGPUVirtualAddress();
	dispatchRaysDesc.MissShaderTable.SizeInBytes = m_missShaderTable->GetDesc().Width;
	dispatchRaysDesc.MissShaderTable.StrideInBytes = m_missShaderTable->GetShaderRecordSize();

	dispatchRaysDesc.RayGenerationShaderRecord.StartAddress = m_rayGenShaderTable->GetGPUVirtualAddress();
	dispatchRaysDesc.RayGenerationShaderRecord.SizeInBytes = m_rayGenShaderTable->GetDesc().Width;
	dispatchRaysDesc.Width = g_theWindow->GetClientDimensions().x;
	dispatchRaysDesc.Height = g_theWindow->GetClientDimensions().y;
	dispatchRaysDesc.Depth = 1;

	m_graphicsCommandList->SetComputeDynamicConstantBuffer(static_cast<int>(GlobalRTRootSignatureParameters::DEBUG_CB), g_rtDebugInfo);
	m_graphicsCommandList->SetComputeDynamicConstantBuffer(static_cast<int>(GlobalRTRootSignatureParameters::APP_CB),	g_rtAppSettings);
	m_graphicsCommandList->SetComputeDynamicConstantBuffer(static_cast<int>(GlobalRTRootSignatureParameters::CAMERA_CB), g_rtCameraConsts);
	m_graphicsCommandList->SetComputeDynamicConstantBuffer(static_cast<uint32_t>(GlobalRTRootSignatureParameters::SCENE_CB), g_rtSceneConsts);
	m_graphicsCommandList->SetComputeDynamicConstantBuffer(static_cast<uint32_t>(GlobalRTRootSignatureParameters::LIGHT_CB), g_rtLightConsts);

	m_graphicsCommandList->m_d3dCommandList->SetComputeRootShaderResourceView(static_cast<UINT>(GlobalRTRootSignatureParameters::ACCELERATION_STRUCTURE), tlas->m_tlasResource->GetGPUVirtualAddress());
	m_graphicsCommandList->m_d3dCommandList->SetPipelineState1(m_rtPSO);
	m_graphicsCommandList->m_d3dCommandList->DispatchRays(&dispatchRaysDesc);
	PIXEndEvent(m_graphicsCommandList->m_d3dCommandList);
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void DX12Renderer::ComputeGBuffers(TopLevelAS* tlas)
{
	PIXBeginEvent(m_graphicsCommandList->m_d3dCommandList, PIX_COLOR_DEFAULT, "Compute GBuffers");
	// Gbuffer Pass

	ID3D12DescriptorHeap* heaps[] = {m_bindlessSRVHeap->CurrentHeap()};
	m_graphicsCommandList->m_d3dCommandList->SetDescriptorHeaps(1, heaps);

	m_gBuffers->PrepareGBuffersForDispatch(m_graphicsCommandList, m_bindlessSRVHeap);
	m_graphicsCommandList->m_d3dCommandList->SetComputeRootDescriptorTable(static_cast<int>(GlobalRTRootSignatureParameters::SRV_BINDLESS), m_bindlessSRVHeap->m_gpuStart[m_bindlessSRVHeap->m_heapIndex]);

	//Reservoirs
	{
		// Temporal Reservoir
		{
			m_graphicsCommandList->TransitionBarrier(m_temporalReservoirBuffer->m_buffer, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES, true);

			TemporaryDescriptorAllocation tempAlloc = m_bindlessSRVHeap->AllocateTemporary(1);

			uint32_t destRanges[1] = {static_cast<uint32_t>(1)};

			static const uint32_t descriptorCopyRanges[] = {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1};

			D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
			uavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
			uavDesc.Format = DXGI_FORMAT_UNKNOWN;
			uavDesc.Buffer.FirstElement = 0;
			uavDesc.Buffer.NumElements = static_cast<UINT>(m_temporalReservoirBuffer->m_numElements);
			uavDesc.Buffer.StructureByteStride = static_cast<UINT>(m_temporalReservoirBuffer->m_sizeOfEachElement);
			uavDesc.Buffer.CounterOffsetInBytes = 0;
			uavDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;

			D3D12_CPU_DESCRIPTOR_HANDLE uavHandle = m_temporalReservoirBuffer->GetUAV(&uavDesc);

			m_device->CopyDescriptors(1, &tempAlloc.m_cpuHandle, destRanges, static_cast<uint32_t>(1), &uavHandle, descriptorCopyRanges, m_bindlessSRVHeap->m_heapType);
			m_graphicsCommandList->m_d3dCommandList->SetComputeRootDescriptorTable(static_cast<int>(GlobalRTRootSignatureParameters::TEMPORAL_RESERVOIR), tempAlloc.m_gpuHandle);
		}

		// Prev
		{
			m_graphicsCommandList->TransitionBarrier(m_prevReservoirBuffer->m_buffer, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES, true);

			TemporaryDescriptorAllocation tempAlloc = m_bindlessSRVHeap->AllocateTemporary(1);

			uint32_t destRanges[1] = {static_cast<uint32_t>(1)};

			static const uint32_t descriptorCopyRanges[] = {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1};

			D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
			uavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
			uavDesc.Format = DXGI_FORMAT_UNKNOWN;
			uavDesc.Buffer.FirstElement = 0;
			uavDesc.Buffer.NumElements = static_cast<UINT>(m_prevReservoirBuffer->m_numElements);
			uavDesc.Buffer.StructureByteStride = static_cast<UINT>(m_prevReservoirBuffer->m_sizeOfEachElement);
			uavDesc.Buffer.CounterOffsetInBytes = 0;
			uavDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;

			D3D12_CPU_DESCRIPTOR_HANDLE uavHandle = m_prevReservoirBuffer->GetUAV(&uavDesc);

			m_device->CopyDescriptors(1, &tempAlloc.m_cpuHandle, destRanges, static_cast<uint32_t>(1), &uavHandle, descriptorCopyRanges, m_bindlessSRVHeap->m_heapType);
			m_graphicsCommandList->m_d3dCommandList->SetComputeRootDescriptorTable(static_cast<int>(GlobalRTRootSignatureParameters::PREV_RESERVOIR), tempAlloc.m_gpuHandle);
		}

		// Final
		{
			m_graphicsCommandList->TransitionBarrier(m_finalReservoirBuffer->m_buffer, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES, true);

			TemporaryDescriptorAllocation tempAlloc = m_bindlessSRVHeap->AllocateTemporary(1);

			uint32_t destRanges[1] = {static_cast<uint32_t>(1)};

			static const uint32_t descriptorCopyRanges[] = {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1};

			D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
			uavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
			uavDesc.Format = DXGI_FORMAT_UNKNOWN;
			uavDesc.Buffer.FirstElement = 0;
			uavDesc.Buffer.NumElements = static_cast<UINT>(m_finalReservoirBuffer->m_numElements);
			uavDesc.Buffer.StructureByteStride = static_cast<UINT>(m_finalReservoirBuffer->m_sizeOfEachElement);
			uavDesc.Buffer.CounterOffsetInBytes = 0;
			uavDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;

			D3D12_CPU_DESCRIPTOR_HANDLE uavHandle = m_finalReservoirBuffer->GetUAV(&uavDesc);

			m_device->CopyDescriptors(1, &tempAlloc.m_cpuHandle, destRanges, static_cast<uint32_t>(1), &uavHandle, descriptorCopyRanges, m_bindlessSRVHeap->m_heapType);
			m_graphicsCommandList->m_d3dCommandList->SetComputeRootDescriptorTable(static_cast<int>(GlobalRTRootSignatureParameters::FINAL_RESERVOIR), tempAlloc.m_gpuHandle);
		}

	}
 
  	D3D12_DISPATCH_RAYS_DESC dispatchRaysDesc = {};
  	dispatchRaysDesc.HitGroupTable.StartAddress = m_hitGroupShaderTable_gBuffer->GetGPUVirtualAddress();
  	dispatchRaysDesc.HitGroupTable.SizeInBytes = m_hitGroupShaderTable_gBuffer->GetDesc().Width;
  	dispatchRaysDesc.HitGroupTable.StrideInBytes = m_hitGroupShaderTable_gBuffer->GetShaderRecordSize();
  
  	dispatchRaysDesc.MissShaderTable.StartAddress = m_missShaderTable_gBuffer->GetGPUVirtualAddress();
  	dispatchRaysDesc.MissShaderTable.SizeInBytes = m_missShaderTable_gBuffer->GetDesc().Width;
  	dispatchRaysDesc.MissShaderTable.StrideInBytes = m_missShaderTable_gBuffer->GetShaderRecordSize();
  
  	dispatchRaysDesc.RayGenerationShaderRecord.StartAddress = m_rayGenShaderTable_gBuffer->GetGPUVirtualAddress();
  	dispatchRaysDesc.RayGenerationShaderRecord.SizeInBytes = m_rayGenShaderTable_gBuffer->GetDesc().Width;
  	dispatchRaysDesc.Width = g_theWindow->GetClientDimensions().x;
  	dispatchRaysDesc.Height = g_theWindow->GetClientDimensions().y;
  	dispatchRaysDesc.Depth = 1;

	m_graphicsCommandList->SetComputeDynamicConstantBuffer(static_cast<int>(GlobalRTRootSignatureParameters::APP_CB), g_rtAppSettings);
	m_graphicsCommandList->SetComputeDynamicConstantBuffer(static_cast<int>(GlobalRTRootSignatureParameters::CAMERA_CB), g_rtCameraConsts);
	m_graphicsCommandList->SetComputeDynamicConstantBuffer(static_cast<uint32_t>(GlobalRTRootSignatureParameters::SCENE_CB), g_rtSceneConsts);
	m_graphicsCommandList->SetComputeDynamicConstantBuffer(static_cast<uint32_t>(GlobalRTRootSignatureParameters::LIGHT_CB), g_rtLightConsts);

	m_graphicsCommandList->m_d3dCommandList->SetComputeRootShaderResourceView(static_cast<UINT>(GlobalRTRootSignatureParameters::ACCELERATION_STRUCTURE), tlas->m_tlasResource->GetGPUVirtualAddress());
	m_graphicsCommandList->m_d3dCommandList->SetPipelineState1(m_gBufferPSO);
	m_graphicsCommandList->m_d3dCommandList->DispatchRays(&dispatchRaysDesc);

	PIXEndEvent(m_graphicsCommandList->m_d3dCommandList);
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void DX12Renderer::PerformTemporalReuse()
{
	PIXBeginEvent(m_graphicsCommandList->m_d3dCommandList, PIX_COLOR_DEFAULT, "Temporal Reuse");

	// Set compute PSO
	m_graphicsCommandList->SetPipelineState(*m_temporalReusePSO);

	// Dispatch
	uint32_t width = g_theWindow->GetClientDimensions().x;
	uint32_t height = g_theWindow->GetClientDimensions().y;
	m_graphicsCommandList->m_d3dCommandList->Dispatch(
		(width + 7) / 8,
		(height + 7) / 8,
		1
	);

	PIXEndEvent(m_graphicsCommandList->m_d3dCommandList);
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void DX12Renderer::PerformSpatialReuse()
{
	PIXBeginEvent(m_graphicsCommandList->m_d3dCommandList, PIX_COLOR_DEFAULT, "Spatial Reuse");

	// Set compute PSO
	m_graphicsCommandList->SetPipelineState(*m_spatialReusePSO);

	// Dispatch
	uint32_t width = g_theWindow->GetClientDimensions().x;
	uint32_t height = g_theWindow->GetClientDimensions().y;
	m_graphicsCommandList->m_d3dCommandList->Dispatch((width + 7) / 8, (height + 7) / 8, 1);

	PIXEndEvent(m_graphicsCommandList->m_d3dCommandList);
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void DX12Renderer::Denoise()
{
	PIXBeginEvent(m_graphicsCommandList->m_d3dCommandList, PIX_COLOR_DEFAULT, "Denoising");

	// Set compute PSO
	m_graphicsCommandList->SetPipelineState(*m_denoiserPSO);

	// Dispatch
	uint32_t width = g_theWindow->GetClientDimensions().x;
	uint32_t height = g_theWindow->GetClientDimensions().y;
	m_graphicsCommandList->m_d3dCommandList->Dispatch((width + 7) / 8, (height + 7) / 8, 1);

	PIXEndEvent(m_graphicsCommandList->m_d3dCommandList);

}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void DX12Renderer::CreateDefaultRootSignature()
{
	
	//---------Root Sig Flags------------
	D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags =
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;

	D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData = {};
	featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;

	HRESULT hr = m_device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &featureData, sizeof(featureData));
	if(FAILED(hr))
	{
		featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
	}

	//------Root Signature Descriptor Ranges------
	// For Texture

	D3D12_DESCRIPTOR_RANGE1 diffuseTextureRange = {};
	diffuseTextureRange.RangeType							= D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	diffuseTextureRange.NumDescriptors						= 1;          
	diffuseTextureRange.BaseShaderRegister					= 0;      
	diffuseTextureRange.RegisterSpace						= 0;
	diffuseTextureRange.Flags								= D3D12_DESCRIPTOR_RANGE_FLAG_NONE;
	diffuseTextureRange.OffsetInDescriptorsFromTableStart	= D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	D3D12_DESCRIPTOR_RANGE1 normalTextureRange = {};
	normalTextureRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	normalTextureRange.NumDescriptors = 1;
	normalTextureRange.BaseShaderRegister = 1;
	normalTextureRange.RegisterSpace = 0;
	normalTextureRange.Flags = D3D12_DESCRIPTOR_RANGE_FLAG_NONE;
	normalTextureRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	D3D12_DESCRIPTOR_RANGE1 sgeTextureRange = {};
	sgeTextureRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	sgeTextureRange.NumDescriptors = 1;
	sgeTextureRange.BaseShaderRegister = 2;
	sgeTextureRange.RegisterSpace = 0;
	sgeTextureRange.Flags = D3D12_DESCRIPTOR_RANGE_FLAG_NONE;
	sgeTextureRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;


	//-----Root Parameters-------
	D3D12_ROOT_PARAMETER1 rootParameters[static_cast<int>(RootParameters::COUNT)] = {};
	
	rootParameters[static_cast<int>(RootParameters::DEBUG_CB)].ParameterType				= D3D12_ROOT_PARAMETER_TYPE_CBV;
	rootParameters[static_cast<int>(RootParameters::DEBUG_CB)].ShaderVisibility				= D3D12_SHADER_VISIBILITY_PIXEL;
	rootParameters[static_cast<int>(RootParameters::DEBUG_CB)].Descriptor.ShaderRegister	= 1;
	rootParameters[static_cast<int>(RootParameters::DEBUG_CB)].Descriptor.RegisterSpace		= 0;
	
	rootParameters[static_cast<int>(RootParameters::CAMERA_CB)].ParameterType				= D3D12_ROOT_PARAMETER_TYPE_CBV;
	rootParameters[static_cast<int>(RootParameters::CAMERA_CB)].ShaderVisibility			= D3D12_SHADER_VISIBILITY_ALL;
	rootParameters[static_cast<int>(RootParameters::CAMERA_CB)].Descriptor.ShaderRegister	= k_cameraConstantsRegister;
	rootParameters[static_cast<int>(RootParameters::CAMERA_CB)].Descriptor.RegisterSpace	= k_cameraConstantsSpace;

	rootParameters[static_cast<int>(RootParameters::MODEL_CB)].ParameterType				= D3D12_ROOT_PARAMETER_TYPE_CBV;
	rootParameters[static_cast<int>(RootParameters::MODEL_CB)].ShaderVisibility				= D3D12_SHADER_VISIBILITY_ALL;
	rootParameters[static_cast<int>(RootParameters::MODEL_CB)].Descriptor.ShaderRegister	= k_modelConstantRegister;
	rootParameters[static_cast<int>(RootParameters::MODEL_CB)].Descriptor.RegisterSpace		= k_modelConstantSpace;

	rootParameters[static_cast<int>(RootParameters::LIGHTS_CB)].ParameterType				= D3D12_ROOT_PARAMETER_TYPE_CBV;
	rootParameters[static_cast<int>(RootParameters::LIGHTS_CB)].ShaderVisibility			= D3D12_SHADER_VISIBILITY_PIXEL;
	rootParameters[static_cast<int>(RootParameters::LIGHTS_CB)].Descriptor.ShaderRegister	= k_lightConstantRegister;
	rootParameters[static_cast<int>(RootParameters::LIGHTS_CB)].Descriptor.RegisterSpace	= k_lightConstantSpace;

	rootParameters[static_cast<int>(RootParameters::DIFFUSE_TEXTURE_DT)].ParameterType				= D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParameters[static_cast<int>(RootParameters::DIFFUSE_TEXTURE_DT)].ShaderVisibility			= D3D12_SHADER_VISIBILITY_PIXEL;
	rootParameters[static_cast<int>(RootParameters::DIFFUSE_TEXTURE_DT)].DescriptorTable.NumDescriptorRanges = 1;
	rootParameters[static_cast<int>(RootParameters::DIFFUSE_TEXTURE_DT)].DescriptorTable.pDescriptorRanges = &diffuseTextureRange;

	rootParameters[static_cast<int>(RootParameters::NORMAL_TEXTURE_DT)].ParameterType			= D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParameters[static_cast<int>(RootParameters::NORMAL_TEXTURE_DT)].ShaderVisibility		= D3D12_SHADER_VISIBILITY_PIXEL;
	rootParameters[static_cast<int>(RootParameters::NORMAL_TEXTURE_DT)].DescriptorTable.NumDescriptorRanges = 1;
	rootParameters[static_cast<int>(RootParameters::NORMAL_TEXTURE_DT)].DescriptorTable.pDescriptorRanges = &normalTextureRange;

	rootParameters[static_cast<int>(RootParameters::SGE_TEXTURE_DT)].ParameterType			= D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParameters[static_cast<int>(RootParameters::SGE_TEXTURE_DT)].ShaderVisibility		= D3D12_SHADER_VISIBILITY_PIXEL;
	rootParameters[static_cast<int>(RootParameters::SGE_TEXTURE_DT)].DescriptorTable.NumDescriptorRanges = 1;
	rootParameters[static_cast<int>(RootParameters::SGE_TEXTURE_DT)].DescriptorTable.pDescriptorRanges = &sgeTextureRange;

	D3D12_STATIC_SAMPLER_DESC staticSamplerDesc = {};
	staticSamplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
	staticSamplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	staticSamplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	staticSamplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	staticSamplerDesc.ShaderRegister = 0;
	staticSamplerDesc.RegisterSpace = 0;
	staticSamplerDesc.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	D3D12_VERSIONED_ROOT_SIGNATURE_DESC rootSigDesc = {};
	rootSigDesc.Version								= D3D_ROOT_SIGNATURE_VERSION_1_1;
	rootSigDesc.Desc_1_1.NumParameters				= static_cast<int>(RootParameters::COUNT);
	rootSigDesc.Desc_1_1.pParameters				= rootParameters;
	rootSigDesc.Desc_1_1.NumStaticSamplers			= 1;
	rootSigDesc.Desc_1_1.pStaticSamplers			= &staticSamplerDesc;
	rootSigDesc.Desc_1_1.Flags						= rootSignatureFlags;

	m_defaultRootSignature = new RootSignature(m_device, "Default", rootSigDesc.Desc_1_1, featureData.HighestVersion);
}


//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void DX12Renderer::CreateRayTracingRootSignatures()
{
	CreateRaytracingGlobalRootSignature();
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void DX12Renderer::CreateRaytracingGlobalRootSignature()
{
	D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData = {};
	featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;

	HRESULT hr = m_device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &featureData, sizeof(featureData));
	if(FAILED(hr))
	{
		featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
	}

	// Global Root Signature
	// Root Signature Descriptor Ranges
	// UAV for RWTexture to render the ray traced output to (u0)
	D3D12_DESCRIPTOR_RANGE1 renderTargetUAVRange			= {};
	renderTargetUAVRange.RangeType							= D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
	renderTargetUAVRange.NumDescriptors						= 1;
	renderTargetUAVRange.BaseShaderRegister					= 0;
	renderTargetUAVRange.RegisterSpace						= 0;
	renderTargetUAVRange.Flags								= D3D12_DESCRIPTOR_RANGE_FLAG_NONE;
	renderTargetUAVRange.OffsetInDescriptorsFromTableStart	= D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	D3D12_DESCRIPTOR_RANGE1 noisyRenderTargetUAV			= {};
	noisyRenderTargetUAV.RangeType							= D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
	noisyRenderTargetUAV.NumDescriptors						= 1;
	noisyRenderTargetUAV.BaseShaderRegister					= 0;
	noisyRenderTargetUAV.RegisterSpace						= 1;
	noisyRenderTargetUAV.Flags								= D3D12_DESCRIPTOR_RANGE_FLAG_NONE;
	noisyRenderTargetUAV.OffsetInDescriptorsFromTableStart	= D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	D3D12_DESCRIPTOR_RANGE1 gBufferRanges[GBUFFER_COUNT] = {};

	for(int i = 0; i < GBUFFER_COUNT; ++i)
	{
		gBufferRanges[i].RangeType			= D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
		gBufferRanges[i].NumDescriptors		= 1;
		gBufferRanges[i].BaseShaderRegister = 1;
		gBufferRanges[i].RegisterSpace		= i;
		gBufferRanges[i].Flags				= D3D12_DESCRIPTOR_RANGE_FLAG_NONE;
		gBufferRanges[i].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
	}
	
	// Reservoir Buffers
	D3D12_DESCRIPTOR_RANGE1 finalReservoirBuffer			= {};
	finalReservoirBuffer.RangeType							= D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
	finalReservoirBuffer.NumDescriptors						= 1;
	finalReservoirBuffer.BaseShaderRegister					= k_reservoirBufferUAVRegister;
	finalReservoirBuffer.RegisterSpace						= k_finalReservoirBufferSpace;
	finalReservoirBuffer.Flags								= D3D12_DESCRIPTOR_RANGE_FLAG_NONE;
	finalReservoirBuffer.OffsetInDescriptorsFromTableStart	= D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	D3D12_DESCRIPTOR_RANGE1 prevReservoirBuffer				= {};
	prevReservoirBuffer.RangeType							= D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
	prevReservoirBuffer.NumDescriptors						= 1;
	prevReservoirBuffer.BaseShaderRegister					= k_reservoirBufferUAVRegister;
	prevReservoirBuffer.RegisterSpace						= k_prevFrameReservoirBufferSpace;
	prevReservoirBuffer.Flags								= D3D12_DESCRIPTOR_RANGE_FLAG_NONE;
	prevReservoirBuffer.OffsetInDescriptorsFromTableStart	= D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	D3D12_DESCRIPTOR_RANGE1 temporalReservoirBuffer				= {};
	temporalReservoirBuffer.RangeType							= D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
	temporalReservoirBuffer.NumDescriptors						= 1;
	temporalReservoirBuffer.BaseShaderRegister					= k_reservoirBufferUAVRegister;
	temporalReservoirBuffer.RegisterSpace						= k_temporalReservoirBufferSpace;
	temporalReservoirBuffer.Flags								= D3D12_DESCRIPTOR_RANGE_FLAG_NONE;
	temporalReservoirBuffer.OffsetInDescriptorsFromTableStart	= D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	D3D12_DESCRIPTOR_RANGE1 bufferRanges[4] = {};
	bufferRanges[0] = {};
	bufferRanges[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	bufferRanges[0].NumDescriptors = UINT_MAX;
	bufferRanges[0].BaseShaderRegister = k_rtVBRegister;
	bufferRanges[0].RegisterSpace = k_rtVBSpace;
	bufferRanges[0].Flags = D3D12_DESCRIPTOR_RANGE_FLAG_NONE;
	bufferRanges[0].OffsetInDescriptorsFromTableStart = 0;
	bufferRanges[0].Flags = D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE;

	bufferRanges[1] = {};
	bufferRanges[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	bufferRanges[1].NumDescriptors = UINT_MAX;
	bufferRanges[1].BaseShaderRegister = k_rtIBRegister;
	bufferRanges[1].RegisterSpace = k_rtIBSpace;
	bufferRanges[1].Flags = D3D12_DESCRIPTOR_RANGE_FLAG_NONE;
	bufferRanges[1].OffsetInDescriptorsFromTableStart = 0;
	bufferRanges[1].Flags = D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE;

	bufferRanges[2] = {};
	bufferRanges[2].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	bufferRanges[2].NumDescriptors = UINT_MAX;
	bufferRanges[2].BaseShaderRegister = 0;
	bufferRanges[2].RegisterSpace = 3;
	bufferRanges[2].Flags = D3D12_DESCRIPTOR_RANGE_FLAG_NONE;
	bufferRanges[2].OffsetInDescriptorsFromTableStart = 0;
	bufferRanges[2].Flags = D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE;

	bufferRanges[3] = {};
	bufferRanges[3].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	bufferRanges[3].NumDescriptors = UINT_MAX;
	bufferRanges[3].BaseShaderRegister = 0;
	bufferRanges[3].RegisterSpace = 4;
	bufferRanges[3].Flags = D3D12_DESCRIPTOR_RANGE_FLAG_NONE;
	bufferRanges[3].OffsetInDescriptorsFromTableStart = 0;
	bufferRanges[3].Flags = D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE;

	// Params - UAV - Render Texture
	D3D12_ROOT_PARAMETER1 globalRootParameters[static_cast<int>(GlobalRTRootSignatureParameters::COUNT)]					= {};
	globalRootParameters[static_cast<int>(GlobalRTRootSignatureParameters::OUPUT_VIEW)].ParameterType						= D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	globalRootParameters[static_cast<int>(GlobalRTRootSignatureParameters::OUPUT_VIEW)].ShaderVisibility					= D3D12_SHADER_VISIBILITY_ALL;
	globalRootParameters[static_cast<int>(GlobalRTRootSignatureParameters::OUPUT_VIEW)].DescriptorTable.NumDescriptorRanges = 1;
	globalRootParameters[static_cast<int>(GlobalRTRootSignatureParameters::OUPUT_VIEW)].DescriptorTable.pDescriptorRanges	= &renderTargetUAVRange;

	globalRootParameters[static_cast<int>(GlobalRTRootSignatureParameters::PREVIOUS_FRAME)].ParameterType						= D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	globalRootParameters[static_cast<int>(GlobalRTRootSignatureParameters::PREVIOUS_FRAME)].ShaderVisibility					= D3D12_SHADER_VISIBILITY_ALL;
	globalRootParameters[static_cast<int>(GlobalRTRootSignatureParameters::PREVIOUS_FRAME)].DescriptorTable.NumDescriptorRanges = 1;
	globalRootParameters[static_cast<int>(GlobalRTRootSignatureParameters::PREVIOUS_FRAME)].DescriptorTable.pDescriptorRanges	= &noisyRenderTargetUAV;

	globalRootParameters[static_cast<int>(GlobalRTRootSignatureParameters::FINAL_RESERVOIR)].ParameterType						= D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	globalRootParameters[static_cast<int>(GlobalRTRootSignatureParameters::FINAL_RESERVOIR)].ShaderVisibility						= D3D12_SHADER_VISIBILITY_ALL;
	globalRootParameters[static_cast<int>(GlobalRTRootSignatureParameters::FINAL_RESERVOIR)].DescriptorTable.NumDescriptorRanges	= 1;
	globalRootParameters[static_cast<int>(GlobalRTRootSignatureParameters::FINAL_RESERVOIR)].DescriptorTable.pDescriptorRanges	= &finalReservoirBuffer;

	globalRootParameters[static_cast<int>(GlobalRTRootSignatureParameters::PREV_RESERVOIR)].ParameterType						= D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	globalRootParameters[static_cast<int>(GlobalRTRootSignatureParameters::PREV_RESERVOIR)].ShaderVisibility					= D3D12_SHADER_VISIBILITY_ALL;
	globalRootParameters[static_cast<int>(GlobalRTRootSignatureParameters::PREV_RESERVOIR)].DescriptorTable.NumDescriptorRanges	= 1;
	globalRootParameters[static_cast<int>(GlobalRTRootSignatureParameters::PREV_RESERVOIR)].DescriptorTable.pDescriptorRanges	= &prevReservoirBuffer;

	globalRootParameters[static_cast<int>(GlobalRTRootSignatureParameters::TEMPORAL_RESERVOIR)].ParameterType						= D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	globalRootParameters[static_cast<int>(GlobalRTRootSignatureParameters::TEMPORAL_RESERVOIR)].ShaderVisibility					= D3D12_SHADER_VISIBILITY_ALL;
	globalRootParameters[static_cast<int>(GlobalRTRootSignatureParameters::TEMPORAL_RESERVOIR)].DescriptorTable.NumDescriptorRanges	= 1;
	globalRootParameters[static_cast<int>(GlobalRTRootSignatureParameters::TEMPORAL_RESERVOIR)].DescriptorTable.pDescriptorRanges	= &temporalReservoirBuffer;

	int gBufferParamStartIndex = static_cast<int>(GlobalRTRootSignatureParameters::GBUFFER_POSITION);


	for(int i = 0; i < GBUFFER_COUNT; ++i)
	{
		globalRootParameters[gBufferParamStartIndex + i].ParameterType							= D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		globalRootParameters[gBufferParamStartIndex + i].ShaderVisibility						= D3D12_SHADER_VISIBILITY_ALL;
		globalRootParameters[gBufferParamStartIndex + i].DescriptorTable.NumDescriptorRanges	= 1;
		globalRootParameters[gBufferParamStartIndex + i].DescriptorTable.pDescriptorRanges		= &gBufferRanges[i];
	}
	
	// TLAS as SRV (Descriptor)
	globalRootParameters[static_cast<int>(GlobalRTRootSignatureParameters::ACCELERATION_STRUCTURE)].ParameterType				= D3D12_ROOT_PARAMETER_TYPE_SRV;
	globalRootParameters[static_cast<int>(GlobalRTRootSignatureParameters::ACCELERATION_STRUCTURE)].ShaderVisibility			= D3D12_SHADER_VISIBILITY_ALL;
	globalRootParameters[static_cast<int>(GlobalRTRootSignatureParameters::ACCELERATION_STRUCTURE)].Descriptor.ShaderRegister	= k_tlasRegister;
	globalRootParameters[static_cast<int>(GlobalRTRootSignatureParameters::ACCELERATION_STRUCTURE)].Descriptor.RegisterSpace	= k_tlasSpace;

	// Debug
	globalRootParameters[static_cast<int>(GlobalRTRootSignatureParameters::DEBUG_CB)].ParameterType				= D3D12_ROOT_PARAMETER_TYPE_CBV;
	globalRootParameters[static_cast<int>(GlobalRTRootSignatureParameters::DEBUG_CB)].ShaderVisibility			= D3D12_SHADER_VISIBILITY_ALL;
	globalRootParameters[static_cast<int>(GlobalRTRootSignatureParameters::DEBUG_CB)].Descriptor.ShaderRegister = k_rtDebugConstantRegister;
	globalRootParameters[static_cast<int>(GlobalRTRootSignatureParameters::DEBUG_CB)].Descriptor.RegisterSpace	= k_rtDebugConstantSpace;

	// App Settings
	globalRootParameters[static_cast<int>(GlobalRTRootSignatureParameters::APP_CB)].ParameterType				= D3D12_ROOT_PARAMETER_TYPE_CBV;
	globalRootParameters[static_cast<int>(GlobalRTRootSignatureParameters::APP_CB)].ShaderVisibility			= D3D12_SHADER_VISIBILITY_ALL;
	globalRootParameters[static_cast<int>(GlobalRTRootSignatureParameters::APP_CB)].Descriptor.ShaderRegister	= k_rtAppSettingsRegister;
	globalRootParameters[static_cast<int>(GlobalRTRootSignatureParameters::APP_CB)].Descriptor.RegisterSpace	= k_rtAppSettingsSpace;

	// Camera
	globalRootParameters[static_cast<int>(GlobalRTRootSignatureParameters::CAMERA_CB)].ParameterType				= D3D12_ROOT_PARAMETER_TYPE_CBV;
	globalRootParameters[static_cast<int>(GlobalRTRootSignatureParameters::CAMERA_CB)].ShaderVisibility				= D3D12_SHADER_VISIBILITY_ALL;
	globalRootParameters[static_cast<int>(GlobalRTRootSignatureParameters::CAMERA_CB)].Descriptor.ShaderRegister	= k_rtCameraConstantsRegister;
	globalRootParameters[static_cast<int>(GlobalRTRootSignatureParameters::CAMERA_CB)].Descriptor.RegisterSpace		= k_rtCameraConstantsSpace;
	
	// Scene
	globalRootParameters[static_cast<int>(GlobalRTRootSignatureParameters::SCENE_CB)].ParameterType					= D3D12_ROOT_PARAMETER_TYPE_CBV;
	globalRootParameters[static_cast<int>(GlobalRTRootSignatureParameters::SCENE_CB)].ShaderVisibility				= D3D12_SHADER_VISIBILITY_ALL;
	globalRootParameters[static_cast<int>(GlobalRTRootSignatureParameters::SCENE_CB)].Descriptor.ShaderRegister		= k_rtSceneConstantRegister;
	globalRootParameters[static_cast<int>(GlobalRTRootSignatureParameters::SCENE_CB)].Descriptor.RegisterSpace		= k_rtSceneConstantSpace;
	
	// Light
	globalRootParameters[static_cast<int>(GlobalRTRootSignatureParameters::LIGHT_CB)].ParameterType					= D3D12_ROOT_PARAMETER_TYPE_CBV;
	globalRootParameters[static_cast<int>(GlobalRTRootSignatureParameters::LIGHT_CB)].ShaderVisibility				= D3D12_SHADER_VISIBILITY_ALL;
	globalRootParameters[static_cast<int>(GlobalRTRootSignatureParameters::LIGHT_CB)].Descriptor.ShaderRegister		= k_rtLightConstantRegister;
	globalRootParameters[static_cast<int>(GlobalRTRootSignatureParameters::LIGHT_CB)].Descriptor.RegisterSpace		= k_rtLightConstantSpace;

 	// Bindless SRV
	globalRootParameters[static_cast<int>(GlobalRTRootSignatureParameters::SRV_BINDLESS)].ParameterType							= D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	globalRootParameters[static_cast<int>(GlobalRTRootSignatureParameters::SRV_BINDLESS)].ShaderVisibility						= D3D12_SHADER_VISIBILITY_ALL;
	globalRootParameters[static_cast<int>(GlobalRTRootSignatureParameters::SRV_BINDLESS)].DescriptorTable.NumDescriptorRanges	= _countof(bufferRanges);
	globalRootParameters[static_cast<int>(GlobalRTRootSignatureParameters::SRV_BINDLESS)].DescriptorTable.pDescriptorRanges		= bufferRanges;

	// Static Sampler
	D3D12_STATIC_SAMPLER_DESC staticSamplerDesc[static_cast<int>(SamplerMode::COUNT)] = {};

	// POINT_CLAMP
	staticSamplerDesc[static_cast<int>(SamplerMode::POINT_CLAMP)].Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
	staticSamplerDesc[static_cast<int>(SamplerMode::POINT_CLAMP)].AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	staticSamplerDesc[static_cast<int>(SamplerMode::POINT_CLAMP)].AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	staticSamplerDesc[static_cast<int>(SamplerMode::POINT_CLAMP)].AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	staticSamplerDesc[static_cast<int>(SamplerMode::POINT_CLAMP)].ShaderRegister = 0;
	staticSamplerDesc[static_cast<int>(SamplerMode::POINT_CLAMP)].RegisterSpace = 0;
	staticSamplerDesc[static_cast<int>(SamplerMode::POINT_CLAMP)].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	// BILINEAR_WRAP
	staticSamplerDesc[static_cast<int>(SamplerMode::BILINEAR_WRAP)].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	staticSamplerDesc[static_cast<int>(SamplerMode::BILINEAR_WRAP)].AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	staticSamplerDesc[static_cast<int>(SamplerMode::BILINEAR_WRAP)].AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	staticSamplerDesc[static_cast<int>(SamplerMode::BILINEAR_WRAP)].AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	staticSamplerDesc[static_cast<int>(SamplerMode::BILINEAR_WRAP)].ShaderRegister = 1;
	staticSamplerDesc[static_cast<int>(SamplerMode::BILINEAR_WRAP)].RegisterSpace = 0;
	staticSamplerDesc[static_cast<int>(SamplerMode::BILINEAR_WRAP)].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	// BILINEAR_COMPARISION_BORDER
	staticSamplerDesc[static_cast<int>(SamplerMode::BILINEAR_COMPARISION_BORDER)].Filter = D3D12_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR;
	staticSamplerDesc[static_cast<int>(SamplerMode::BILINEAR_COMPARISION_BORDER)].AddressU = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
	staticSamplerDesc[static_cast<int>(SamplerMode::BILINEAR_COMPARISION_BORDER)].AddressV = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
	staticSamplerDesc[static_cast<int>(SamplerMode::BILINEAR_COMPARISION_BORDER)].AddressW = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
	staticSamplerDesc[static_cast<int>(SamplerMode::BILINEAR_COMPARISION_BORDER)].ComparisonFunc = D3D12_COMPARISON_FUNC_LESS;
	staticSamplerDesc[static_cast<int>(SamplerMode::BILINEAR_COMPARISION_BORDER)].BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE;
	staticSamplerDesc[static_cast<int>(SamplerMode::BILINEAR_COMPARISION_BORDER)].ShaderRegister = 2;
	staticSamplerDesc[static_cast<int>(SamplerMode::BILINEAR_COMPARISION_BORDER)].RegisterSpace = 0;
	staticSamplerDesc[static_cast<int>(SamplerMode::BILINEAR_COMPARISION_BORDER)].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	// POINT_WRAP
	staticSamplerDesc[static_cast<int>(SamplerMode::POINT_WRAP)].Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
	staticSamplerDesc[static_cast<int>(SamplerMode::POINT_WRAP)].AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	staticSamplerDesc[static_cast<int>(SamplerMode::POINT_WRAP)].AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	staticSamplerDesc[static_cast<int>(SamplerMode::POINT_WRAP)].AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	staticSamplerDesc[static_cast<int>(SamplerMode::POINT_WRAP)].ShaderRegister = 3;
	staticSamplerDesc[static_cast<int>(SamplerMode::POINT_WRAP)].RegisterSpace = 0;
	staticSamplerDesc[static_cast<int>(SamplerMode::POINT_WRAP)].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	// POINT_MIRROR
	staticSamplerDesc[static_cast<int>(SamplerMode::POINT_MIRROR)].Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
	staticSamplerDesc[static_cast<int>(SamplerMode::POINT_MIRROR)].AddressU = D3D12_TEXTURE_ADDRESS_MODE_MIRROR;
	staticSamplerDesc[static_cast<int>(SamplerMode::POINT_MIRROR)].AddressV = D3D12_TEXTURE_ADDRESS_MODE_MIRROR;
	staticSamplerDesc[static_cast<int>(SamplerMode::POINT_MIRROR)].AddressW = D3D12_TEXTURE_ADDRESS_MODE_MIRROR;
	staticSamplerDesc[static_cast<int>(SamplerMode::POINT_MIRROR)].ShaderRegister = 4;
	staticSamplerDesc[static_cast<int>(SamplerMode::POINT_MIRROR)].RegisterSpace = 0;
	staticSamplerDesc[static_cast<int>(SamplerMode::POINT_MIRROR)].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	// BILINEAR_CLAMP
	staticSamplerDesc[static_cast<int>(SamplerMode::BILINEAR_CLAMP)].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	staticSamplerDesc[static_cast<int>(SamplerMode::BILINEAR_CLAMP)].AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	staticSamplerDesc[static_cast<int>(SamplerMode::BILINEAR_CLAMP)].AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	staticSamplerDesc[static_cast<int>(SamplerMode::BILINEAR_CLAMP)].AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	staticSamplerDesc[static_cast<int>(SamplerMode::BILINEAR_CLAMP)].ShaderRegister = 5;
	staticSamplerDesc[static_cast<int>(SamplerMode::BILINEAR_CLAMP)].RegisterSpace = 0;
	staticSamplerDesc[static_cast<int>(SamplerMode::BILINEAR_CLAMP)].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	// BILINEAR_MIRROR
	staticSamplerDesc[static_cast<int>(SamplerMode::BILINEAR_MIRROR)].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	staticSamplerDesc[static_cast<int>(SamplerMode::BILINEAR_MIRROR)].AddressU = D3D12_TEXTURE_ADDRESS_MODE_MIRROR;
	staticSamplerDesc[static_cast<int>(SamplerMode::BILINEAR_MIRROR)].AddressV = D3D12_TEXTURE_ADDRESS_MODE_MIRROR;
	staticSamplerDesc[static_cast<int>(SamplerMode::BILINEAR_MIRROR)].AddressW = D3D12_TEXTURE_ADDRESS_MODE_MIRROR;
	staticSamplerDesc[static_cast<int>(SamplerMode::BILINEAR_MIRROR)].ShaderRegister = 6;
	staticSamplerDesc[static_cast<int>(SamplerMode::BILINEAR_MIRROR)].RegisterSpace = 0;
	staticSamplerDesc[static_cast<int>(SamplerMode::BILINEAR_MIRROR)].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	// TRILINEAR_WRAP
	staticSamplerDesc[static_cast<int>(SamplerMode::TRILINEAR_WRAP)].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	staticSamplerDesc[static_cast<int>(SamplerMode::TRILINEAR_WRAP)].AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	staticSamplerDesc[static_cast<int>(SamplerMode::TRILINEAR_WRAP)].AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	staticSamplerDesc[static_cast<int>(SamplerMode::TRILINEAR_WRAP)].AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	staticSamplerDesc[static_cast<int>(SamplerMode::TRILINEAR_WRAP)].ShaderRegister = 7;
	staticSamplerDesc[static_cast<int>(SamplerMode::TRILINEAR_WRAP)].RegisterSpace = 0;
	staticSamplerDesc[static_cast<int>(SamplerMode::TRILINEAR_WRAP)].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	// TRILINEAR_CLAMP
	staticSamplerDesc[static_cast<int>(SamplerMode::TRILINEAR_CLAMP)].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	staticSamplerDesc[static_cast<int>(SamplerMode::TRILINEAR_CLAMP)].AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	staticSamplerDesc[static_cast<int>(SamplerMode::TRILINEAR_CLAMP)].AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	staticSamplerDesc[static_cast<int>(SamplerMode::TRILINEAR_CLAMP)].AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	staticSamplerDesc[static_cast<int>(SamplerMode::TRILINEAR_CLAMP)].ShaderRegister = 8;
	staticSamplerDesc[static_cast<int>(SamplerMode::TRILINEAR_CLAMP)].RegisterSpace = 0;
	staticSamplerDesc[static_cast<int>(SamplerMode::TRILINEAR_CLAMP)].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	// TRILINEAR_MIRROR
	staticSamplerDesc[static_cast<int>(SamplerMode::TRILINEAR_MIRROR)].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	staticSamplerDesc[static_cast<int>(SamplerMode::TRILINEAR_MIRROR)].AddressU = D3D12_TEXTURE_ADDRESS_MODE_MIRROR;
	staticSamplerDesc[static_cast<int>(SamplerMode::TRILINEAR_MIRROR)].AddressV = D3D12_TEXTURE_ADDRESS_MODE_MIRROR;
	staticSamplerDesc[static_cast<int>(SamplerMode::TRILINEAR_MIRROR)].AddressW = D3D12_TEXTURE_ADDRESS_MODE_MIRROR;
	staticSamplerDesc[static_cast<int>(SamplerMode::TRILINEAR_MIRROR)].ShaderRegister = 9;
	staticSamplerDesc[static_cast<int>(SamplerMode::TRILINEAR_MIRROR)].RegisterSpace = 0;
	staticSamplerDesc[static_cast<int>(SamplerMode::TRILINEAR_MIRROR)].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	// Root Sig Creation
	D3D12_VERSIONED_ROOT_SIGNATURE_DESC globalRootSigDesc	= {};
	globalRootSigDesc.Version								= D3D_ROOT_SIGNATURE_VERSION_1_1;
	globalRootSigDesc.Desc_1_1.NumParameters				= static_cast<int>(GlobalRTRootSignatureParameters::COUNT);
	globalRootSigDesc.Desc_1_1.pParameters					= globalRootParameters;
	globalRootSigDesc.Desc_1_1.NumStaticSamplers			= static_cast<int>(SamplerMode::COUNT);
	globalRootSigDesc.Desc_1_1.pStaticSamplers				= staticSamplerDesc;

	m_globalRTRootSignature = new RootSignature(m_device, "Global RT Root Sig", globalRootSigDesc.Desc_1_1, featureData.HighestVersion);
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void DX12Renderer::CreateClosestHitRootSignature()
{
	D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData = {};
	featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;

	HRESULT hr = m_device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &featureData, sizeof(featureData));
	if(FAILED(hr))
	{
		featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
	}

	// 	// Local Root signature - Raygen Shader - Viewport Info
	D3D12_ROOT_PARAMETER1 localRootParams[static_cast<int>(ClosestHitRootSignatureParameters::COUNT)] = {};
	localRootParams[static_cast<int>(ClosestHitRootSignatureParameters::LIGHT_CB)].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	localRootParams[static_cast<int>(ClosestHitRootSignatureParameters::LIGHT_CB)].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	localRootParams[static_cast<int>(ClosestHitRootSignatureParameters::LIGHT_CB)].Descriptor.ShaderRegister = k_lightConstantRegister;
	localRootParams[static_cast<int>(ClosestHitRootSignatureParameters::LIGHT_CB)].Descriptor.RegisterSpace = k_lightConstantSpace;

	D3D12_VERSIONED_ROOT_SIGNATURE_DESC localRootSigDesc = {};
	localRootSigDesc.Version = D3D_ROOT_SIGNATURE_VERSION_1_1;
	localRootSigDesc.Desc_1_1.NumParameters = static_cast<int>(ClosestHitRootSignatureParameters::COUNT);
	localRootSigDesc.Desc_1_1.pParameters = localRootParams;
	localRootSigDesc.Desc_1_1.NumStaticSamplers = 0;
	localRootSigDesc.Desc_1_1.Flags = D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE;

	m_closestHitRootSig = new RootSignature(m_device, "Closest Hit Root Sig", localRootSigDesc.Desc_1_1, featureData.HighestVersion);
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void DX12Renderer::CreateSRVBindlessHeap()
{
	m_bindlessSRVHeap = new BindlessDescriptorHeap(m_device, 1024, 1024, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, true, L"Bindless SRV Heap");
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void DX12Renderer::CreateDefaultPSO()
{
	m_defaultPSO = CreatePipelineStateObject(m_defaultRootSignature, RasterizerMode::SOLID_CULL_BACK, DepthMode::READ_WRITE_LESS_EQUAL, BlendMode::OPAQUE, m_defaultShader, InputLayoutType::VERTEX_PCU, "Default");
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
PipelineStateObject* DX12Renderer::CreateOrGetPipelineStateObject(RootSignature* rootSignature, Shader* shader, InputLayoutType inputLayout, RasterizerMode rasterMode, 
										BlendMode blendMode, DepthMode depthMode, bool usingDSV)
{
	std::string psoName = MakePSOName(rootSignature, shader, inputLayout, rasterMode, blendMode, depthMode, usingDSV);

	PipelineStateObject* pso = GetPipelineStateObject(psoName);

	if(pso)
	{
		return pso;
	}

	RootSignature* rootToUse = nullptr;
	Shader* shaderToUse = nullptr;

	if(rootSignature == nullptr)
	{
		rootToUse = m_defaultRootSignature;
	}
	else
	{
		rootToUse = rootSignature;
	}

	if(shader == nullptr)
	{
		shaderToUse = GetShader("Default");
	}
	else
	{
		shaderToUse = shader;
	}

	pso = CreatePipelineStateObject(rootToUse, rasterMode, depthMode, blendMode, shaderToUse, inputLayout, psoName, usingDSV);

	return pso;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
PipelineStateObject* DX12Renderer::CreateComputePSO(RootSignature* rootSig, Shader* computeShader)
{
	PipelineStateObject* csPSO = new PipelineStateObject(Stringf("%s_CS_PSO", computeShader->GetName().c_str()).c_str());

	D3D12_COMPUTE_PIPELINE_STATE_DESC computePSODesc	= {};
	computePSODesc.pRootSignature						= rootSig->GetRootSignature();
	computePSODesc.CS.pShaderBytecode					= computeShader->m_csBlob->GetBufferPointer();
	computePSODesc.CS.BytecodeLength					= computeShader->m_csBlob->GetBufferSize();

	HRESULT result = m_device->CreateComputePipelineState(&computePSODesc, IID_PPV_ARGS(&csPSO->m_pso));

	if(FAILED(result))
	{
		ERROR_AND_DIE(Stringf("Failed to create compute PSO: %s", csPSO->m_name.c_str()).c_str())
	}

	return csPSO;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
PipelineStateObject* DX12Renderer::CreatePipelineStateObject(RootSignature* rootSignature, RasterizerMode rasterMode, DepthMode depthMode, BlendMode blendMode, 
								Shader* shader, InputLayoutType inputType, std::string name, bool usingDSV)
{

	PipelineStateObject* newPSO = new PipelineStateObject(name);
	
	PipelineStateStream pss = CreatePipelineStateStream(rootSignature, rasterMode, depthMode, blendMode, shader, inputType, usingDSV);

	// stream desc and pso creation
	D3D12_PIPELINE_STATE_STREAM_DESC pipelineStateStreamDesc = {sizeof(PipelineStateStream), &pss};

	
	HRESULT hr;
	hr = m_device->CreatePipelineState(&pipelineStateStreamDesc, IID_PPV_ARGS(&newPSO->m_pso));
	if(FAILED(hr))
	{
		ERROR_AND_DIE("PSO Creation failed")
	}

	std::string debugName = Stringf("PSO: %s", newPSO->m_name.c_str());
	newPSO->m_pso->SetPrivateData(WKPDID_D3DDebugObjectName, static_cast<UINT>(strlen(debugName.c_str())), debugName.c_str());

	m_loadedPSOs.push_back(newPSO);

	return newPSO;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
RenderTarget* DX12Renderer::GetRenderTarget()
{
	m_renderTarget->AttachTexture(AttachmentPoint::RASTER_OUTPUT, m_backBuffers[m_currentBackBufferIndex]);
	return m_renderTarget;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
RootSignature* DX12Renderer::GetRootSignature(std::string name)
{
	UNUSED(name)
	return m_defaultRootSignature;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
PipelineStateObject* DX12Renderer::GetPipelineStateObject(std::string name)
{
	for(PipelineStateObject* pso : m_loadedPSOs)
	{
		if(pso->m_name == name)
		{
			return pso;
		}
	}
	return nullptr;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
PipelineStateStream DX12Renderer::CreatePipelineStateStream(RootSignature* rootSignature, RasterizerMode rasterMode, DepthMode depthMode, BlendMode blendMode,
															Shader* shader, InputLayoutType inputLayout, bool usingDSV)
{
	UNUSED(rootSignature)

	PipelineStateStream pss = {};
	
	D3D12_RT_FORMAT_ARRAY rtvFormats = {};
	rtvFormats.NumRenderTargets = 1;
	rtvFormats.RTFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;

	// Root Signature
	pss.m_rootSignatureSubObject = {};
	pss.m_rootSignatureSubObject.m_subObjectType = D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_ROOT_SIGNATURE;
	pss.m_rootSignatureSubObject.m_rootSignature = m_defaultRootSignature->m_d3dRootSignature;

	// Input Layout
	pss.m_inputLayoutSubObject = {};
	pss.m_inputLayoutSubObject.m_subObjectType = D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_INPUT_LAYOUT;
	pss.m_inputLayoutSubObject.m_inputLayoutDesc = GetInputLayoutDesc(inputLayout);

	// primitive topology
	pss.m_primitiveTopologyTypeSubObject = {};
	pss.m_primitiveTopologyTypeSubObject.m_subObjectType			= D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_PRIMITIVE_TOPOLOGY;
	pss.m_primitiveTopologyTypeSubObject.m_primitiveTopologyType	= D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

	// Shader Byte code
	pss.m_vsByteCodeSubObject = {};
	pss.m_vsByteCodeSubObject.m_subObjectType = D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_VS;
	pss.m_vsByteCodeSubObject.m_vertexShaderByteCode.pShaderBytecode = shader->m_vsBlob->GetBufferPointer();
	pss.m_vsByteCodeSubObject.m_vertexShaderByteCode.BytecodeLength = shader->m_vsBlob->GetBufferSize();

	pss.m_psByteCodeSubObject = {};
	pss.m_psByteCodeSubObject.m_subObjectType = D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_PS;
	pss.m_psByteCodeSubObject.m_pixelShaderByteCode.pShaderBytecode = shader->m_psBlob->GetBufferPointer();
	pss.m_psByteCodeSubObject.m_pixelShaderByteCode.BytecodeLength = shader->m_psBlob->GetBufferSize();

	// DSV
	pss.m_dsvFormatSubObject = {};
	pss.m_dsvFormatSubObject.subObjectType	= D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_DEPTH_STENCIL_FORMAT;
	pss.m_dsvFormatSubObject.m_format		= usingDSV ? DXGI_FORMAT_D32_FLOAT : DXGI_FORMAT_UNKNOWN;

	// RTV
	pss.m_rtvFormatsSubObject = {};
	pss.m_rtvFormatsSubObject.subObjectType = D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_RENDER_TARGET_FORMATS;
	pss.m_rtvFormatsSubObject.m_formatArray = rtvFormats;
	
	// Rasterizer State
	pss.m_rasterizerStateSubObject = {};
	pss.m_rasterizerStateSubObject.m_subObjectType = D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_RASTERIZER;
	pss.m_rasterizerStateSubObject.m_rasterizerDesc = GetRasterizerDesc(rasterMode);

	// Blend State
	pss.m_blendStateSubObject = {};
	pss.m_blendStateSubObject.m_subObjectType = D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_BLEND;
	pss.m_blendStateSubObject.m_blendDesc = GetBlendDesc(blendMode);

	// Depth State
	pss.m_depthStencilStateSubObject = {};
	pss.m_depthStencilStateSubObject.m_subObjectType = D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_DEPTH_STENCIL;
	pss.m_depthStencilStateSubObject.m_desc = GetDepthStencilDesc(depthMode);
	
	return pss;
}

//------------------------------------------------------------------------------------------------------------------
BitmapFont* DX12Renderer::CreateOrGetBitmapFont(char const* bitmapFontFilePathWithNoExtension)
{

	BitmapFont* existingFont = GetBitmapFont(bitmapFontFilePathWithNoExtension);

	if(existingFont)
	{
		return existingFont;
	}

	BitmapFont* newFont = CreateBitmapFont(bitmapFontFilePathWithNoExtension);

	m_loadedFonts.push_back(newFont);

	return newFont;

}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
BitmapFont* DX12Renderer::CreateOrGetBitmapFontWithFontName(std::string const& fontName)
{
	std::string fontFilePath = "Data/Fonts/" + fontName;

	return CreateOrGetBitmapFont(fontFilePath.c_str());
}


//------------------------------------------------------------------------------------------------------------------
BitmapFont* DX12Renderer::CreateBitmapFont(char const* bitmapFontFilePathWithNoExtension)
{
	std::string textureFilePath = Stringf("%s.png", bitmapFontFilePathWithNoExtension);

	Texture* fontTexture = CreateOrGetTexture(textureFilePath);

	BitmapFont* font = new BitmapFont(bitmapFontFilePathWithNoExtension, *fontTexture);

	m_loadedFonts.push_back(font);

	return font;
}


//------------------------------------------------------------------------------------------------------------------
BitmapFont* DX12Renderer::GetBitmapFont(char const* bitmapFontFilePathWithNoExtension)
{
	for(size_t fonts = 0; fonts < m_loadedFonts.size(); fonts++)
	{
		if(m_loadedFonts[fonts]->GetImageFilePath() == bitmapFontFilePathWithNoExtension)
		{

			return m_loadedFonts[fonts];

		}
	}

	return nullptr;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void DX12Renderer::CreateFactory()
{
	HRESULT hr;
	UINT flags = 0;

#if defined(ENGINE_DEBUG_RENDER)
	flags |= DXGI_CREATE_FACTORY_DEBUG;
#endif

	hr = CreateDXGIFactory2(flags, IID_PPV_ARGS(&m_factory));
	if(FAILED(hr))
	{
		ERROR_AND_DIE("Create Factory Failed!")
	}

	std::string debugName = Stringf("DX12 Factory");

	m_factory->SetPrivateData(WKPDID_D3DDebugObjectName, static_cast<UINT>(strlen(debugName.c_str())), debugName.c_str());

	CheckTearingSupport();
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void DX12Renderer::CheckTearingSupport()
{
	HRESULT hr;

	BOOL allowTearing = false;

	hr = m_factory->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &allowTearing, sizeof(allowTearing));
	if(FAILED(hr))
	{
		m_config.m_tearingSupported = false;
	}
	else
	{
		m_config.m_tearingSupported = true;
	}

}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void DX12Renderer::FetchAdapter()
{
	// #Todo: Later, add fallback to manually choosing the GPU Adapter we need by doing a for loop (similar to the else branch in FristTriangle)

	HRESULT hr;

	hr = m_factory->EnumAdapterByGpuPreference(0, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&m_adapter));
	if(FAILED(hr))
	{
		ERROR_AND_DIE("Could not get an adapter through EnumAdapterByGPUPreference!")
	}


	std::string debugName = Stringf("DX12 Adapter");

	m_adapter->SetPrivateData(WKPDID_D3DDebugObjectName, static_cast<UINT>(strlen(debugName.c_str())), debugName.c_str());
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
Shader* DX12Renderer::GetShader(char const* shaderName)
{
	for(size_t shaderIndex = 0; shaderIndex < m_loadedShaders.size(); ++shaderIndex)
	{
		if(m_loadedShaders[shaderIndex]->GetName() == shaderName)
		{
			return m_loadedShaders[shaderIndex];
		}
	}
	return nullptr;
}

//------------------------------------------------------------------------------------------------------------------
Shader* DX12Renderer::CreateShaderFromFile(char const* fileName, InputLayoutType type)
{
	std::string shaderFilePath = Stringf("Data/Shaders/%s.hlsl", fileName);

	std::string shaderData;

	bool result = FileReadToString(shaderData, shaderFilePath);

	if(!result)
	{
		ERROR_RECOVERABLE(Stringf("Could not load shader from file: %s", shaderFilePath.c_str()))
	}

	Shader* newShader = CreateShader(fileName, shaderData.c_str(), type);

	return newShader;
}
//------------------------------------------------------------------------------------------------------------------
Shader* DX12Renderer::CreateShader(char const* shaderName, char const* shaderData, InputLayoutType type)
{

	UNUSED(type)

	ShaderConfig shaderConfig;
	shaderConfig.m_name = shaderName;

	Shader* shader = new Shader(shaderConfig);

	std::vector<uint8_t> vertexShaderByteCode;
	std::vector<uint8_t> pixelShaderByteCode;

	CreateShaderBlob(&shader->m_vsBlob, shaderData, shaderName, shader->m_config.m_vertexEntryPoint.c_str(), "vs_5_1");
	CreateShaderBlob(&shader->m_psBlob, shaderData, shaderName, shader->m_config.m_pixelEntryPoint.c_str(), "ps_5_1");

	m_loadedShaders.push_back(shader);

	return shader;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void DX12Renderer::CreateShaderBlob(ID3DBlob** shaderBlob, char const* shaderData, char const* shaderName, char const* entryPoint, char const* target)
{
	DWORD shaderFlags = D3DCOMPILE_OPTIMIZATION_LEVEL3;

#if defined(ENGINE_DEBUG_RENDER)
	shaderFlags = D3DCOMPILE_DEBUG;
	shaderFlags |= D3DCOMPILE_SKIP_OPTIMIZATION;
	shaderFlags |= D3DCOMPILE_WARNINGS_ARE_ERRORS;
#endif

	ID3DBlob* errorBlob = nullptr;

	HRESULT hr;
	hr = D3DCompile(shaderData, strlen(shaderData), shaderName, nullptr, nullptr, entryPoint, target, shaderFlags, 0, shaderBlob, &errorBlob);
	if(!SUCCEEDED(hr) || shaderBlob == nullptr)
	{
		if(errorBlob != NULL)
		{
			DebuggerPrintf(static_cast<char*>(errorBlob->GetBufferPointer()));
		}
		ERROR_AND_DIE(Stringf("Could not compile shader. EntryPoint: %s", entryPoint).c_str())
	}

	if(errorBlob != NULL)
	{
		errorBlob->Release();
	}
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void DX12Renderer::ResizeDepthBuffer(int width, int height)
{
//	Flush();

	D3D12_CLEAR_VALUE optimizedClearValue	= {};
	optimizedClearValue.Format				= DXGI_FORMAT_D32_FLOAT;
	optimizedClearValue.DepthStencil		= {1.f, 0};

	D3D12_HEAP_PROPERTIES heapProperties	= {};
	heapProperties.Type						= D3D12_HEAP_TYPE_DEFAULT;

	D3D12_RESOURCE_DESC textureDesc			= {};
	textureDesc.Dimension					= D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	textureDesc.Alignment					= 0;
	textureDesc.Width						= width;
	textureDesc.Height						= height;
	textureDesc.DepthOrArraySize			= 1;
	textureDesc.MipLevels					= 1;
	textureDesc.Format						= DXGI_FORMAT_D32_FLOAT;
	textureDesc.SampleDesc.Count			= 1;
	textureDesc.SampleDesc.Quality			= 0;
	textureDesc.Layout						= D3D12_TEXTURE_LAYOUT_UNKNOWN;
	textureDesc.Flags						= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

	ID3D12Resource* depthBuffer = nullptr;

	HRESULT hr;
	hr = m_device->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &textureDesc, D3D12_RESOURCE_STATE_COMMON, &optimizedClearValue, IID_PPV_ARGS(&depthBuffer));

	if(FAILED(hr))
	{
		ERROR_AND_DIE("Depth Buffer resource creation failed");
	}

	depthBuffer->SetName(L"Depth Buffer");

	ResourceStateTracker::AddGlobalResourceState(depthBuffer, D3D12_RESOURCE_STATE_COMMON);

	m_depthTexture = new Texture(m_device, depthBuffer, "Depth Texture");
	m_depthTexture->CreateViews();

	m_renderTarget->AttachTexture(AttachmentPoint::DEPTH_STENCIL, m_depthTexture);

// 	// Composite Depth Buffer
// 
// 	D3D12_HEAP_PROPERTIES compositeHeapProperties = {};
// 	compositeHeapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;
// 
// 	D3D12_RESOURCE_DESC compositeDepthDesc = {};
// 	compositeDepthDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
// 	compositeDepthDesc.Alignment = 0;
// 	compositeDepthDesc.Width = width;
// 	compositeDepthDesc.Height = height;
// 	compositeDepthDesc.DepthOrArraySize = 1;
// 	compositeDepthDesc.MipLevels = 1;
// 	compositeDepthDesc.Format = DXGI_FORMAT_R32_FLOAT;
// 	compositeDepthDesc.SampleDesc.Count = 1;
// 	compositeDepthDesc.SampleDesc.Quality = 0;
// 	compositeDepthDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
// 
// 	ID3D12Resource* compositeDepthBuffer = nullptr;
// 
// 	hr = m_device->CreateCommittedResource(&compositeHeapProperties, D3D12_HEAP_FLAG_NONE, &compositeDepthDesc, D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&compositeDepthBuffer));
// 
// 	if(FAILED(hr))
// 	{
// 		ERROR_AND_DIE("Depth Buffer resource creation failed");
// 	}
// 
// 	compositeDepthBuffer->SetName(L"Composite Depth Buffer");
// 
// 	ResourceStateTracker::AddGlobalResourceState(compositeDepthBuffer, D3D12_RESOURCE_STATE_COMMON);
// 
// 	m_compositeDepthBuffer = new Texture(m_device, compositeDepthBuffer, "Composite Depth Texture");
// 	m_compositeDepthBuffer->CreateViews();

}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void DX12Renderer::ReleaseStaleDescriptors(uint64_t finishedFrame)
{
	for(int i = 0; i < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES; ++i)
	{
		s_descriptorAllocators[i]->ReleaseStaleDescriptors(finishedFrame);
	}
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
Shader* DX12Renderer::CreateOrGetShader(char const* shaderName, InputLayoutType type, bool useDXC, ShaderType shaderType)
{
	Shader* shader = GetShader(shaderName);

	if(!shader)
	{
		if(useDXC)
		{
			shader = m_shaderCompiler->CreateShaderFromFile(shaderName, shaderType);
			m_loadedShaders.push_back(shader);
		}
		else
		{
			shader = CreateShaderFromFile(shaderName, type);
		}
	}

	return shader;
}


//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
Texture* DX12Renderer::CreateOrGetTexture(std::string const& textureFilePath)
{
	Texture* texture = m_copyCommandList->GetTexture(textureFilePath);

	if(texture)
	{
		return texture;
	}

	return m_copyCommandList->LoadTextureFromFile(textureFilePath);
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
Texture* DX12Renderer::CreateOrGetBindlessTexture(std::string const& textureFilePath)
{
	Texture* texture = m_copyCommandList->GetTexture(textureFilePath);

	if(texture)
	{
		return texture;
	}

	texture = m_copyCommandList->LoadTextureFromFile(textureFilePath);

	PersistentDescriptorAllocation srvAlloc = m_bindlessSRVHeap->AllocatePersistent();
	texture->m_bindlessIndex = srvAlloc.m_index;

	for(uint32_t i = 0; i < m_bindlessSRVHeap->m_numHeaps; ++i)
	{
		m_device->CreateShaderResourceView(texture->m_textureResource, nullptr, srvAlloc.m_handles[i]);
	}
	
	return texture;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
Texture* DX12Renderer::CreateOrGetTextureFromFilePath(std::string const& textureFilepath)
{
	Texture* texture = m_copyCommandList->GetTexture(textureFilepath);

	if(texture)
	{
		return texture;
	}

	return m_copyCommandList->LoadTextureFromFile(textureFilepath);
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void DX12Renderer::BindTexture(Texture* texture, RootParameters rootParam, uint32_t descriptorOffset)
{
	if(!texture)
	{
		texture = m_defaultTexture;
	}

	m_graphicsCommandList->SetTextureShaderResourceView(static_cast<int>(rootParam), descriptorOffset, texture, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void DX12Renderer::BindTextures(std::vector<Texture*> textures)
{
	uint32_t offset = 0;

	for(Texture* texture : textures)
	{
		UNUSED(texture)
// 		BindTexture(texture, offset);
		offset += 1;
	}
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void DX12Renderer::SetPipelineState(PipelineStateObject* incomingPSO)
{
	if(!incomingPSO)
	{
		m_graphicsCommandList->SetPipelineState(*m_defaultPSO);
		return;
	}
	
	m_graphicsCommandList->SetPipelineState(*incomingPSO);
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void DX12Renderer::SetModelConstants(Mat44 const& modelMatrix, Rgba8 const& color)
{
	ModelConstants modelConstant;
	modelConstant.cb_modelTransform = modelMatrix;
	modelConstant.cb_modelTint = color.GetAsVec4();
	
	m_graphicsCommandList->SetGraphicsDynamicConstantBuffer(static_cast<int>(RootParameters::MODEL_CB), modelConstant);
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void DX12Renderer::SetDebugConstants(DebugInfo const& debugInfo)
{
	g_rtDebugInfo = debugInfo;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void DX12Renderer::EnableJitter(bool jitter)
{
	if(static_cast<int>(jitter) != g_rtAppSettings.m_enableJitter)
	{
		g_rtAppSettings.m_accumCount = 0;
	}
	
	g_rtAppSettings.m_enableJitter = jitter ? 1 : 0;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void DX12Renderer::EnableFrameAccumulation(bool accum)
{
	if(static_cast<int>(accum) != g_rtAppSettings.m_enableFrameAccumulation)
	{
		g_rtAppSettings.m_accumCount = 0;
	}

	g_rtAppSettings.m_enableFrameAccumulation = accum ? 1 : 0;
}
//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void DX12Renderer::ResetFrameAccumulationCounter()
{
	g_rtAppSettings.m_accumCount = 0;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void DX12Renderer::SetMinLightRayBounces(unsigned int minBounces)
{
	if(g_rtAppSettings.m_minBounces == minBounces)
	{
		return;
	}

	g_rtAppSettings.m_minBounces = minBounces;
	ResetFrameAccumulationCounter();
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void DX12Renderer::ToggleIndirectLighting(bool enableIndirect)
{
	if(g_rtAppSettings.m_doIndirect == static_cast<unsigned int>(enableIndirect))
	{
		return;
	}
	g_rtAppSettings.m_doIndirect = static_cast<unsigned int>(enableIndirect);
	ResetFrameAccumulationCounter();
}
//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void DX12Renderer::ToggleDirectLighting(bool enableDirect)
{
	if(g_rtAppSettings.m_doDirect == static_cast<unsigned int>(enableDirect))
	{
		return;
	}
	g_rtAppSettings.m_doDirect = static_cast<unsigned int>(enableDirect);
	ResetFrameAccumulationCounter();
}
//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void DX12Renderer::SetSamplesPerPixel(int spp)
{
	if(spp == g_rtAppSettings.m_maxSamples)
	{
		return;
	}

	g_rtAppSettings.m_maxSamples = spp;
	ResetFrameAccumulationCounter();
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void DX12Renderer::SetNumDenoisePasses(int passes)
{
	if(passes == g_rtAppSettings.m_denoisePasses)
	{
		return;
	}

	g_rtAppSettings.m_denoisePasses = passes;
	ResetFrameAccumulationCounter();
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void DX12Renderer::SetDenoiseRadius(int radius)
{
	if(radius == g_rtAppSettings.m_denoiseRadius)
	{
		return;
	}

	g_rtAppSettings.m_denoiseRadius = radius;
	ResetFrameAccumulationCounter();

}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void DX12Renderer::SetDenoiseSigmaSpatial(float sigma)
{
	if(sigma == g_rtAppSettings.m_denoiseSigmaSpatial)
	{
		return;
	}

	g_rtAppSettings.m_denoiseSigmaSpatial = sigma;
	ResetFrameAccumulationCounter();
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void DX12Renderer::SetMaxFramesToAccumulate(int maxFrames)
{
	if(maxFrames == g_rtAppSettings.m_maxFramesToAccumulate)
	{
		return;
	}

	g_rtAppSettings.m_maxFramesToAccumulate = maxFrames;
	ResetFrameAccumulationCounter();

}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void DX12Renderer::ToggleTemporalReuse(bool newSetting)
{
	if(g_rtAppSettings.m_doTemporalReuse == static_cast<unsigned int>(newSetting))
	{
		return;
	}
	g_rtAppSettings.m_doTemporalReuse = static_cast<unsigned int>(newSetting);
	ResetFrameAccumulationCounter();
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void DX12Renderer::ToggleSpatialReuse(bool newSetting)
{
	if(g_rtAppSettings.m_doSpatialReuse == static_cast<unsigned int>(newSetting))
	{
		return;
	}
	g_rtAppSettings.m_doSpatialReuse = static_cast<unsigned int>(newSetting);
	ResetFrameAccumulationCounter();
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void DX12Renderer::ToggleDenoiser(bool newSetting)
{
	if(g_rtAppSettings.m_doDenoise == static_cast<unsigned int>(newSetting))
	{
		return;
	}
	g_rtAppSettings.m_doDenoise = static_cast<unsigned int>(newSetting);
	ResetFrameAccumulationCounter();
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
int DX12Renderer::GetAccumulatedFrameCount() const
{
	return g_rtAppSettings.m_accumCount;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void DX12Renderer::SetSceneConstants(unsigned int sceneMeshBufferIndex, unsigned int numStaticGeometry)
{
	SceneConstants sceneConsts;
	sceneConsts.m_sceneMeshBufferIndex = sceneMeshBufferIndex;
	sceneConsts.m_numStaticGeometry = numStaticGeometry;
	sceneConsts.m_padding = Vec2();

	if(g_rtSceneConsts.m_sceneMeshBufferIndex != sceneMeshBufferIndex || g_rtSceneConsts.m_numStaticGeometry != numStaticGeometry)
	{
		g_rtAppSettings.m_accumCount = 0;
	}

	g_rtSceneConsts = sceneConsts;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void DX12Renderer::SetLightConstants(Light const& directionalLight, std::vector<Light> const& allLights, float ambientIntensity, RootSignatureType rootSigType)
{
	LightConstants lightConstants;
	lightConstants.cb_directionalLight = directionalLight;
	lightConstants.cb_numLights = static_cast<int>(allLights.size());
	lightConstants.cb_ambientIntensity = ambientIntensity;

	for(int index = 0; index < static_cast<int>(allLights.size()); ++index)
	{
		lightConstants.cb_allLights[index] = allLights[index];
	}
	
	if(rootSigType == RootSignatureType::DEFAULT)
	{
		m_graphicsCommandList->SetGraphicsDynamicConstantBuffer(static_cast<uint32_t>(RootParameters::LIGHTS_CB), lightConstants);
	}
	else
	{
		// #ToDo: Comparision Operator for Light Consts
		g_rtLightConsts = lightConstants;
	}
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void DX12Renderer::DrawVertexBuffer(VertexBuffer* vbo)
{
	m_graphicsCommandList->SetVertexBuffer(0, *vbo);
	m_graphicsCommandList->SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	m_graphicsCommandList->Draw(vbo->GetSize());
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void DX12Renderer::DrawIndexedVertexBuffer(VertexBuffer* vbo, IndexBuffer* ibo, unsigned int indexCount)
{
	UNUSED(indexCount)
	m_graphicsCommandList->SetVertexBuffer(0, *vbo);
	m_graphicsCommandList->SetIndexBuffer(*ibo);
	m_graphicsCommandList->SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	m_graphicsCommandList->DrawIndexed(ibo->GetSize());
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void DX12Renderer::SetStructuredIndexedVertexBuffer(VertexBuffer* vbo, IndexBuffer* ibo)
{
	if(!vbo->m_device)
	{
		vbo->m_device = m_device;
	}

	if(!ibo->m_device)
	{
		ibo->m_device = m_device;
	}

// 	m_graphicsCommandList->SetStructuredVertexBuffer(static_cast<uint32_t>(GlobalRTRootSignatureParameters::VERTEX_BUFFER), *vbo);
// 	m_graphicsCommandList->SetStructuredIndexBuffer(static_cast<uint32_t>(GlobalRTRootSignatureParameters::INDEX_BUFFER),*ibo);
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void DX12Renderer::DrawVertexArray(std::vector<Vertex_PCU> verts)
{
	m_graphicsCommandList->SetDynamicVertexBuffer(0, verts);
	m_graphicsCommandList->SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	m_graphicsCommandList->Draw(static_cast<uint32_t>(verts.size()), 1, 0, 0);
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void DX12Renderer::DrawVertexArray(std::vector<Vertex_PCUTBN> verts)
{
	m_graphicsCommandList->SetDynamicVertexBuffer(0, verts);
	m_graphicsCommandList->SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	m_graphicsCommandList->Draw(static_cast<uint32_t>(verts.size()));
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void DX12Renderer::DrawIndexedVertexArray(std::vector<Vertex_PCU> verts, std::vector<uint32_t> indexes)
{
	m_graphicsCommandList->SetDynamicVertexBuffer(0, verts);
	m_graphicsCommandList->SetDynamicIndexBuffer(indexes);

	m_graphicsCommandList->SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	m_graphicsCommandList->DrawIndexed(static_cast<uint32_t>(indexes.size()));
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void DX12Renderer::DrawIndexedVertexArray(std::vector<Vertex_PCUTBN> verts, std::vector<uint32_t> indexes)
{
	m_graphicsCommandList->SetDynamicVertexBuffer(0, verts);
	m_graphicsCommandList->SetDynamicIndexBuffer(indexes);

	m_graphicsCommandList->SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	m_graphicsCommandList->DrawIndexed(static_cast<uint32_t>(indexes.size()));
}


#endif
