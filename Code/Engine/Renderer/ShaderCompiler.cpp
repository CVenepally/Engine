#include "Engine/Renderer/ShaderCompiler.hpp"
#include "Engine/Renderer/Shader.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Core/StringUtils.hpp"
#include "Engine/Core/FileUtils.hpp"
#include "Engine/Core/Time.hpp"

#include <vector>

#if defined(USING_DX12)

#include <d3d12.h>
#include <d3d11.h>

#include <dxcapi.h>
#include <d3d12shader.h>
#include <d3dcompiler.h>


#pragma comment (lib, "dxcompiler.lib")

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
ShaderCompiler::ShaderCompiler()
{

	HRESULT hr;
	hr = DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&m_dxcUtils));

	if(FAILED(hr))
	{
		ERROR_AND_DIE("Failed to Create DXC Utils")
	}

	hr = DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&m_dxcCompiler));
	if(FAILED(hr))
	{
		ERROR_AND_DIE("Failed to Create DXC Compiler")
	}

	// Make a different one?
	hr = m_dxcUtils->CreateDefaultIncludeHandler(&m_dxcIncludeHandler);
	if(FAILED(hr))
	{
		ERROR_AND_DIE("Failed to Create DXC Include Handler")
	}
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
ShaderCompiler::~ShaderCompiler()
{}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
Shader* ShaderCompiler::CreateShaderFromFile(std::string const& shaderName, ShaderType shaderType)
{	
	Shader* newShader = new Shader();

//#if !defined (ENGINE_DEBUG_RENDER)	
//
//	if(TryLoadCompiledShaderObject(shaderName, newShader, shaderType))
//	{
//		DebuggerPrintf("###========================================= LOADED FROM CSO FILE ===============================================###\n");
//		return newShader;
//	}
//
//#endif

	double timeStart = GetCurrentTimeSeconds();

	std::wstring name		= std::wstring(shaderName.begin(), shaderName.end()) + L".hlsl";
	DxcBuffer shaderSource	= GenerateShaderSourceBuffer(name.c_str());


	if(shaderType & SHADER_VERTEX)
	{
		CreateShader(shaderSource, &newShader->m_vsBlob, L"vs_6_0", L"VertexMain", L"Vertex Shader", shaderName + "_VS");
	}

	if(shaderType & SHADER_PIXEL)
	{
		CreateShader(shaderSource, &newShader->m_psBlob, L"ps_6_0", L"PixelMain", L"Pixel Shader", shaderName + "_PS");
	}

	if(shaderType & SHADER_COMPUTE)
	{
		CreateShader(shaderSource, &newShader->m_csBlob, L"cs_6_0", L"ComputeMain", L"Compute Shader", shaderName + "_CS");
	}

	if(shaderType & SHADER_RAYTRACE)
	{
		CreateShader(shaderSource, &newShader->m_rtBlob, L"lib_6_6", L"RT", L"RT Shader", shaderName);
		IDxcUtils* utils;
		DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&utils));

		ID3D12LibraryReflection* reflection;
		DxcBuffer reflectionData = {
			newShader->m_rtBlob->GetBufferPointer(),
			newShader->m_rtBlob->GetBufferSize(),
			0
		};

		utils->CreateReflection(&reflectionData, IID_PPV_ARGS(&reflection));

		D3D12_LIBRARY_DESC libDesc;
		reflection->GetDesc(&libDesc);

		DebuggerPrintf("Library has %d functions:\n", libDesc.FunctionCount);
		for(UINT i = 0; i < libDesc.FunctionCount; i++)
		{
			ID3D12FunctionReflection* funcReflection = reflection->GetFunctionByIndex(i);
			D3D12_FUNCTION_DESC funcDesc;
			funcReflection->GetDesc(&funcDesc);
			DebuggerPrintf("  Function: %s\n", funcDesc.Name);
		}
	}


	double timeTotal = GetCurrentTimeSeconds() - timeStart;

	DebuggerPrintf(Stringf("\n===============Shader Compiled. Name: %s. Time: %0.2f==============\n", shaderName.c_str(), timeTotal*1000.f).c_str());


	return newShader;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
DxcBuffer ShaderCompiler::GenerateShaderSourceBuffer(std::wstring const& shaderSourceFilePath)
{
	HRESULT hr;

	IDxcBlobEncoding* shaderSource = nullptr;
	hr = m_dxcUtils->LoadFile(shaderSourceFilePath.c_str(), nullptr, &shaderSource);

	if(FAILED(hr))
	{
		ERROR_AND_DIE("Failed to Load Shader File")
	}

	DxcBuffer shaderBuffer;
	shaderBuffer.Ptr = shaderSource->GetBufferPointer();
	shaderBuffer.Size = shaderSource->GetBufferSize();
	shaderBuffer.Encoding = DXC_CP_UTF8;

	return shaderBuffer;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void ShaderCompiler::CreateShader(DxcBuffer const& shaderBuffer, ID3DBlob** blobToCopyTo, std::wstring const& shaderTarget, std::wstring const& entryPoint, std::wstring const& name, std::string const& outputFileName)
{
	std::vector<LPCWSTR> shaderArgs;
	shaderArgs.push_back(name.c_str());

	// #TODO - Do something about reflection data?
	// EntryPoint

	if(shaderTarget != L"lib_6_3")
	{
		shaderArgs.push_back(L"-E");
		shaderArgs.push_back(entryPoint.c_str());
	}
	
	// Shader model
	shaderArgs.push_back(L"-T");
	shaderArgs.push_back(shaderTarget.c_str());
	
	// Includes
	shaderArgs.push_back(L"-I");
	shaderArgs.push_back(L"Data/Shaders");

	// Output file name
	std::string saveFilePath = Stringf("%s.cso", outputFileName.c_str());
	std::wstring wSaveFilePath(saveFilePath.begin(), saveFilePath.end());
	shaderArgs.push_back(L"-Fo");
	shaderArgs.push_back(wSaveFilePath.c_str());

	// Debug and Release Optimizations
#if defined (ENGINE_DEBUG_RENDER)

	// PDB file name
	std::string pdbFilePath = Stringf("%s.pdb", outputFileName.c_str());
	std::wstring wPDBFilePath(pdbFilePath.begin(), pdbFilePath.end());
	shaderArgs.push_back(L"-Fd");
	shaderArgs.push_back(wPDBFilePath.c_str());

	shaderArgs.push_back(L"-Zi"); // 
	shaderArgs.push_back(L"-WX"); // Warnings as erros
	shaderArgs.push_back(L"-Od"); // No optimization
#else
	shaderArgs.push_back(L"-O3"); // Level 3 Optimization
#endif

	HRESULT hr;

	// Compile Shader
	IDxcResult* shaderResult = nullptr;
	hr = m_dxcCompiler->Compile(&shaderBuffer, shaderArgs.data(), static_cast<UINT32>(shaderArgs.size()), m_dxcIncludeHandler, IID_PPV_ARGS(&shaderResult));

	// Create blobs from Results
	// error blob
	IDxcBlobUtf8* errorBlob = nullptr;
	IDxcBlobWide* errorBlobName = nullptr;
	shaderResult->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&errorBlob), &errorBlobName);

	if(errorBlob && errorBlob->GetStringLength() != 0)
	{
		DebuggerPrintf("DXC ERROR BLOB: %s\n", errorBlob->GetStringPointer());
	}

	if(FAILED(hr) || !shaderResult)
	{
		ERROR_AND_DIE("Failed to Compile Shader")
	}


	// shaderBlob
	IDxcBlob*		shaderBlob = nullptr;
	IDxcBlobUtf16*	shaderName = nullptr;
	hr = shaderResult->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&shaderBlob), &shaderName);
	if(FAILED(hr) || !shaderBlob)
	{
		ERROR_AND_DIE("Failed to Get Shader Output blob")
	}
	
	// Save CSO File
	FILE* shaderFile = nullptr;
	int result = _wfopen_s(&shaderFile, shaderName->GetStringPointer(), L"wb");

	if(result != 0 || !shaderFile)
	{
		ERROR_AND_DIE("Failed to create compiled shader file.");
	}

	fwrite(shaderBlob->GetBufferPointer(), shaderBlob->GetBufferSize(), 1, shaderFile);
	fclose(shaderFile);

	CopyToD3DBlob(shaderBlob, blobToCopyTo);

	shaderBlob->Release();
	shaderName->Release();

#if defined(ENGINE_DEBUG_RENDER)
	// PDB
	IDxcBlob*		pdbBlob = nullptr;
	IDxcBlobUtf16*	pdbFileName = nullptr;
	hr = shaderResult->GetOutput(DXC_OUT_PDB, IID_PPV_ARGS(&pdbBlob), &pdbFileName);
	if(FAILED(hr) || !pdbBlob)
	{
		ERROR_AND_DIE("Failed to Get PDB blob")
	}

	// Save PDB File
	FILE* pdbFile = nullptr;
	result = _wfopen_s(&pdbFile, pdbFileName->GetStringPointer(), L"wb");

	if(result != 0 || !pdbFile)
	{
		ERROR_AND_DIE("Failed to create PDB shader file.");
	}

	fwrite(pdbBlob->GetBufferPointer(), pdbBlob->GetBufferSize(), 1, pdbFile);
	fclose(pdbFile);
#endif


}
//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void ShaderCompiler::CopyToD3DBlob(IDxcBlob* sourceBlob, ID3DBlob** destinationBlob)
{
	HRESULT hr;

	if(!*destinationBlob)
	{
		hr = D3DCreateBlob(sourceBlob->GetBufferSize(), destinationBlob);

		if(FAILED(hr) || !*destinationBlob)
		{
			ERROR_AND_DIE("Failed to Create shader blob")
		}
	}
	
	memcpy((*destinationBlob)->GetBufferPointer(), sourceBlob->GetBufferPointer(), sourceBlob->GetBufferSize());

	if(!*destinationBlob)
	{
		ERROR_AND_DIE("Failed to mempy shader")
	}
}
//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
bool ShaderCompiler::TryLoadCompiledShaderObject(std::string const& shaderName, Shader* shader, ShaderType shaderType)
{
	double timeStart = GetCurrentTimeSeconds();

	if(shaderType & SHADER_RAYTRACE)
	{
		std::string nameWithPath = Stringf("%s.cso", shaderName.c_str());

		if(!DoesFileExist(nameWithPath))
		{
			return false;
		}

		std::vector<uint8_t> buffer;


		HRESULT hr;

		if(FileReadToBuffer(buffer, nameWithPath))
		{
			hr = D3DCreateBlob(buffer.size(), &shader->m_rtBlob);

			if(FAILED(hr) || !shader->m_rtBlob)
			{
				return false;
			}
		}

		memcpy(shader->m_rtBlob->GetBufferPointer(), buffer.data(), buffer.size());

		if(!shader->m_rtBlob)
		{
			return false;
		}
	}

	// VS
	if(shaderType & SHADER_VERTEX)
	{
		std::string nameWithPath = Stringf("%s_VS.cso", shaderName.c_str());

		if(!DoesFileExist(nameWithPath))
		{
			return false;
		}

		std::vector<uint8_t> buffer;


		HRESULT hr;

		if(FileReadToBuffer(buffer, nameWithPath))
		{
			hr = D3DCreateBlob(buffer.size(), &shader->m_vsBlob);

			if(FAILED(hr) || !shader->m_vsBlob)
			{
				return false;
			}
		}

		memcpy(shader->m_vsBlob->GetBufferPointer(), buffer.data(), buffer.size());

		if(!shader->m_vsBlob)
		{
			return false;
		}
	}

	// PS
	if(shaderType & SHADER_PIXEL)
	{
		std::string nameWithPath = Stringf("%s_PS.cso", shaderName.c_str());

		if(!DoesFileExist(nameWithPath))
		{
			return false;
		}

		std::vector<uint8_t> buffer;


		HRESULT hr;

		if(FileReadToBuffer(buffer, nameWithPath))
		{
			hr = D3DCreateBlob(buffer.size(), &shader->m_psBlob);

			if(FAILED(hr) || !shader->m_psBlob)
			{
				return false;
			}
		}

		memcpy(shader->m_psBlob->GetBufferPointer(), buffer.data(), buffer.size());

		if(!shader->m_psBlob)
		{
			return false;
		}
	}

	double timeTotal = GetCurrentTimeSeconds() - timeStart;

	DebuggerPrintf(Stringf("\n===============Compiled Shader Loaded. Name: %s Time: %0.2f==============\n", shaderName.c_str(), timeTotal * 1000.f).c_str());

	return true;
}
#endif
