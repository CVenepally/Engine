#pragma once
#include "Game/EngineBuildPreferences.hpp"
#include <cstdint>
#include <functional>

#if defined(OPAQUE)
#undef OPAQUE
#endif

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
enum class InputLayoutType
{
	VERTEX_P,
	VERTEX_PCU,
	VERTEX_PCUTBN,

	COUNT
};

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
enum class BlendMode
{
	ALPHA,
	ADDITIVE,
	OPAQUE,

	COUNT
};

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
enum class SamplerMode
{
	POINT_CLAMP,
	BILINEAR_WRAP,
	BILINEAR_COMPARISION_BORDER,

	// New entries for glTF coverage
	POINT_WRAP,
	POINT_MIRROR,
	BILINEAR_CLAMP,
	BILINEAR_MIRROR,
	TRILINEAR_WRAP,
	TRILINEAR_CLAMP,
	TRILINEAR_MIRROR,

	COUNT
};

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
SamplerMode ConvertGLTFSamplerToMode(int magFilter, int minFilter, int wrapS, int wrapT);

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
enum class RasterizerMode
{
	SOLID_CULL_NONE,
	SOLID_CULL_BACK,
	WIREFRAME_CULL_NONE,
	WIREFRAME_CULL_BACK,

	COUNT
};

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
enum class DepthMode
{
	DISABLED,
	READ_ONLY_ALWAYS,
	READ_ONLY_LESS_EQUAL,
	READ_WRITE_LESS_EQUAL,

	COUNT
};

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
enum class TextureType
{
	NONE = -1,
	DIFFUSE,
	NORMAL,
	SGE,
	ALBEDO,
	METAL,
	ROUGH,
	AO
};

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
enum ShaderType
{
	SHADER_NONE			= 0,
	SHADER_VERTEX		= 1 << 0,
	SHADER_PIXEL		= 1 << 1,
	SHADER_COMPUTE		= 1 << 2,
	SHADER_RAYTRACE		= 1 << 3,
	SHADER_ALL			= SHADER_VERTEX | SHADER_PIXEL | SHADER_COMPUTE | SHADER_RAYTRACE,
};

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
enum class RootParameters
{
	DEBUG_CB,
	CAMERA_CB,
	MODEL_CB,
	LIGHTS_CB,
	DIFFUSE_TEXTURE_DT,
	NORMAL_TEXTURE_DT,
	SGE_TEXTURE_DT,
	//SAMPLER_DT,

	COUNT
};

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
enum class RootSignatureType
{
	DEFAULT,
	RAY_TRACED,

	COUNT
};


//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
enum class ClosestHitRootSignatureParameters
{
	LIGHT_CB,
	COUNT
};

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
enum GBufferType
{
	GBUFFER_POSITION,
	GBUFFER_NORMAL,
	GBUFFER_BASE_COLOR,
	GBUFFER_METALNESS,
	GBUFFER_ROUGHNESS,
	GBUFFER_VERTEX_COLOR,
	GBUFFER_SURFACE_TANGENT,
	GBUFFER_SURFACE_BITANGENT,
	GBUFFER_SURFACE_NORMAL,
	GBUFFER_VELOCITY,
	GBUFFER_PREV_NORMAL,
	GBUFFER_PREV_DEPTH,
	
	GBUFFER_COUNT
};

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
inline ShaderType operator|(ShaderType a, ShaderType b)
{
	return static_cast<ShaderType>(static_cast<int>(a) | static_cast<int>(b));
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
inline ShaderType operator&(ShaderType a, ShaderType b)
{
	return static_cast<ShaderType>(static_cast<int>(a) & static_cast<int>(b));
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
inline ShaderType& operator|=(ShaderType& a, ShaderType b)
{
	a = a | b;
	return a;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
inline ShaderType& operator&=(ShaderType& a, ShaderType b)
{
	a = a & b;
	return a;
}
//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
#if defined (USING_DX12)

#include <d3d12.h>

namespace std
{
	template <typename T>
	inline void hash_combine(std::size_t& seed, const T& v)
	{
		std::hash<T> hasher;
		seed ^= hasher(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
	}

	template<>
	struct hash<D3D12_SHADER_RESOURCE_VIEW_DESC>
	{
		std::size_t operator()(const D3D12_SHADER_RESOURCE_VIEW_DESC& srvDesc) const noexcept
		{
			std::size_t seed = 0;

			hash_combine(seed, srvDesc.Format);
			hash_combine(seed, srvDesc.ViewDimension);
			hash_combine(seed, srvDesc.Shader4ComponentMapping);

			switch(srvDesc.ViewDimension)
			{
			case D3D12_SRV_DIMENSION_BUFFER:
				hash_combine(seed, srvDesc.Buffer.FirstElement);
				hash_combine(seed, srvDesc.Buffer.NumElements);
				hash_combine(seed, srvDesc.Buffer.StructureByteStride);
				hash_combine(seed, srvDesc.Buffer.Flags);
				break;
			case D3D12_SRV_DIMENSION_TEXTURE1D:
				hash_combine(seed, srvDesc.Texture1D.MostDetailedMip);
				hash_combine(seed, srvDesc.Texture1D.MipLevels);
				hash_combine(seed, srvDesc.Texture1D.ResourceMinLODClamp);
				break;
			case D3D12_SRV_DIMENSION_TEXTURE1DARRAY:
				hash_combine(seed, srvDesc.Texture1DArray.MostDetailedMip);
				hash_combine(seed, srvDesc.Texture1DArray.MipLevels);
				hash_combine(seed, srvDesc.Texture1DArray.FirstArraySlice);
				hash_combine(seed, srvDesc.Texture1DArray.ArraySize);
				hash_combine(seed, srvDesc.Texture1DArray.ResourceMinLODClamp);
				break;
			case D3D12_SRV_DIMENSION_TEXTURE2D:
				hash_combine(seed, srvDesc.Texture2D.MostDetailedMip);
				hash_combine(seed, srvDesc.Texture2D.MipLevels);
				hash_combine(seed, srvDesc.Texture2D.PlaneSlice);
				hash_combine(seed, srvDesc.Texture2D.ResourceMinLODClamp);
				break;
			case D3D12_SRV_DIMENSION_TEXTURE2DARRAY:
				hash_combine(seed, srvDesc.Texture2DArray.MostDetailedMip);
				hash_combine(seed, srvDesc.Texture2DArray.MipLevels);
				hash_combine(seed, srvDesc.Texture2DArray.FirstArraySlice);
				hash_combine(seed, srvDesc.Texture2DArray.ArraySize);
				hash_combine(seed, srvDesc.Texture2DArray.PlaneSlice);
				hash_combine(seed, srvDesc.Texture2DArray.ResourceMinLODClamp);
				break;
			case D3D12_SRV_DIMENSION_TEXTURE2DMS:
				//                hash_combine(seed, srvDesc.Texture2DMS.UnusedField_NothingToDefine);
				break;
			case D3D12_SRV_DIMENSION_TEXTURE2DMSARRAY:
				hash_combine(seed, srvDesc.Texture2DMSArray.FirstArraySlice);
				hash_combine(seed, srvDesc.Texture2DMSArray.ArraySize);
				break;
			case D3D12_SRV_DIMENSION_TEXTURE3D:
				hash_combine(seed, srvDesc.Texture3D.MostDetailedMip);
				hash_combine(seed, srvDesc.Texture3D.MipLevels);
				hash_combine(seed, srvDesc.Texture3D.ResourceMinLODClamp);
				break;
			case D3D12_SRV_DIMENSION_TEXTURECUBE:
				hash_combine(seed, srvDesc.TextureCube.MostDetailedMip);
				hash_combine(seed, srvDesc.TextureCube.MipLevels);
				hash_combine(seed, srvDesc.TextureCube.ResourceMinLODClamp);
				break;
			case D3D12_SRV_DIMENSION_TEXTURECUBEARRAY:
				hash_combine(seed, srvDesc.TextureCubeArray.MostDetailedMip);
				hash_combine(seed, srvDesc.TextureCubeArray.MipLevels);
				hash_combine(seed, srvDesc.TextureCubeArray.First2DArrayFace);
				hash_combine(seed, srvDesc.TextureCubeArray.NumCubes);
				hash_combine(seed, srvDesc.TextureCubeArray.ResourceMinLODClamp);
				break;

			case D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE:
			    hash_combine(seed, srvDesc.RaytracingAccelerationStructure.Location);
			    break;
			}

			return seed;
		}
	};

	template<>
	struct hash<D3D12_UNORDERED_ACCESS_VIEW_DESC>
	{
		std::size_t operator()(const D3D12_UNORDERED_ACCESS_VIEW_DESC& uavDesc) const noexcept
		{
			std::size_t seed = 0;

			hash_combine(seed, uavDesc.Format);
			hash_combine(seed, uavDesc.ViewDimension);

			switch(uavDesc.ViewDimension)
			{
			case D3D12_UAV_DIMENSION_BUFFER:
				hash_combine(seed, uavDesc.Buffer.FirstElement);
				hash_combine(seed, uavDesc.Buffer.NumElements);
				hash_combine(seed, uavDesc.Buffer.StructureByteStride);
				hash_combine(seed, uavDesc.Buffer.CounterOffsetInBytes);
				hash_combine(seed, uavDesc.Buffer.Flags);
				break;
			case D3D12_UAV_DIMENSION_TEXTURE1D:
				hash_combine(seed, uavDesc.Texture1D.MipSlice);
				break;
			case D3D12_UAV_DIMENSION_TEXTURE1DARRAY:
				hash_combine(seed, uavDesc.Texture1DArray.MipSlice);
				hash_combine(seed, uavDesc.Texture1DArray.FirstArraySlice);
				hash_combine(seed, uavDesc.Texture1DArray.ArraySize);
				break;
			case D3D12_UAV_DIMENSION_TEXTURE2D:
				hash_combine(seed, uavDesc.Texture2D.MipSlice);
				hash_combine(seed, uavDesc.Texture2D.PlaneSlice);
				break;
			case D3D12_UAV_DIMENSION_TEXTURE2DARRAY:
				hash_combine(seed, uavDesc.Texture2DArray.MipSlice);
				hash_combine(seed, uavDesc.Texture2DArray.FirstArraySlice);
				hash_combine(seed, uavDesc.Texture2DArray.ArraySize);
				hash_combine(seed, uavDesc.Texture2DArray.PlaneSlice);
				break;
			case D3D12_UAV_DIMENSION_TEXTURE3D:
				hash_combine(seed, uavDesc.Texture3D.MipSlice);
				hash_combine(seed, uavDesc.Texture3D.FirstWSlice);
				hash_combine(seed, uavDesc.Texture3D.WSize);
				break;
			}

			return seed;
		}
	};
}

#endif