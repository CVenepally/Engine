#include "Engine/Core/BufferUtils.hpp"
#include "Engine/Core/EngineCommon.hpp"
//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
BufferParser::BufferParser(unsigned char const* bufferToParse, size_t sizeInBytes)
	: m_dataStart(bufferToParse)
	, m_dataSize(sizeInBytes)
{}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
unsigned char BufferParser::ParseByte()
{
	unsigned char byteValue = m_dataStart[m_currentReadOffSet];
	m_currentReadOffSet += 1;
	return byteValue;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
float BufferParser::ParseFloat()
{
	float finalValue;

	unsigned char* floatAsArrayOfBytes = reinterpret_cast<unsigned char*>(&finalValue);

	floatAsArrayOfBytes[0] = ParseByte();
	floatAsArrayOfBytes[1] = ParseByte();
	floatAsArrayOfBytes[2] = ParseByte();
	floatAsArrayOfBytes[3] = ParseByte();

	return finalValue;
}

BufferWriter::BufferWriter(std::vector<unsigned char>& bufferToWriteTo)
{
	UNUSED(bufferToWriteTo);
}

void BufferWriter::AppendByte(unsigned char byteToAppend)
{
	UNUSED(byteToAppend)
}

void BufferWriter::AppendFloat(float floatToAppend)
{
	UNUSED(floatToAppend)
}
