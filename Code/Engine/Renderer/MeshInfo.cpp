#include "Engine/Renderer/MeshInfo.hpp"

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
MeshInfo::MeshInfo(int vbIndex, int ibIndex, MaterialInfo const& materialInfo)
	: m_vbIndex(vbIndex)
	, m_ibIndex(ibIndex)
	, m_materialInfo(materialInfo)
{
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
MeshInfo::~MeshInfo()
{}
