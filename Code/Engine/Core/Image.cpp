#include "Engine/Core/Image.hpp"
#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Core/DebugRender.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include "ThirdParty/stb/stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "ThirdParty/stb/stb_image_write.h"

//------------------------------------------------------------------------------------------------------------------
Image::~Image()
{

}

//------------------------------------------------------------------------------------------------------------------
Image::Image(char const* imageFilePath)
	: m_imageFilePath(imageFilePath)
{
	
	int numChannels;

	stbi_set_flip_vertically_on_load(1);
	unsigned char* imageData = stbi_load(imageFilePath, &m_dimensions.x, &m_dimensions.y, &numChannels, 0);

	if(!imageData)
	{
		DebugAddMessage(Stringf("Image Not Loaded: %s", imageFilePath), 10.f, Rgba8::RED);
	}

	int numTexels = m_dimensions.x * m_dimensions.y;
	
	m_rgbaTexels.resize(numTexels);

	int byteNum = 0;

	for(int tIndex = 0; tIndex < numTexels; ++tIndex)
	{
		m_rgbaTexels[tIndex].r = imageData[byteNum];
		
		byteNum++;

		m_rgbaTexels[tIndex].g = imageData[byteNum];

		byteNum++;

		m_rgbaTexels[tIndex].b = imageData[byteNum];

		byteNum++;

		if(numChannels < 4)
		{
			m_rgbaTexels[tIndex].a = 255;
			continue;
		}

		m_rgbaTexels[tIndex].a = imageData[byteNum];

		byteNum++;

	}

	stbi_image_free(imageData);

}

//------------------------------------------------------------------------------------------------------------------
Image::Image(IntVec2 size, Rgba8 color)
	: m_dimensions(size)
{

	int numTexels = m_dimensions.x * m_dimensions.y;

	m_rgbaTexels.resize(numTexels);

	int byteNum = 0;

	for(int tIndex = 0; tIndex < numTexels; ++tIndex)
	{
		m_rgbaTexels[tIndex].r = color.r;

		byteNum++;

		m_rgbaTexels[tIndex].g = color.g;

		byteNum++;

		m_rgbaTexels[tIndex].b = color.b;

		byteNum++;

		m_rgbaTexels[tIndex].a = color.a;

		byteNum++;

	}

}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
Image::Image(unsigned char* imageData, std::string const& imageFilePath, IntVec2 const& dimensions, int numChannels)
	: m_imageFilePath(imageFilePath)
	, m_dimensions(dimensions)
{
	if(!imageData)
	{
		DebugAddMessage(Stringf("Image Not Loaded: %s", imageFilePath.c_str()), 10.f, Rgba8::RED);
	}

	int numTexels = m_dimensions.x * m_dimensions.y;

	m_rgbaTexels.resize(numTexels);

	int byteNum = 0;

	for(int tIndex = 0; tIndex < numTexels; ++tIndex)
	{
		m_rgbaTexels[tIndex].r = imageData[byteNum];

		byteNum++;

		m_rgbaTexels[tIndex].g = imageData[byteNum];

		byteNum++;

		m_rgbaTexels[tIndex].b = imageData[byteNum];

		byteNum++;

		if(numChannels < 4)
		{
			m_rgbaTexels[tIndex].a = 255;
			continue;
		}

		m_rgbaTexels[tIndex].a = imageData[byteNum];

		byteNum++;

	}
}
//------------------------------------------------------------------------------------------------------------------
std::string const& Image::GetImageFilePath() const
{

	return m_imageFilePath;

}

//------------------------------------------------------------------------------------------------------------------
IntVec2 Image::GetDimensions() const
{
	return m_dimensions;
}


//------------------------------------------------------------------------------------------------------------------
Rgba8 Image::GetTexelColor(IntVec2 const& texelCoords) const
{
	int texelIndex = (texelCoords.y * m_dimensions.x) + texelCoords.x;

	return m_rgbaTexels[texelIndex];
}


//------------------------------------------------------------------------------------------------------------------
void Image::SetTexelColor(IntVec2 const& texelCoords, Rgba8 const& newColor)
{

	int texelIndex = (texelCoords.y * m_dimensions.x) + texelCoords.x;
	
	m_rgbaTexels[texelIndex] = newColor;

}

//------------------------------------------------------------------------------------------------------------------
const void* Image::GetRawData() const
{

	return m_rgbaTexels.data();

}


