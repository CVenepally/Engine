#pragma once
#include <string>
#include "Engine/Math/Mat44.hpp"
#include "Engine/Math/Vec4.hpp"
#include "Engine/Math/Vec3.hpp"
#include "Engine/Math/Vec2.hpp"

#define SizeOfInUint32(obj) ((sizeof(obj) - 1) / sizeof(UINT32) + 1)

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
enum class GlobalRTRootSignatureParameters
{
	OUPUT_VIEW,
	PREVIOUS_FRAME,
	FINAL_RESERVOIR,
	PREV_RESERVOIR,
	TEMPORAL_RESERVOIR,
	GBUFFER_POSITION,
	GBUFFER_NORMALS,
	GBUFFER_ALBEDO,
	GBUFFER_METALNESS,
	GBUFFER_ROUGHNESS,
	GBUFFER_VERTEX_COLOR,
	GBUFFER_SURFACE_TANGENT,
	GBUFFER_SURFACE_BITANGENT,
	GBUFFER_SURFACE_NORMAL,
	GBUFFER_VELOCITY,
	GBUFFER_PREV_NORMAL,
	GBUFFER_PREV_DEPTH,
	ACCELERATION_STRUCTURE,
	DEBUG_CB,
	APP_CB,
	CAMERA_CB,
	SCENE_CB,
	LIGHT_CB,
	SRV_BINDLESS,
	COUNT
};

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
struct RTCameraConstants
{
	Mat44	m_cameraToWorld		= Mat44();      
	Mat44   m_prevViewMatrix	= Mat44();
	Mat44	m_renderToCamera	= Mat44();
	Mat44	m_clipToRender		= Mat44();
	Vec4	m_cameraPosition	= Vec4();
	Mat44   m_currentFrameWorldToCamera = Mat44();
	Mat44   m_prevFrameWorldToCamera	= Mat44();
	Mat44   m_cameraToRender			= Mat44();
	Mat44   m_renderToClip				= Mat44();
	Vec2	m_screenDims				= Vec2();
	Vec2	padding						= Vec2();
};

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
struct SceneConstants
{
	unsigned int m_sceneMeshBufferIndex;
	unsigned int m_numStaticGeometry = 0;

	Vec2		 m_padding = Vec2();
};

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
struct BindlessIndexes
{
	int m_diffuseTextureIndex;
	int m_normalTextureIndex;
	int m_sgeTextureIndex;
	int m_roughnessTextureIndex;
};

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
struct DebugInfo
{
	int		m_debugView					= 0;
	int		m_diffuseModel				= 0;
	int		m_specularModel				= 0;
	int		m_lightSamplingMethod		= 1;
	
	int		m_envLighting				= 1;
	Vec3	m_envLightingColor			= Vec3();
};

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
struct AppSettings
{
	int				m_enableJitter					= 0;
	int				m_enableFrameAccumulation		= 0;
	unsigned int	m_frameCount					= 0;
	unsigned int	m_accumCount					= 0;
	
	int				m_maxSamples					= 32;
	unsigned int	m_minBounces					= 0;
	unsigned int	m_doDirect						= 1;
	unsigned int	m_doIndirect					= 1;

	unsigned int	m_doTemporalReuse				= 1;
	unsigned int	m_doSpatialReuse				= 1;
	int				m_maxFramesToAccumulate			= -1; // -1 is uncapped
	int				m_spatialReuseSamplingRadius	= 32;

	int				m_spatialReuseIterations			= 1;
	int				m_spatialReuseSamplesPerIteration	= 5;
	int				m_doDenoise							= 1;
	int				m_denoiseRadius						= 2;

	float			m_denoiseSigmaSpatial				= 2.f;
	float			m_denoiseSigmaPosition				= 0.5f;
	float			m_denoiseNormalPower				= 32.f;
	int				m_denoisePasses						= 3;
};

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
enum RayTypes : unsigned int
{
	RAY_PRIMARY,
	RAY_SHADOW,

	RAY_COUNT
};

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
struct RayPayload
{
	bool	m_didHit;
	Vec3	m_worldPosition;
	Vec3	m_pixelNormal;
	Vec3	m_surfaceNormal;
	Vec3	m_worldTangent;
	Vec3	m_worldBitangent;
	Vec3	m_worldRayDirection;


	Vec2	m_uv;

	Vec3	m_albedo;
	float	m_metalness;
	float	m_roughness;

	bool	m_hasNormalMap;
};

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
// CBVs
static const int k_rtDebugConstantRegister		= 0;
static const int k_rtDebugConstantSpace			= 0;

static const int k_rtAppSettingsRegister		= 1;
static const int k_rtAppSettingsSpace			= 0;

static const int k_rtCameraConstantsRegister	= 2;
static const int k_rtCameraConstantsSpace		= 0;

static const int k_rtSceneConstantRegister		= 3;
static const int k_rtSceneConstantSpace			= 0;

static const int k_rtLightConstantRegister		= 4;
static const int k_rtLightConstantSpace			= 0;

// SRVs
static const int k_tlasRegister					= 0;
static const int k_tlasSpace					= 0;

static const int k_rtVBRegister					= 0;
static const int k_rtVBSpace					= 1;

static const int k_rtIBRegister					= 0;
static const int k_rtIBSpace					= 2;

static const int k_bindlessSRVRegister			= 0;
static const int k_bindlessSRVSpace				= 3;

static const int k_reservoirBufferUAVRegister		= 2;
static const int k_finalReservoirBufferSpace		= 0;
static const int k_prevFrameReservoirBufferSpace	= 1;
static const int k_temporalReservoirBufferSpace		= 2;

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
static const std::wstring k_rayGenShaderName			= L"RayGenShader";

static const std::wstring k_closestHitShaderName		= L"ClosestHitShader";
static const std::wstring k_missShaderName				= L"MissShader";
static const std::wstring k_hitGroupName				= L"DefaultHitGroup";

static const std::wstring k_shadowMissShaderName		= L"ShadowMissShader";
static const std::wstring k_shadowClosestHitShaderName	= L"ShadowClosestHitShader";
static const std::wstring k_shadowHitGroupName			= L"ShadowHitGroup";
