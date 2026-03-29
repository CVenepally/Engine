#pragma once
#include <string>
#include <vector>
#include "Engine/Core/Vertex_PCUTBN.hpp"
#include "Engine/Renderer/RendererUtils.hpp"

class Texture;
class Shader;
class VertexBuffer;
class IndexBuffer;
class Renderer;
class ModelLoader;

struct Vertex_PCUTBN;

class StaticMesh
{
public:
	friend class ModelLoader;
	~StaticMesh();

	StaticMesh() = default;

public:

	std::string		m_meshName;
	
	Texture*		m_albedoTexture = nullptr;
	Texture*		m_normalTexture = nullptr;
	Texture*		m_rmTexture = nullptr;
	Texture*		m_aoTexture = nullptr;

	Texture*		m_sgeTexture = nullptr;

	Shader*			m_shader = nullptr;
	VertexBuffer*	m_vbo = nullptr;
	IndexBuffer*	m_ibo = nullptr;

	SamplerMode		m_diffuseSampler	= SamplerMode::POINT_CLAMP;
	SamplerMode		m_normalSampler		= SamplerMode::POINT_CLAMP;
	SamplerMode		m_rmSampler			= SamplerMode::POINT_CLAMP;
	SamplerMode		m_aoSampler			= SamplerMode::POINT_CLAMP;
	SamplerMode		m_sgeSampler		= SamplerMode::POINT_CLAMP;

 	// temp
 	std::vector<Vertex_PCUTBN>	m_pcutbnVerts;
 	std::vector<unsigned int>	m_indexes;
};