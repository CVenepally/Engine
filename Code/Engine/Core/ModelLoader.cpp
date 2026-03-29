#include "Engine/Core/ModelLoader.hpp"
#include "Engine/Core/StaticMesh.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Core/StringUtils.hpp"
#include "Engine/Core/FileUtils.hpp"
#include "Engine/Core/XMLUtils.hpp"
#include "Engine/Core/VertexUtils.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Renderer/DX12Renderer.hpp"
#include "Engine/Math/Vec3.hpp"

#if defined (USING_DX12)
ModelLoader::ModelLoader(DX12Renderer* renderer)
	:m_renderer(renderer)
{}
#else
//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
ModelLoader::ModelLoader(Renderer* renderer)
	:m_renderer(renderer)
{}
#endif
//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
ModelLoader::~ModelLoader()
{
	for(StaticMesh* mesh : m_loadedMeshes)
	{
		if(mesh)
		{
			delete mesh;
			mesh = nullptr;
		}
	}

	m_renderer = nullptr;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
StaticMesh* ModelLoader::LoadStaticMeshFromXML(std::string const& meshMetaFilePath)
{
	XmlDocument meshDef;

	XmlResult result = meshDef.LoadFile(meshMetaFilePath.c_str());
	GUARANTEE_OR_DIE(result == tinyxml2::XML_SUCCESS, Stringf("Could not open %s.xml", meshMetaFilePath.c_str()).c_str());

	XmlElement* meshInfoElement = meshDef.RootElement();
	GUARANTEE_OR_DIE(meshInfoElement, "Failed to access StaticMeshInfo root element");

	std::string meshFilePath = ParseXmlAttribute(*meshInfoElement, "meshFilePath", "default");

	if(meshFilePath == "default")
	{
		DebuggerPrintf("===================Mesh File Path Not Found===================");
		return nullptr;
	}

	float unitsPerMeter = ParseXmlAttribute(*meshInfoElement, "unitsPerMeter", 1.f);
	float scale			= 1.f / unitsPerMeter;

	std::string iBasis = ParseXmlAttribute(*meshInfoElement, "x", "forward");
	std::string jBasis = ParseXmlAttribute(*meshInfoElement, "y", "left");
	std::string kBasis = ParseXmlAttribute(*meshInfoElement, "z", "up");

	Vec3 iBasisVec = Vec3::MakeFromWord(iBasis);
	Vec3 jBasisVec = Vec3::MakeFromWord(jBasis);
	Vec3 kBasisVec = Vec3::MakeFromWord(kBasis);

	Mat44 transform = Mat44();
	transform.SetIJK3D(iBasisVec, jBasisVec, kBasisVec);

	std::string fileExtension = GetFileExtension(meshFilePath);

	StaticMesh* mesh = nullptr;

	if(fileExtension == "obj")
	{
		mesh = LoadStaticMeshFromOBJ(meshFilePath, transform, scale);
	}

	if(mesh)
	{
		std::string diffuseTexture	= ParseXmlAttribute(*meshInfoElement, "diffuseTexture", "");
		std::string normalTexture	= ParseXmlAttribute(*meshInfoElement, "normalTexture", "");
		std::string sgeTexture		= ParseXmlAttribute(*meshInfoElement, "sgeTexture", "");
		std::string shader			= ParseXmlAttribute(*meshInfoElement, "shader", "Data/Shaders/Default");

		mesh->m_albedoTexture = m_renderer->CreateOrGetTextureFromFilePath(diffuseTexture.c_str());
		mesh->m_normalTexture = m_renderer->CreateOrGetTextureFromFilePath(normalTexture.c_str());
		mesh->m_sgeTexture = m_renderer->CreateOrGetTextureFromFilePath(sgeTexture.c_str());
		mesh->m_shader = m_renderer->CreateOrGetShader(shader.c_str(), InputLayoutType::VERTEX_PCUTBN);
	}

	return mesh;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
StaticMesh* ModelLoader::LoadStaticMeshFromOBJ(std::string const& meshFilePath, Mat44 const& transform, float scale)
{
	DebuggerPrintf(Stringf("-------------------------Trying to open OBJ File from %s-------------------------\n", meshFilePath.c_str()).c_str());

	std::string objData;

	if(!FileReadToString(objData, meshFilePath))
	{
		DebuggerPrintf(Stringf("-------------------------OBJ file loading failed! File Path: %s-------------------------\n", meshFilePath.c_str()).c_str());
		return nullptr;
	}

	StaticMesh* mesh = new StaticMesh();

	// verts
	AddVertsForOBJMesh(objData, mesh->m_pcutbnVerts, mesh->m_indexes);

	TransformVertexArray3D(mesh->m_pcutbnVerts, transform, scale, true);

#if defined (USING_DX12)
	mesh->m_vbo = m_renderer->CreateVertexBuffer(static_cast<unsigned int>(mesh->m_pcutbnVerts.size()), mesh->m_pcutbnVerts.data(), InputLayoutType::VERTEX_PCUTBN);
	mesh->m_ibo = m_renderer->CreateIndexBuffer(static_cast<unsigned int>(mesh->m_indexes.size()), mesh->m_indexes.data());

#else
	unsigned int vboSize = sizeof(Vertex_PCUTBN) * static_cast<unsigned int>(mesh->m_pcutbnVerts.size());
	unsigned int iboSize = sizeof(unsigned int) * static_cast<unsigned int>(mesh->m_indexes.size());

	mesh->m_vbo = m_renderer->CreateVertexBuffer(sizeof(Vertex_PCUTBN), sizeof(Vertex_PCUTBN));
	mesh->m_ibo = m_renderer->CreateIndexBuffer(sizeof(unsigned int), sizeof(unsigned int));

	m_renderer->CopyCPUToGPU(mesh->m_pcutbnVerts.data(), vboSize, mesh->m_vbo);
	m_renderer->CopyCPUToGPU(mesh->m_indexes.data(), iboSize, mesh->m_ibo);
#endif

	m_loadedMeshes.push_back(mesh);

	return mesh;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
StaticMesh* ModelLoader::CreateStaticMeshFromGLTFPrimitive(tinygltf::Primitive const& primitive, tinygltf::Model& model, Mat44 const& transform, std::string const& name, std::string const& texturePath)
{
	for(StaticMesh* mesh : m_loadedMeshes)
	{
		if(mesh->m_meshName == name)
		{
			return mesh;
		}
	}

	unsigned int indicesAccessorIndex	= primitive.indices;
	unsigned int positionAccessorIndex	= primitive.attributes.find("POSITION")->second;
	unsigned int normalAccessorIndex	= primitive.attributes.find("NORMAL")->second;
	unsigned int materialIndex			= primitive.material;
	// unsigned int uvAccessorIndex		= primitive.attributes.find("TEXCOORD_0")->second;

	StaticMesh* mesh = new StaticMesh();
	mesh->m_meshName = name;
	std::vector<Vertex_PCUTBN>& verts = mesh->m_pcutbnVerts;
	std::vector<unsigned int>& inds = mesh->m_indexes;

	tinygltf::Accessor&		positionAccessor	= model.accessors[positionAccessorIndex];
	tinygltf::BufferView&	positionBufferView	= model.bufferViews[positionAccessor.bufferView];
	tinygltf::Buffer&		positionBuffer		= model.buffers[positionBufferView.buffer];
	unsigned char*			positionDataStart	= positionBuffer.data.data() + positionBufferView.byteOffset + positionAccessor.byteOffset;
	unsigned int 			numPositions		= static_cast<unsigned int>(positionAccessor.count);
	verts.resize(numPositions);

	tinygltf::Accessor&		normalAccessor		= model.accessors[normalAccessorIndex];
	tinygltf::BufferView&	normalBufferView	= model.bufferViews[normalAccessor.bufferView];
	tinygltf::Buffer&		normalBuffer		= model.buffers[normalBufferView.buffer];
	unsigned char*			normalDataStart		= normalBuffer.data.data() + normalBufferView.byteOffset + normalAccessor.byteOffset;

	// Material Loading
	tinygltf::Material&		material			= model.materials[materialIndex];

	LoadGLTFTextures(material, mesh, model, m_renderer, texturePath);

	std::vector<double> baseColorDouble = material.pbrMetallicRoughness.baseColorFactor;
	Rgba8 baseColor = Rgba8::WHITE;

	if(baseColorDouble.size() == 4)
	{
		baseColor.SetFromDouble(baseColorDouble.data());	
	}

	//UVs
	int			texCoordAccesorNumber = material.pbrMetallicRoughness.baseColorTexture.texCoord; // Relates to TEXCOORD_x under meshes->primitives->attributes
	std::string texCoordAttributeName = Stringf("TEXCOORD_%d", texCoordAccesorNumber);
	
	int		uvAccessorIndex = -1;
	auto	texCoordAttributeIter = primitive.attributes.find(texCoordAttributeName);
	if(texCoordAttributeIter != primitive.attributes.end())
	{
		uvAccessorIndex = primitive.attributes.find(texCoordAttributeName)->second;
	}

	for(unsigned int posIndex = 0; posIndex < numPositions; ++posIndex)
	{
		float* position = reinterpret_cast<float*>(positionDataStart + posIndex * positionAccessor.ByteStride(positionBufferView));
		verts[posIndex].m_position = Vec3(position[0], position[1], position[2]);

		verts[posIndex].m_color = baseColor;

		if(uvAccessorIndex != -1)
		{
			tinygltf::Accessor& uvAccessor = model.accessors[uvAccessorIndex];
			tinygltf::BufferView& uvBufferView = model.bufferViews[uvAccessor.bufferView];
			tinygltf::Buffer& uvBuffer = model.buffers[uvBufferView.buffer];
			
			unsigned char*	uvDataStart	= uvBuffer.data.data() + uvBufferView.byteOffset + uvAccessor.byteOffset;
			float*			uv			= reinterpret_cast<float*>(uvDataStart + posIndex * uvAccessor.ByteStride(uvBufferView));

			verts[posIndex].m_uvCoords = Vec2(uv[0], uv[1]);
		}

		float* normal = reinterpret_cast<float*>(normalDataStart + posIndex * normalAccessor.ByteStride(normalBufferView));
		verts[posIndex].m_normal = Vec3(normal[0], normal[1], normal[2]);
	}

	TransformVertexArray3D(verts, transform, 1.f, true);

	tinygltf::Accessor&		indicesAccessor		= model.accessors[indicesAccessorIndex];
	tinygltf::BufferView&	indicesBufferView	= model.bufferViews[indicesAccessor.bufferView];
	tinygltf::Buffer&		indicesBuffer		= model.buffers[indicesBufferView.buffer];
	unsigned char*			indicesDataStart	= indicesBuffer.data.data() + indicesBufferView.byteOffset + indicesAccessor.byteOffset;
	unsigned int 			numIndices			= static_cast<unsigned int>(indicesAccessor.count);
	inds.resize(numIndices);

	for(unsigned int indexIndex = 0; indexIndex < numIndices; ++indexIndex)
	{
		if(indicesAccessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT)
		{
			unsigned short* index = reinterpret_cast<unsigned short*>(indicesDataStart + indexIndex * indicesAccessor.ByteStride(indicesBufferView));
			inds[indexIndex] = static_cast<unsigned int>(*index);
		}
		else if(indicesAccessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT)
		{
			unsigned int* index = reinterpret_cast<unsigned int*>(indicesDataStart + indexIndex * indicesAccessor.ByteStride(indicesBufferView));
			inds[indexIndex] = *index;
		}
		else
		{
			ERROR_AND_DIE(Stringf("Unsupported index component type in glTF model: %d", indicesAccessor.componentType).c_str());
		}
	}
	
	CalculateTangentSpaceBasisVectors(verts, inds, true, false);

#if defined (USING_DX12)
	mesh->m_vbo = m_renderer->CreateVertexBuffer(static_cast<unsigned int>(mesh->m_pcutbnVerts.size()), mesh->m_pcutbnVerts.data(), InputLayoutType::VERTEX_PCUTBN, true);
	mesh->m_ibo = m_renderer->CreateIndexBuffer(static_cast<unsigned int>(mesh->m_indexes.size()), mesh->m_indexes.data(), true);

#else
	unsigned int vboSize = sizeof(Vertex_PCUTBN) * static_cast<unsigned int>(mesh->m_pcutbnVerts.size());
	unsigned int iboSize = sizeof(unsigned int) * static_cast<unsigned int>(mesh->m_indexes.size());

	mesh->m_vbo = m_renderer->CreateVertexBuffer(sizeof(Vertex_PCUTBN), sizeof(Vertex_PCUTBN));
	mesh->m_ibo = m_renderer->CreateIndexBuffer(sizeof(unsigned int), sizeof(unsigned int));

	m_renderer->CopyCPUToGPU(mesh->m_pcutbnVerts.data(), vboSize, mesh->m_vbo);
	m_renderer->CopyCPUToGPU(mesh->m_indexes.data(), iboSize, mesh->m_ibo);
#endif

	m_loadedMeshes.push_back(mesh);

	return mesh;
}
