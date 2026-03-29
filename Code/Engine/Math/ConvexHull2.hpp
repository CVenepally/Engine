#pragma once
#include "Engine/Math/Plane2.hpp"
#include <vector>

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
struct ConvexPoly2;

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
struct ConvexHull2
{
public:
	std::vector<Plane2> m_planes;

public:
	ConvexHull2() = default;
	explicit ConvexHull2(std::vector<Plane2> const& planes);
	explicit ConvexHull2(ConvexPoly2 const& covexPoly);
	
	~ConvexHull2() = default;

	void OffsetPlanes(float distanceOffset);

};