#include <d3d11.h>
#include <d3d12.h>
#include "Engine/Renderer/Shader.hpp"
#include "Engine/Core/EngineCommon.hpp"

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
Shader::Shader()
{
	m_config = ShaderConfig();
}

//------------------------------------------------------------------------------------------------------------------
Shader::Shader(const ShaderConfig& config)
	:m_config(config)
{
}

//------------------------------------------------------------------------------------------------------------------
Shader::~Shader()
{
#if defined USING_DX12
	DX_SAFE_RELEASE(m_vsBlob);
	DX_SAFE_RELEASE(m_psBlob);
	DX_SAFE_RELEASE(m_rtBlob);
#else

	DX_SAFE_RELEASE(m_vertexShader);
	DX_SAFE_RELEASE(m_pixelShader);
	DX_SAFE_RELEASE(m_inputLayout);
#endif

}

//------------------------------------------------------------------------------------------------------------------
const std::string& Shader::GetName() const
{
	return m_config.m_name;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void Shader::ReleaseVertexShader()
{
#if defined USING_DX12
	DX_SAFE_RELEASE(m_vsBlob);
#else
	DX_SAFE_RELEASE(m_vertexShader);
#endif

}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void Shader::ReleasePixelShader()
{
#if defined USING_DX12

	DX_SAFE_RELEASE(m_psBlob);

#else

	DX_SAFE_RELEASE(m_pixelShader);

#endif

}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void Shader::ReleaseInputLayout()
{
// 	DX_SAFE_RELEASE(m_inputLayout);
}
