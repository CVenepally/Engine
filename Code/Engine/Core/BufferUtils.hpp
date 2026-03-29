#pragma once
#include <vector>

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
class BufferParser
{
public:
	BufferParser(unsigned char const* bufferToParse, size_t sizeInBytes);

	unsigned char	ParseByte();
	float			ParseFloat();

public:
	unsigned char const*	m_dataStart			= nullptr;
	size_t					m_dataSize			= 0;
	size_t					m_currentReadOffSet = 0;
};

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
class BufferWriter
{
public:
	BufferWriter(std::vector<unsigned char>& bufferToWriteTo);
	void	AppendByte(unsigned char byteToAppend);
	void	AppendFloat(float floatToAppend);
};