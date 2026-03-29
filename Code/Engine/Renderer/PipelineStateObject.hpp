#pragma once

#include <string>
//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
struct ID3D12PipelineState;
struct ID3D12Device10;

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
class PipelineStateObject
{
	friend class DX12Renderer;

private:
	PipelineStateObject(std::string name);
	~PipelineStateObject();

public:
	ID3D12PipelineState* GetD3DPSO() const;

private:
	std::string			 m_name;
	ID3D12PipelineState* m_pso = nullptr;

};