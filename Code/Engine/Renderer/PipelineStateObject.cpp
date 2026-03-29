#define WIN32_LEAN_AND_MEAN
#include "Engine/Renderer/PipelineStateObject.hpp"
#include "Engine/Core/EngineCommon.hpp"
#include <d3d12.h>

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
PipelineStateObject::PipelineStateObject(std::string name)
	: m_name(name)
{

}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
PipelineStateObject::~PipelineStateObject()
{
	DX_SAFE_RELEASE(m_pso);
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
ID3D12PipelineState* PipelineStateObject::GetD3DPSO() const
{
	return m_pso;
}
