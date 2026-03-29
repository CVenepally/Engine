#pragma once
#include <string>
#include "Game/EngineBuildPreferences.hpp"

//------------------------------------------------------------------------------------------------------------------
struct	ID3D11VertexShader;
struct	ID3D11PixelShader;
struct	ID3D11InputLayout;
struct	ID3D10Blob;
typedef ID3D10Blob ID3DBlob;

//------------------------------------------------------------------------------------------------------------------
struct ShaderConfig
{
	std::string		m_name;
	std::string		m_vertexEntryPoint = "VertexMain";
	std::string		m_pixelEntryPoint = "PixelMain";
};

//------------------------------------------------------------------------------------------------------------------

class Shader
{
	friend class Renderer;
	friend class DX12Renderer;
	friend class ShaderCompiler;
	friend struct PipelineStateStream;

private:
			 Shader();
	explicit Shader(const ShaderConfig& config);
	Shader(const Shader& copy) = delete;

	~Shader();

	const std::string& GetName() const;

	void ReleaseVertexShader();
	void ReleasePixelShader();
	void ReleaseInputLayout();

private:

	ShaderConfig m_config;

#if defined USING_DX12

	ID3DBlob* m_vsBlob = nullptr;
	ID3DBlob* m_psBlob = nullptr;
	ID3DBlob* m_rtBlob = nullptr;
	ID3DBlob* m_csBlob = nullptr;

#else
	ID3D11VertexShader* m_vertexShader = nullptr;
	ID3D11PixelShader*	m_pixelShader  = nullptr;
	ID3D11InputLayout*	m_inputLayout  = nullptr;
#endif

};