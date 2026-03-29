#include "Engine/Renderer/RendererUtils.hpp"
#include "Engine/Core/EngineCommon.hpp"

SamplerMode ConvertGLTFSamplerToMode(int magFilter, int minFilter, int wrapS, int wrapT)
{
	UNUSED(wrapT);
	// Determine filter type
	bool isMagNearest = (magFilter == 9728); // NEAREST
//	bool isMagLinear = (magFilter == 9729);  // LINEAR
	
	bool isMinNearest = (minFilter == 9728 || minFilter == 9984); // NEAREST or NEAREST_MIPMAP_NEAREST
//	bool isMinLinear = (minFilter == 9729 || minFilter == 9985 || minFilter == 9986 || minFilter == 9987); // LINEAR variants
	
	// Simplify: if both mag and min are nearest -> POINT, otherwise -> BILINEAR/TRILINEAR
	bool usePoint = isMagNearest && isMinNearest;
	
	// Determine wrap mode (assuming S and T are the same for simplicity)
	int wrapMode = wrapS; // Use wrapS as primary, could check if wrapS != wrapT
	
	// Build the mode
	if(usePoint)
	{
		switch(wrapMode)
		{
		case 10497: return SamplerMode::POINT_WRAP;    // REPEAT
		case 33071: return SamplerMode::POINT_CLAMP;   // CLAMP_TO_EDGE
		case 33648: return SamplerMode::POINT_MIRROR;  // MIRRORED_REPEAT
		default:    return SamplerMode::POINT_CLAMP;   // Fallback
		}
	}
	else // Bilinear/Trilinear (we treat them the same since D3D11 handles mipmaps automatically)
	{
		switch(wrapMode)
		{
		case 10497: return SamplerMode::BILINEAR_WRAP;    // REPEAT
		case 33071: return SamplerMode::BILINEAR_CLAMP;   // CLAMP_TO_EDGE
		case 33648: return SamplerMode::BILINEAR_MIRROR;  // MIRRORED_REPEAT
		default:    return SamplerMode::BILINEAR_WRAP;    // Fallback
		}
	}
}
