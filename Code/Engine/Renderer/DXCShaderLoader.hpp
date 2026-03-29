#pragma once

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
struct IDxcUtils;
struct IDxcCompiler3;
struct IDxcIncludeHandler;

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
class DXCShaderLoader
{
	friend class DX12Renderer;

private:
			DXCShaderLoader();
	virtual ~DXCShaderLoader();



private:
	
	IDxcUtils*			m_dxcUtils		 = nullptr;
	IDxcCompiler3*		m_dxcCompiler	 = nullptr;
	IDxcIncludeHandler*	m_includeHandler = nullptr;
};