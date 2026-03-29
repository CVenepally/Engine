#include "Engine/Math/ConvexPoly2.hpp"

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
ConvexPoly2::ConvexPoly2(std::vector<Vec2> const& vertexPositions)
	: m_vertexPositions(vertexPositions)
{}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void ConvexPoly2::GetVertexPositions(std::vector<Vec2>& out_positions) const
{
	for(Vec2 const& pos : m_vertexPositions)
	{
		out_positions.push_back(pos);
	}
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void ConvexPoly2::OffsetVertexPositions(Vec2 const& offset)
{
	for(Vec2& pos : m_vertexPositions)
	{
		pos += offset;
	}
}
