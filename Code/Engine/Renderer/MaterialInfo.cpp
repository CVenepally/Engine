#include "Engine/Renderer/MaterialInfo.hpp"

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
MaterialInfo::MaterialInfo(int albedoIndex, int normalIndex, int aoIndex, int rmIndex, int albedoSampler, int normalSampler, int rmSampler)
	: m_albedoIndex(albedoIndex)
	, m_normalIndex(normalIndex)
	, m_aoIndex(aoIndex)
	, m_rmIndex(rmIndex)
// 	, m_roughnessIndex(roughnessIndex)
	, m_albedoSamplerIndex(albedoSampler)
	, m_normalSamplerIndex(normalSampler)
	, m_rmSamplerIndex(rmSampler)

{}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
MaterialInfo::~MaterialInfo()
{}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void MaterialInfo::SetAlbedoTextureIndex(int albedoIndex)
{
	m_albedoIndex = albedoIndex;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void MaterialInfo::SetNormalTextureIndex(int normalIndex)
{
	m_normalIndex = normalIndex;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void MaterialInfo::SetAOTextureIndex(int aoIndex)
{
	m_aoIndex = aoIndex;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void MaterialInfo::SetRMTextureIndex(int metalnessIndex)
{
	m_rmIndex = metalnessIndex;
}

// -------------------------------------------------------------------------------------------------------------------------------------------------------------------
// void MaterialInfo::SetRoughnessTextureIndex(int roughnessIndex)
// {
// 	m_roughnessIndex = roughnessIndex;
// }

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void MaterialInfo::SetAlbedoSamplerIndex(int samplerIndex)
{
	m_albedoSamplerIndex = samplerIndex;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void MaterialInfo::SetNormalSamplerIndex(int samplerIndex)
{
	m_normalSamplerIndex = samplerIndex;

}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void MaterialInfo::SetRMSamplerIndex(int samplerIndex)
{
	m_rmSamplerIndex = samplerIndex;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
bool MaterialInfo::operator==(MaterialInfo const& otherMaterial)
{
	return (m_albedoIndex == otherMaterial.m_albedoIndex) &&
		(m_aoIndex == otherMaterial.m_aoIndex)&&
		(m_normalIndex == otherMaterial.m_normalIndex) &&
		(m_rmIndex == otherMaterial.m_rmIndex) &&
		(m_albedoSamplerIndex == otherMaterial.m_albedoSamplerIndex) &&
		(m_normalSamplerIndex == otherMaterial.m_normalSamplerIndex) &&
		(m_rmSamplerIndex == otherMaterial.m_rmSamplerIndex);
}
