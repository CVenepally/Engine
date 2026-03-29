#pragma once


#include "Engine/Math/Mat44.hpp"

#include "Engine/Core/Rgba8.hpp"
#include "Engine/Core/Image.hpp"

#include "Engine/Renderer/RendererUtils.hpp"
#include "Engine/Renderer/DescriptorAllocator.hpp"

#include "Game/EngineBuildPreferences.hpp"

#include <vector>
#include <string>

#if defined(OPAQUE)
#undef OPAQUE
#endif

#if defined(USING_DX12)
//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
struct ID3D12Device10;
struct ID3D12CommandQueue;
struct IDXGISwapChain4;
struct IDXGIAdapter4;
struct IDXGIFactory6;
struct ID3D12Resource2;
struct ID3D12GraphicsCommandList7;
struct ID3D12CommandAllocator;
struct ID3D12DescriptorHeap;
struct ID3D12Fence1;
struct ID3D12RootSignature;
struct ID3D12PipelineState;
struct ID3D12Debug6;
struct ID3D12DebugDevice1;
struct ID3D10Blob;
struct ID3D12Object;
struct ID3D12StateObject;
struct D3D12_INPUT_LAYOUT_DESC;
struct D3D12_MEMCPY_DEST;
struct D3D12_SUBRESOURCE_DATA;
struct D3D12_PLACED_SUBRESOURCE_FOOTPRINT;
struct D3D12_VERTEX_BUFFER_VIEW;
struct D3D12_GPU_DESCRIPTOR_HANDLE;
struct PipelineStateStream;
struct BufferAllocationResult;
struct Light;
struct D3D12_RASTERIZER_DESC;
struct D3D12_INPUT_LAYOUT_DESC;	
struct D3D12_BLEND_DESC;	
struct D3D12_DEPTH_STENCIL_DESC;
struct Vertex_PCU;
struct Vertex_PCUTBN;
struct MaterialInfo;
struct DebugInfo;
struct AppSettings;

typedef ID3D10Blob ID3DBlob;
typedef std::vector<ID3D12Object*> TrackedObjects;

class Window;
class Camera;
class VertexBuffer;
class IndexBuffer;
class StructuredBuffer;
class Shader;
class RootSignature;
class PipelineStateObject;
class ConstantBuffer;
class Texture;
class RingBuffer;
class BitmapFont;
class CommandQueue;
class CommandList;
class RenderTarget;
class ShaderCompiler;
class ShaderTable;
class BottomLevelAS;
class TopLevelAS;
class BindlessDescriptorHeap;
class ImGuiHeap;
class GBuffer;

enum class CommandListType;

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
struct DX12RendererConfig
{
	Window*		m_window					= nullptr;
	bool		m_useWARP					= false;
	bool		m_vSync						= true;
	bool		m_tearingSupported			= false;
	bool		m_fullScreen				= false;
	bool		m_enableRayTracing			= false;
	bool		m_enableJitter				= true;
	bool		m_enableFrameAccumulation	= true;
	int			m_maxRayBounces				= 1;
	int			m_minRayBounces				= 0;
};

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
class DX12Renderer
{
public:
										DX12Renderer(DX12RendererConfig const& config);
	virtual								~DX12Renderer();

	void								BeginStartup();
	void								EndStartup();
	void								Shutdown();

	void								WaitForRendererToFinish();

	void								BeginFrame();
	void								EndFrame();

	uint64_t							UpdateFrameCount();

	void								ClearScreen(Rgba8 const& clearColor);

	void								BeginCamera(Camera const& camera);
	void								EndCamera(Camera const& camera);

	void								BeginRTCamera(Camera const& camera);
	void								EndRTCamera(Camera const& camera);

	void								BindVertexBuffer(VertexBuffer* vertexBuffer);
	void								BindIndexBuffer(IndexBuffer* indexBuffer);

	void								BindTexture(Texture* texture, RootParameters rootParam ,uint32_t descriptorOffset = 0);
	void								BindTextures(std::vector<Texture*> textures);

	void								SetPipelineState(PipelineStateObject* incomingPSO);
	void								SetModelConstants(Mat44 const& modelMatrix = Mat44(), Rgba8 const& color = Rgba8::WHITE);
	void								SetDebugConstants(DebugInfo const& debugInfo);
	void								EnableJitter(bool jitter);
	void								EnableFrameAccumulation(bool jitter);
	void								ResetFrameAccumulationCounter();

	void								SetMinLightRayBounces(unsigned int minBounces);
	void								ToggleIndirectLighting(bool enableIndirect);
	void								ToggleDirectLighting(bool enableDirect);
	void								SetSamplesPerPixel(int spp);

	void								SetNumDenoisePasses(int passes);
	void								SetDenoiseRadius(int radius);
	void								SetDenoiseSigmaSpatial(float sigma);
	//void								SetNumDenoisePasses(int passes);
	//void								SetNumDenoisePasses(int passes);
	
	void								SetMaxFramesToAccumulate(int maxFrames);
	void								ToggleTemporalReuse(bool newSetting);
	void								ToggleSpatialReuse(bool newSetting);
	void								ToggleDenoiser(bool newSetting);
	int									GetAccumulatedFrameCount() const;

	void								SetSceneConstants(unsigned int sceneMeshBufferIndex, unsigned int numStaticGeometry);
	void								SetLightConstants(Light const& directionalLight, std::vector<Light> const& allLights, float ambientIntensity, RootSignatureType rootSigType = RootSignatureType::DEFAULT);

	void								DrawVertexBuffer(VertexBuffer* vbo);
	void								DrawIndexedVertexBuffer(VertexBuffer* vbo, IndexBuffer* ibo, unsigned int indexCount);
	void								SetStructuredIndexedVertexBuffer(VertexBuffer* vbo, IndexBuffer* ibo);

	void								DrawVertexArray(std::vector<Vertex_PCU> verts);
	void								DrawVertexArray(std::vector<Vertex_PCUTBN> verts);

	void								DrawIndexedVertexArray(std::vector<Vertex_PCU> verts, std::vector<uint32_t> indexes);
	void								DrawIndexedVertexArray(std::vector<Vertex_PCUTBN> verts, std::vector<uint32_t> indexes);

	PipelineStateObject*				CreateOrGetPipelineStateObject(RootSignature* rootSignature, Shader* shader, 
																	   InputLayoutType inputLayout = InputLayoutType::VERTEX_PCU, 
																	   RasterizerMode rasterMode = RasterizerMode::SOLID_CULL_BACK, 
																	   BlendMode blendMode = BlendMode::OPAQUE, 
																	   DepthMode depthMode = DepthMode::READ_WRITE_LESS_EQUAL, bool usingDSV = true);
	
	Shader*								CreateOrGetShader(char const* shaderName, InputLayoutType type = InputLayoutType::VERTEX_PCU, bool useDXC = false, ShaderType shaderType = SHADER_VERTEX | SHADER_PIXEL);


	Texture*							CreateOrGetTexture(std::string const& textureFilePath);
	Texture*							CreateOrGetBindlessTexture(std::string const& textureFilePath);
	Texture*							CreateOrGetTextureFromFilePath(std::string const& textureFilepath);
	BitmapFont*							CreateOrGetBitmapFont(char const* bitmapFontFilePathWithNoExtension);
	BitmapFont*							CreateOrGetBitmapFontWithFontName(std::string const& fontName);

	VertexBuffer*						CreateVertexBuffer(unsigned int numElements, const void* vertexData, InputLayoutType vertexType = InputLayoutType::VERTEX_PCU, bool useBindless = false);
	IndexBuffer*						CreateIndexBuffer(unsigned int numElements, const void* indexData, bool useBindless = false);
	StructuredBuffer*					CreateStructuredBuffer(unsigned int numElements, unsigned int elementSize, const void* bufferData);
	
	void								ReleaseStaleDescriptors(uint64_t finishedFrame);
	static uint64_t						GetCurrentFrameNumber();
	static DescriptorAllocationResult	AllocateDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t numDecriptors = 1);

	void								DispatchRays(TopLevelAS* tlas);
	void								PerformDirectLighting(TopLevelAS* tlas);


	BottomLevelAS*						CreateBLAS(std::vector<VertexBuffer*> const& VBOs, std::vector<IndexBuffer*> const& IBOs);
	BottomLevelAS*						CreateBLAS(VertexBuffer* VBO, IndexBuffer* IBO);
	TopLevelAS*							CreateTLAS(std::vector<BottomLevelAS*> sceneBLASes);

	// Shader records (for hit shaders) are to be built for EVERY MESH.
	// #ToDo: Modify when opacity testing is added.
	void								BuildShaderBindingTables(int numRayGenRecordsPerRayType, int numMissShaderRecordsPerRayType, int numHitShaderRecordsPerRayType);

private:

	void								CreateFactory();
	void								CheckTearingSupport();
	void								FetchAdapter();

	void								InitializeDebugInterfaces();
	
	void								CreateDevice();
	void								CreateCommandQueues();
	void								CreateSwapChainAndGetBackBufferIndex();
	void								CreateRenderTargetTextures();
	void								UpdateRenderTargetViews();
	
	void								CreateDefaultRootSignature();

	// Raytracing-----------------------------------------------------------
	void								CreateRayTracingRootSignatures();
	void								CreateRaytracingGlobalRootSignature();
	void								CreateClosestHitRootSignature();

	void								CreateRTPSOs();
	void								CreateDefaultRayTracingPSO();
	void								CreateGBufferRayTracingPSO();
	
	void								BuildGBufferShaderTables(int numRayGenRecordsPerRayType, int numMissShaderRecordsPerRayType, int numHitShaderRecordsPerRayType);
	void								BuildDefaultShaderBindingTables(int numRayGenRecordsPerRayType, int numMissShaderRecordsPerRayType, int numHitShaderRecordsPerRayType);

	void								CreateRaytracingRenderTargets();
	void								CreateReservoirBuffers();
	void								ComputeGBuffers(TopLevelAS* tlas);
	void								PerformTemporalReuse();
	void								PerformSpatialReuse();
	void								Denoise();

	//----------------------------------------------------------------------

	void								CreateDefaultPSO();
	void								CreateSRVBindlessHeap();

	RenderTarget*						GetRenderTarget();

	RootSignature*						GetRootSignature(std::string name);

 	PipelineStateObject*				CreateComputePSO(RootSignature* rootSig, Shader* computeShader);
	PipelineStateObject*				CreatePipelineStateObject(RootSignature* rootSignature, RasterizerMode rasterMode, DepthMode depthMode, BlendMode blendMode, Shader* shader, InputLayoutType inputLayout = InputLayoutType::VERTEX_PCU, std::string debugName = "none", bool usingDSV = true);
	PipelineStateStream					CreatePipelineStateStream(RootSignature* rootSignature, RasterizerMode rasterMode, DepthMode depthMode, BlendMode blendMode, Shader* shader, InputLayoutType inputLayout = InputLayoutType::VERTEX_PCU, bool usingDSV = true);
	PipelineStateObject*				GetPipelineStateObject(std::string name);

	Shader*								CreateShaderFromFile(char const* fileName, InputLayoutType type = InputLayoutType::VERTEX_PCU);
	Shader*								CreateShader(char const* shaderName, char const* shaderData, InputLayoutType type = InputLayoutType::VERTEX_PCU);
	void								CreateShaderBlob(ID3DBlob** shaderBlob, char const* shaderData, char const* shaderName, char const* entryPoint, char const* target);
	Shader*								GetShader(char const* shaderName);

	BitmapFont*							CreateBitmapFont(char const* bitmapFontFilePathWithNoExtension);
	BitmapFont*							GetBitmapFont(char const* bitmapFontFilePathWithNoExtension);

	void								ResizeDepthBuffer(int width, int height);

	std::string							MakePSOName(RootSignature* rootSignature, Shader* shader, InputLayoutType inputLayout, RasterizerMode rasterMode, BlendMode blendMode, DepthMode depthMode, bool usingDSV = true);
	std::string							GetInputLayoutName(InputLayoutType inputLayout);
	std::string							GetRasterModeName(RasterizerMode rasterMode);
	std::string							GetBlendModeName(BlendMode blendMode);
	std::string							GetDepthModeName(DepthMode depthMode);

	D3D12_RASTERIZER_DESC				GetRasterizerDesc(RasterizerMode rasterMode);
	D3D12_INPUT_LAYOUT_DESC				GetInputLayoutDesc(InputLayoutType inputLayout);
	D3D12_BLEND_DESC					GetBlendDesc(BlendMode blendMode);
	D3D12_DEPTH_STENCIL_DESC			GetDepthStencilDesc(DepthMode depthMode);
	
	void								InitImGui();

	void								CompositeRenderTargetOutputs();

public:
	static uint64_t						s_frameCount;
	static DescriptorAllocator*			s_descriptorAllocators[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES];
	
	DX12RendererConfig					m_config;
	static constexpr int				m_numBuffers						= 3;
	unsigned int						m_currentBackBufferIndex			= 0;

	IDXGIFactory6*						m_factory							= nullptr;
	IDXGIAdapter4*						m_adapter							= nullptr;
	ID3D12Device10*						m_device							= nullptr;
	IDXGISwapChain4*					m_swapChain							= nullptr;
	Texture*							m_backBuffers[m_numBuffers]			= {};

	RenderTarget*						m_renderTarget						= nullptr;

	CommandQueue*						m_directCommandQueue				= nullptr;
	CommandQueue*						m_copyCommandQueue					= nullptr;
	CommandQueue*						m_computeCommandQueue				= nullptr;

	unsigned int						m_frameFenceValues[m_numBuffers]	= {};
	uint64_t							m_frameValues[m_numBuffers]			= {};

	CommandList*						m_graphicsCommandList				= nullptr;
	CommandList*						m_copyCommandList					= nullptr;
	CommandList*						m_computeCommandList				= nullptr;
	
	Texture*							m_depthTexture						= nullptr;

	PipelineStateObject*				m_defaultPSO						= nullptr;

	RootSignature*						m_defaultRootSignature				= nullptr;	

	Shader*								m_defaultShader						= nullptr;
	Shader*								m_compositeShader					= nullptr;

	Texture*							m_defaultTexture					= nullptr;
	Texture*							m_compositeDepthBuffer				= nullptr;

	PipelineStateObject*				m_currentPSO						= nullptr;

	PipelineStateObject*				m_psoToSet							= nullptr;

	BindlessDescriptorHeap*				m_bindlessSRVHeap					= nullptr;

	// Caches
	std::vector<PipelineStateObject*>	m_loadedPSOs;
	std::vector<Shader*>				m_loadedShaders;
	std::vector<BitmapFont*>			m_loadedFonts;

	// Loaded Structured Buffers
// 	std::vector<MaterialBuffer*>		m_loadedMaterials;

	ShaderCompiler*						m_shaderCompiler					= nullptr;

	// Raytracing-------------------------------------------------------------
	RootSignature*						m_globalRTRootSignature				= nullptr;
	RootSignature*						m_closestHitRootSig					= nullptr;

	ID3D12StateObject*					m_rtPSO								= nullptr;
	ID3D12StateObject*					m_gBufferPSO						= nullptr;

	PipelineStateObject*				m_temporalReusePSO					= nullptr;
	PipelineStateObject*				m_spatialReusePSO					= nullptr;
	PipelineStateObject*				m_denoiserPSO						= nullptr;

	ShaderTable*						m_rayGenShaderTable					= nullptr;
	ShaderTable*						m_hitGroupShaderTable				= nullptr;
	ShaderTable*						m_missShaderTable					= nullptr;

	ShaderTable*						m_rayGenShaderTable_gBuffer			= nullptr;
	ShaderTable*						m_hitGroupShaderTable_gBuffer		= nullptr;
	ShaderTable*						m_missShaderTable_gBuffer			= nullptr;

	GBuffer*							m_gBuffers							= nullptr;
	Texture*							m_noisyRenderTarget					= nullptr;

	StructuredBuffer*					m_temporalReservoirBuffer			= nullptr;
	StructuredBuffer*					m_prevReservoirBuffer				= nullptr;
	StructuredBuffer*					m_finalReservoirBuffer				= nullptr;

	Shader*								m_temporalReuseShader				= nullptr;
	Shader*								m_spatialReuseShader				= nullptr;
	Shader*								m_denoiseShader							= nullptr;

	// Debug------------------------------------------------------------------
	ID3D12Debug6*						m_debugInterface					= nullptr;
	void*								m_dxgiDebug							= nullptr;
	void*								m_dxgiDebugModule					= nullptr;
	void*								m_pixLib							= nullptr;
	static ImGuiHeap*					s_imGuiHeap;


};
#endif
