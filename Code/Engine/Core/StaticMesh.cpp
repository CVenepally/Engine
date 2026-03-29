#include "Engine/Core/StaticMesh.hpp"
#include "Engine/Core/StaticMeshUtils.hpp"
#include "Engine/Renderer/VertexBuffer.hpp"
#include "Engine/Renderer/IndexBuffer.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Renderer/Texture.hpp"
#include "Engine/Renderer/Shader.hpp"

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
StaticMesh::~StaticMesh()
{
	delete m_vbo;
	delete m_ibo;

	m_vbo = nullptr;
	m_ibo = nullptr;
}
