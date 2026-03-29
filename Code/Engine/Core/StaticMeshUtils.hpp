#pragma once
#include <string>
#include <vector>
#include "ThirdParty/tinygltf/tiny_gltf.h"

struct Vertex_PCUTBN;
class StaticMesh;
class Renderer;
class DX12Renderer;

void AddVertsForOBJMesh(std::string const& meshData, std::vector<Vertex_PCUTBN>& verts, std::vector<unsigned int>& indexes);
bool LoadStaticMeshFileFromXML(std::string const& meshFilePath);
void CalculateTangentSpaceBasisVectors(std::vector<Vertex_PCUTBN>& vertexes, std::vector<unsigned int>& indexes, bool computeTangents, bool computeNormals);

#if defined (USING_DX12)
void LoadGLTFTextures(tinygltf::Material const& material, StaticMesh* mesh, tinygltf::Model& model, DX12Renderer* renderer, std::string const& texturePath);
#else
void LoadGLTFTextures(tinygltf::Material const& material, StaticMesh* mesh, tinygltf::Model& model, Renderer* renderer, std::string const& texturePath);
#endif