#pragma once

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
struct ShaderRecordEntry
{
	friend class ShaderRecord;
private:

	void*			dataPtr;
	unsigned int	dataSize;
};

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
typedef ShaderRecordEntry ShaderIdentifierEntry;
typedef ShaderRecordEntry LocalRootArgsEntry;

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Shader Record is combination of a shader ID and it's root args (more clarification is required)
class ShaderRecord
{
	friend class ShaderTable;
	friend class DX12Renderer;
	friend class CommandList;

private:
	ShaderRecord(void* shaderIdentifier, unsigned int shaderIdentifierSize);
	ShaderRecord(void* shaderIdentifier, unsigned int shaderIdentifierSize, void* localRootArgs, unsigned int localRootArgsSize);

	void CopyTo(void* destination) const;

private:
	ShaderIdentifierEntry	m_shaderIdentifier;
	LocalRootArgsEntry		m_localRootArgs;
};

