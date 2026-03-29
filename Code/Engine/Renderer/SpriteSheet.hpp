#pragma once

#include "Engine/Math/IntVec2.hpp"
#include "Engine/Renderer/SpriteDefinition.hpp"
#include <vector>
//------------------------------------------------------------------------------------------------------------------
class Texture;

//------------------------------------------------------------------------------------------------------------------
class SpriteSheet
{

public:

	SpriteSheet() = default;
	explicit SpriteSheet(Texture& texture, IntVec2 const& simpleGridLayout);
	

	Texture&					GetTexture() const;
	int							GetNumSprites() const;
	SpriteDefinition const&		GetSpriteDef(int spriteIndex) const;
	void						GetSpriteUVs(Vec2& out_uvAtMins, Vec2& out_uvAtMaxs, int spriteIndex) const;
	AABB2						GetSpriteUVs(int spriteIndex) const;
	AABB2						GetSpriteUVs(IntVec2 spriteCoords, int spriteSheetWidth) const;
	AABB2						GetSpriteUVs(IntVec2 spriteCoords) const;


protected:

	Texture&							m_texture;
	std::vector<SpriteDefinition>		m_spriteDefs;
	IntVec2								m_sheetDimensions;	
};