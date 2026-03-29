#pragma once
#include "Engine/Math/Vec2.hpp"

struct Plane2
{
public:

	Vec2  m_normal;
	float m_distanceFromOrigin;

public:

	float GetAltitudeFromPoint(Vec2 const& point) const;

};