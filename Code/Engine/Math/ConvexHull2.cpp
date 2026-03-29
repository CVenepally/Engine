#include "Engine/Math/ConvexHull2.hpp"
#include "Engine/Math/ConvexPoly2.hpp"
#include "Engine/Math/MathUtils.hpp"

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
ConvexHull2::ConvexHull2(std::vector<Plane2> const& planes)
	:m_planes(planes)
{

}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
ConvexHull2::ConvexHull2(ConvexPoly2 const& convexPoly)
{
	std::vector<Vec2> positions;
	
	convexPoly.GetVertexPositions(positions);
	
	for(size_t posIndex = 0; posIndex < positions.size(); ++posIndex)
	{
		Vec2 start	= positions[posIndex];
		Vec2 end	= positions[(posIndex + size_t(1)) % positions.size()];

		Plane2 plane;
		plane.m_normal = ((end - start).GetNormalized()).GetRotatedMinus90Degrees();
		plane.m_distanceFromOrigin = DotProduct2D(start, plane.m_normal);

		m_planes.push_back(plane);
	}
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void ConvexHull2::OffsetPlanes(float distanceOffset)
{
	for(Plane2& plane : m_planes)
	{
		plane.m_distanceFromOrigin += distanceOffset;
	}
}

