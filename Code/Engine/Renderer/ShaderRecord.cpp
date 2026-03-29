#include "Engine/Renderer/ShaderRecord.hpp"
#include <cstdint>
#include <string>
//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
ShaderRecord::ShaderRecord(void* shaderIdentifier, unsigned int shaderIdentifierSize)
{
	m_shaderIdentifier.dataPtr	= shaderIdentifier;
	m_shaderIdentifier.dataSize = shaderIdentifierSize;

	m_localRootArgs.dataPtr = nullptr;
	m_localRootArgs.dataSize = 0;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
ShaderRecord::ShaderRecord(void* shaderIdentifier, unsigned int shaderIdentifierSize, void* localRootArgs, unsigned int localRootArgsSize)
{
	m_shaderIdentifier.dataPtr = shaderIdentifier;
	m_shaderIdentifier.dataSize = shaderIdentifierSize;

	m_localRootArgs.dataPtr = localRootArgs;
	m_localRootArgs.dataSize = localRootArgsSize;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void ShaderRecord::CopyTo(void* destination) const
{
	uint8_t* byteDest = static_cast<uint8_t*>(destination);

	memcpy(byteDest, m_shaderIdentifier.dataPtr, m_shaderIdentifier.dataSize);

	if(m_localRootArgs.dataPtr)
	{
		memcpy(byteDest + m_shaderIdentifier.dataSize, m_localRootArgs.dataPtr, m_localRootArgs.dataSize);
	}
}
