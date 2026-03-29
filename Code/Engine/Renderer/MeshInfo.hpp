#pragma once
#include "Engine/Renderer/MaterialInfo.hpp"

struct MeshInfo
{
public:
	MeshInfo() = default;
	MeshInfo(int vbIndex, int ibIndex, MaterialInfo const& materialInfo);
	~MeshInfo();

public:
	int m_vbIndex			= -1;
	int m_ibIndex			= -1;
	int m_isStatic			= 1; // 1 is true 0 is false
	int padding				= 0;

	MaterialInfo m_materialInfo = MaterialInfo();
	
};