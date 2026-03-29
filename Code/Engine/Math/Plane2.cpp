#include "Engine/Math/Plane2.hpp"
#include "Engine/Math/MathUtils.hpp"

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
float Plane2::GetAltitudeFromPoint(Vec2 const& point) const
{
	return DotProduct2D(point, m_normal) - m_distanceFromOrigin;
}
