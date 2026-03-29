#define TINYGLTF_IMPLEMENTATION
#include "Engine/Core/StaticMeshUtils.hpp"
#include "Engine/Core/StaticMesh.hpp"
#include "Engine/Core/StringUtils.hpp"
#include "Engine/Core/XMLUtils.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Core/Vertex_PCUTBN.hpp"

#include "Engine/Math/Vec3.hpp"
#include "Engine/Math/IntVec3.hpp"
#include "Engine/Math/MathUtils.hpp"

#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Renderer/DX12Renderer.hpp"
#include "Engine/Renderer/RendererUtils.hpp"

#include <vector>

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
bool LoadStaticMeshFileFromXML(std::string const& meshFilePath)
{
	DebuggerPrintf("-------------------------Trying to open XML File-------------------------\n");

	std::string xmlFilePath = meshFilePath + ".xml";

	XmlDocument meshFileMetaData;
	XmlResult	fileLoadResult = meshFileMetaData.LoadFile(meshFilePath.c_str());

	if(fileLoadResult != tinyxml2::XML_SUCCESS)
	{
		DebuggerPrintf("-------------------------Failed to open XML Mesh MetadataFile. File may not exist-------------------------\n");
		return false;
	}

	return false;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void CalculateTangentSpaceBasisVectors(std::vector<Vertex_PCUTBN>& vertexes, std::vector<unsigned int>& indexes, bool computeTangents, bool computeNormals)
{
	std::vector<Vec3> triangles;
	triangles.reserve(static_cast<size_t>(indexes.size() * 0.33f));

	for(int index = 0; index < static_cast<int>(indexes.size()); index = index + 3)
	{
		Vec3 triangle;
		triangle.x = static_cast<float>(indexes[index]);
		triangle.y = static_cast<float>(indexes[index + 1]);
		triangle.z = static_cast<float>(indexes[index + 2]);

		triangles.push_back(triangle);
	}

	for(Vec3 const& triangle : triangles)
	{
		int triIndexOne		= static_cast<int>(triangle.x);
		int triIndexTwo		= static_cast<int>(triangle.y);
		int triIndexThree	= static_cast<int>(triangle.z);

		Vertex_PCUTBN& triVertOne	= vertexes[triIndexOne];
		Vertex_PCUTBN& triVertTwo	= vertexes[triIndexTwo];
		Vertex_PCUTBN& triVertThree	= vertexes[triIndexThree];

		Vec3 vertOneTwoVector = triVertTwo.m_position - triVertOne.m_position;
		Vec3 vertOneThreeVector = triVertThree.m_position - triVertOne.m_position;

		if(computeNormals)
		{
			Vec3 normalVector = CrossProduct3D(vertOneTwoVector, vertOneThreeVector).GetNormalized();

			triVertOne.m_normal		+= normalVector;
			triVertTwo.m_normal		+= normalVector;
			triVertThree.m_normal	+= normalVector;
		}

		if(computeTangents)
		{
			float deltaUOne = triVertTwo.m_uvCoords.x - triVertOne.m_uvCoords.x;
			float deltaUTwo	= triVertThree.m_uvCoords.x - triVertOne.m_uvCoords.x;

			float deltaVOne = triVertTwo.m_uvCoords.y - triVertOne.m_uvCoords.y;
			float deltaVTwo = triVertThree.m_uvCoords.y - triVertOne.m_uvCoords.y;

			float r = 1 / ((deltaUOne * deltaVTwo) - (deltaUTwo * deltaVOne));

			Vec3 tangent	= r * (deltaVTwo * vertOneTwoVector - deltaVOne * vertOneThreeVector).GetNormalized();
			Vec3 bitangent	= r * (deltaUOne * vertOneTwoVector - deltaUTwo * vertOneThreeVector).GetNormalized();

			triVertOne.m_tangent	+= tangent;
			triVertTwo.m_tangent	+= tangent;
			triVertThree.m_tangent	+= tangent;

			triVertOne.m_bitangent		+= bitangent;
			triVertTwo.m_bitangent		+= bitangent;
			triVertThree.m_bitangent	+= bitangent;
		}
	}

	for(Vertex_PCUTBN& vert : vertexes)
	{
		vert.m_normal.Normalize();

		Vec3 normalPartsInTangent = GetProjectedOnto3D(vert.m_tangent, vert.m_normal);
		vert.m_tangent -= normalPartsInTangent;
		vert.m_tangent.Normalize();
		
		vert.m_bitangent = CrossProduct3D(vert.m_normal, vert.m_tangent);
	}
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
#if defined (USING_DX12)
void LoadGLTFTextures(tinygltf::Material const& material, StaticMesh* mesh, tinygltf::Model& model, DX12Renderer* renderer, std::string const& texturePath)
{
	// Index should be 0 or above. If not, there is no texture.
	int baseColorTextureIndex			= material.pbrMetallicRoughness.baseColorTexture.index;
	int normalTextureIndex				= material.normalTexture.index;
	int metallicRoughnessTextureIndex	= material.pbrMetallicRoughness.metallicRoughnessTexture.index;

	if(baseColorTextureIndex >= 0)
	{
		tinygltf::Texture& baseColorTexture = model.textures[baseColorTextureIndex];
		int baseColorTextureImageIndex = baseColorTexture.source;
		tinygltf::Image& albedo = model.images[baseColorTextureImageIndex];
		Texture* albedoTexture = renderer->CreateOrGetBindlessTexture((texturePath + albedo.uri).c_str());
		mesh->m_albedoTexture = albedoTexture;

		// Sampler
		int samplerIndex = baseColorTexture.sampler;
		tinygltf::Sampler& sampler = model.samplers[samplerIndex];
		int minFilter = sampler.minFilter;
		int magFilter = sampler.magFilter;
		int wrapS = sampler.wrapS;
		int wrapT = sampler.wrapT;
		mesh->m_diffuseSampler = ConvertGLTFSamplerToMode(magFilter, minFilter, wrapS, wrapT);
	}

	if(normalTextureIndex >= 0)
	{
		tinygltf::Texture& normalTextureTG = model.textures[normalTextureIndex];
		int normalTextureImageIndex = normalTextureTG.source;
		tinygltf::Image& normalTextureImage = model.images[normalTextureImageIndex];

		Texture* normalTexture = renderer->CreateOrGetBindlessTexture((texturePath + normalTextureImage.uri).c_str());
		mesh->m_normalTexture = normalTexture;

		// Sampler
		int samplerIndex = normalTextureTG.sampler;
		tinygltf::Sampler& sampler = model.samplers[samplerIndex];
		int minFilter = sampler.minFilter;
		int magFilter = sampler.magFilter;
		int wrapS = sampler.wrapS;
		int wrapT = sampler.wrapT;
		mesh->m_normalSampler = ConvertGLTFSamplerToMode(magFilter, minFilter, wrapS, wrapT);
	}

	if(metallicRoughnessTextureIndex >= 0)
	{
		tinygltf::Texture& rmTextureTG = model.textures[metallicRoughnessTextureIndex];
		int rmTextureImageIndex = rmTextureTG.source;
		tinygltf::Image& rmTextureImage = model.images[rmTextureImageIndex];

		Texture* rmTexture = renderer->CreateOrGetBindlessTexture((texturePath + rmTextureImage.uri).c_str());
		mesh->m_rmTexture = rmTexture;

		// Sampler
		int samplerIndex			= rmTextureTG.sampler;
		tinygltf::Sampler& sampler	= model.samplers[samplerIndex];
		int minFilter				= sampler.minFilter;
		int magFilter				= sampler.magFilter;
		int wrapS					= sampler.wrapS;
		int wrapT					= sampler.wrapT;
		mesh->m_rmSampler			= ConvertGLTFSamplerToMode(magFilter, minFilter, wrapS, wrapT);
	}

}

#else
void LoadGLTFTextures(tinygltf::Material const& material, StaticMesh* mesh, tinygltf::Model& model, Renderer* renderer, std::string const& texturePath)
{
	// Index should be 0 or above. If not, there is no texture.
	int baseColorTextureIndex	= material.pbrMetallicRoughness.baseColorTexture.index;
	int normalTextureIndex		= material.normalTexture.index;
	// int metallicRoughnessTextureIndex			= material.pbrMetallicRoughness.metallicRoughnessTexture.index;

	if(baseColorTextureIndex >= 0)
	{
		tinygltf::Texture& baseColorTexture = model.textures[baseColorTextureIndex];
		int baseColorTextureImageIndex = baseColorTexture.source;
		tinygltf::Image& albedo = model.images[baseColorTextureImageIndex];

		// #NOTE: The way I'm loading images here IS NOT OPTIMAL. Essentially I am loading the image twice as tinygltf already loads it while loading the mesh. 
		// Perhaps use the image data from tinygltf
		Texture* albedoTexture = renderer->CreateOrGetTextureFromFilePath((texturePath + albedo.uri).c_str());
		mesh->m_albedoTexture = albedoTexture;

		// Sampler
		int samplerIndex = baseColorTexture.sampler;
		tinygltf::Sampler& sampler = model.samplers[samplerIndex];
		int minFilter	= sampler.minFilter;
		int magFilter	= sampler.magFilter;
		int wrapS		= sampler.wrapS;
		int wrapT		= sampler.wrapT;
		mesh->m_diffuseSampler = ConvertGLTFSamplerToMode(magFilter, minFilter, wrapS, wrapT);
	}

	if(normalTextureIndex >= 0)
	{
		tinygltf::Texture& normalTextureTG = model.textures[normalTextureIndex];
		int normalTextureImageIndex = normalTextureTG.source;
		tinygltf::Image& normalTextureImage = model.images[normalTextureImageIndex];

		Texture* normalTexture = renderer->CreateOrGetTextureFromFilePath((texturePath + normalTextureImage.uri).c_str());
		mesh->m_normalTexture = normalTexture;

		// Sampler
		int samplerIndex = normalTextureTG.sampler;
		tinygltf::Sampler& sampler = model.samplers[samplerIndex];
		int minFilter = sampler.minFilter;
		int magFilter = sampler.magFilter;
		int wrapS = sampler.wrapS;
		int wrapT = sampler.wrapT;
		mesh->m_normalSampler = ConvertGLTFSamplerToMode(magFilter, minFilter, wrapS, wrapT);
	}
}
#endif

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void AddVertsForOBJMesh(std::string const& meshData, std::vector<Vertex_PCUTBN>& verts, std::vector<unsigned int>& indexes)
{
	std::vector<std::string>	objDataStrings;
	std::vector<Vec3>			positions;
	std::vector<Vec3>			normals;
	std::vector<Vec2>			texCoords;
	
	bool computeTangents = false;
	bool computeNormals = true;

	size_t dataSize = meshData.size();

	objDataStrings.reserve(static_cast<size_t>(dataSize * 0.25f));
	positions.reserve(static_cast<size_t>(dataSize * 0.25f));
	normals.reserve(static_cast<size_t>(dataSize * 0.25f));
	texCoords.reserve(static_cast<size_t>(dataSize * 0.25f));
	verts.reserve(static_cast<size_t>(dataSize * 0.25f));
	indexes.reserve(static_cast<size_t>(dataSize * 0.25f));
	
	objDataStrings = SplitStringOnDelimiter(meshData, '\n');

	objDataStrings.erase(std::remove_if(objDataStrings.begin(), objDataStrings.end(), [](std::string& objString) { return !objString.empty() && (objString[0] == '#' || objString[0] == '\r'); }), objDataStrings.end());

	for(std::string& objString : objDataStrings)
	{
		if(objString.empty())
		{
			continue;
		}

		int returnCaretIndex = static_cast<int>(objString.find_last_of('\r'));
		if(returnCaretIndex > -1)
		{
			objString.erase(objString.find_last_of('\r'));
		}

		std::vector<std::string> triangleString = SplitStringOnDelimiter(objString, ' ');

		if(triangleString[0] == "v")
		{
			Vec3 position = Vec3();
			position.x = static_cast<float>(atof(triangleString[1].c_str()));
			position.y = static_cast<float>(atof(triangleString[2].c_str()));
			position.z = static_cast<float>(atof(triangleString[3].c_str()));
			positions.push_back(position);
		}
		if(triangleString[0] == "vt")
		{
			Vec2 texCoord = Vec2();
			texCoord.x = static_cast<float>(atof(triangleString[1].c_str()));
			texCoord.y = static_cast<float>(atof(triangleString[2].c_str()));
			texCoords.push_back(texCoord);
			computeTangents = true;
		}

		if(triangleString[0] == "vn")
		{
			Vec3 normal = Vec3();
			normal.x = static_cast<float>(atof(triangleString[1].c_str()));
			normal.y = static_cast<float>(atof(triangleString[2].c_str()));
			normal.z = static_cast<float>(atof(triangleString[3].c_str()));
			normals.push_back(normal);
			computeNormals = false;
		}

		if(triangleString[0] == "f") 
		{
			Vertex_PCUTBN pcutbnVertex = Vertex_PCUTBN();
			std::vector<unsigned int> faceIndexes;

			for(int stringIndex = 1; stringIndex < static_cast<int>(triangleString.size()); ++stringIndex)
			{
				std::string faceString = triangleString[stringIndex];

				int firstSlashIndex		= static_cast<int>(faceString.find('/'));
				int secondSlashIndex	= static_cast<int>(faceString.find('/', firstSlashIndex + 1));

				if(firstSlashIndex == -1)
				{
					int posIndex = atoi(faceString.c_str()) - 1;
					pcutbnVertex.m_position		= positions[posIndex];
				}
				else if(firstSlashIndex != -1 && secondSlashIndex == -1)
				{
					std::string posIndexString = faceString.substr(0, firstSlashIndex);
					std::string uvIndexString = faceString.substr(firstSlashIndex + 1);

					int posIndex = atoi(posIndexString.c_str()) - 1;
					int uvIndex = atoi(uvIndexString.c_str()) - 1;

					pcutbnVertex.m_position = positions[posIndex];
					pcutbnVertex.m_uvCoords = texCoords[uvIndex];
				}
				else if(firstSlashIndex > -1 && secondSlashIndex > -1)
				{
					if(secondSlashIndex == firstSlashIndex + 1)
					{
						std::string posIndexString = faceString.substr(0, firstSlashIndex);
						std::string normalIndexString = faceString.substr(secondSlashIndex + 1);

						int posIndex = atoi(posIndexString.c_str()) - 1;
						int normalIndex = atoi(normalIndexString.c_str()) - 1;

						pcutbnVertex.m_position = positions[posIndex];
						pcutbnVertex.m_normal = normals[normalIndex];
					}
					else
					{
						std::string posIndexString		= faceString.substr(0, firstSlashIndex);
						std::string uvIndexString		= faceString.substr(firstSlashIndex + 1, secondSlashIndex - firstSlashIndex - 1);
						std::string normalIndexString	= faceString.substr(secondSlashIndex + 1);

						int posIndex	= atoi(posIndexString.c_str()) - 1;
						int uvIndex		= atoi(uvIndexString.c_str()) - 1;
						int normalIndex = atoi(normalIndexString.c_str()) - 1;

						pcutbnVertex.m_position = positions[posIndex];
						pcutbnVertex.m_uvCoords = texCoords[uvIndex];
						pcutbnVertex.m_normal	= normals[normalIndex];
					}
				}

				unsigned int vertexIndex = static_cast<unsigned int>(verts.size());

				for(int i = 0; i < static_cast<int>(verts.size()); ++i)
				{
					Vertex_PCUTBN const& vert = verts[i];

					if(vert.m_position == pcutbnVertex.m_position && vert.m_uvCoords == pcutbnVertex.m_uvCoords && vert.m_normal == pcutbnVertex.m_normal)
					{
						vertexIndex = i;
						break;
					}
				}

				if(vertexIndex == static_cast<unsigned int>(verts.size()))
				{
					verts.push_back(pcutbnVertex);
				}
				faceIndexes.push_back(vertexIndex);
			}

			for(int i = 1; i < static_cast<int>(faceIndexes.size()) - 1; ++i)
			{
				indexes.push_back(faceIndexes[0]);
				indexes.push_back(faceIndexes[i]);
				indexes.push_back(faceIndexes[i + 1]);
			}

		}
	}

	CalculateTangentSpaceBasisVectors(verts, indexes, computeTangents, computeNormals);
}
