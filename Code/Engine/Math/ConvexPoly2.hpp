#pragma once
#include <vector>
#include "Engine/Math/Vec2.hpp"

struct ConvexPoly2
{
	ConvexPoly2() = default;
	explicit ConvexPoly2(std::vector<Vec2> const& vertexPositions);
	~ConvexPoly2() = default;

	void GetVertexPositions(std::vector<Vec2>& out_positions) const;
	void OffsetVertexPositions(Vec2 const& offset);

private:
	std::vector<Vec2> m_vertexPositions;		
};