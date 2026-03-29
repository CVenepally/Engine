#pragma once
#include "Engine/Renderer/RendererUtils.hpp"
#include "Game/EngineBuildPreferences.hpp"
#include <string>

#if defined(USING_DX12)

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
struct IDxcUtils;
struct IDxcCompiler3;
struct IDxcIncludeHandler;
//struct ID3DBlob;
struct IDxcBlob;
struct DxcBuffer;

class Shader;
//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
class ShaderCompiler
{
	friend class DX12Renderer;
	friend class Renderer;

private:
				ShaderCompiler();
	virtual		~ShaderCompiler();

	Shader*		CreateShaderFromFile(std::string const& shaderName, ShaderType shaderType = SHADER_PIXEL | SHADER_VERTEX);
	
	DxcBuffer	GenerateShaderSourceBuffer(std::wstring const& shaderSourceFilePath);
	
	void		CreateShader(DxcBuffer const& shaderBuffer, ID3DBlob** blobToCopyTo, std::wstring const& shaderTarget, std::wstring const& entryPoint, std::wstring const& name = L"Shader", std::string const& outputFileName = "Default");
	
	void		CopyToD3DBlob(IDxcBlob* sourceBlob, ID3DBlob** destinationBlob);

//	void		SaveCompiledShaderObject();
//	void		SavePDBObject();
	bool		TryLoadCompiledShaderObject(std::string const& shaderName, Shader* shader, ShaderType shaderType);

private:
	IDxcUtils*			m_dxcUtils			= nullptr;
	IDxcCompiler3*		m_dxcCompiler		= nullptr;
	IDxcIncludeHandler* m_dxcIncludeHandler = nullptr;

};

#endif