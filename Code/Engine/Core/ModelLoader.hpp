#pragma once
#include "Engine/Math/Mat44.hpp"
#include "Game/EngineBuildPreferences.hpp"
#include "Engine/Core/StaticMeshUtils.hpp"

#include <vector>
#include <string>

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
class Renderer;
class DX12Renderer;
class StaticMesh;

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
class ModelLoader
{
public:

#if defined (USING_DX12)
	ModelLoader(DX12Renderer* renderer);
#else
	ModelLoader(Renderer* renderer);
#endif

	virtual ~ModelLoader();

	StaticMesh* LoadStaticMeshFromXML(std::string const& meshMetaDataFile);
	StaticMesh* LoadStaticMeshFromOBJ(std::string const& modelName, Mat44 const& transform = Mat44(), float scale = 1);
	
	StaticMesh*	CreateStaticMeshFromGLTFPrimitive(tinygltf::Primitive const& primitive, tinygltf::Model& model, Mat44 const& transform, std::string const& name, std::string const& texturesPath);

private:
	std::vector<StaticMesh*>	m_loadedMeshes;

#if defined (USING_DX12)
	DX12Renderer*				m_renderer;
#else
	Renderer*					m_renderer;
#endif

};