#pragma once

#include "Engine/Math/Vec2.hpp"
#include "Engine/Math/Vec3.hpp"

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
struct MaterialInfo
{
	friend class CommandList;
	friend class DX12Renderer;

public:
	MaterialInfo() = default;
	MaterialInfo(int albedoIndex, int normalIndex, int aoIndex, int metalnessIndex, int albedoSampler = 0, int normalSampler = 0, int rmSampler = 0);
	~MaterialInfo();

	void SetAlbedoTextureIndex(int albedoIndex);
	void SetNormalTextureIndex(int normalIndex);
	void SetAOTextureIndex(int aoIndex);
	void SetRMTextureIndex(int metalnessIndex);
	void SetAlbedoSamplerIndex(int samplerIndex = 0);
	void SetNormalSamplerIndex(int samplerIndex = 0);
	void SetRMSamplerIndex(int samplerIndex = 0);
	bool operator==(MaterialInfo const& otherMaterial);

private:
	
	int		m_albedoIndex		= -1;
	int		m_normalIndex		= -1;
	int		m_aoIndex			= -1;
	int		m_rmIndex	= -1;
	int		m_albedoSamplerIndex	= 0;
	int		m_normalSamplerIndex	= 0;
	int		m_rmSamplerIndex		= 0;
	int		padding0;
};
