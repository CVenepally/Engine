#include "Engine/Math/AABB3.hpp"


//------------------------------------------------------------------------------------------------------------------
AABB3::AABB3(AABB3 const& copyFrom)
{

	m_mins = copyFrom.m_mins;
	m_maxs = copyFrom.m_maxs;

}


//------------------------------------------------------------------------------------------------------------------
AABB3::AABB3(float minX, float minY, float minZ, float maxX, float maxY, float maxZ)
{

	m_mins = Vec3(minX, minY, minZ);
	m_maxs = Vec3(maxX, maxY, maxZ);

}

//------------------------------------------------------------------------------------------------------------------
AABB3::AABB3(Vec3 mins, Vec3 maxs)
	: m_mins(mins)
	, m_maxs(maxs)
{

}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void AABB3::GetCornerPoints(std::vector<Vec3>& out_CornerPoints) const
{
	Vec3 backRightBottom = m_mins;                                            // BACK_RIGHT_BOTTOM
	Vec3 backRightTop = Vec3(m_mins.x, m_mins.y, m_maxs.z);                // BACK_RIGHT_TOP
	Vec3 backLeftTop = Vec3(m_mins.x, m_maxs.y, m_maxs.z);                // BACK_LEFT_TOP
	Vec3 backLeftBottom = Vec3(m_mins.x, m_maxs.y, m_mins.z);                // BACK_LEFT_BOTTOM

	Vec3 forwardLeftBottom = Vec3(m_maxs.x, m_maxs.y, m_mins.z);              // FORWARD_LEFT_BOTTOM
	Vec3 forwardRightBottom = Vec3(m_maxs.x, m_mins.y, m_mins.z);              // FORWARD_RIGHT_BOTTOM
	Vec3 forwardRightTop = Vec3(m_maxs.x, m_mins.y, m_maxs.z);              // FORWARD_RIGHT_TOP
	Vec3 forwardLeftTop = m_maxs;                                          // FORWARD_LEFT_TOP

	out_CornerPoints.push_back(backRightBottom);    // 0
	out_CornerPoints.push_back(backRightTop);       // 1
	out_CornerPoints.push_back(backLeftTop);        // 2
	out_CornerPoints.push_back(backLeftBottom);     // 3

	out_CornerPoints.push_back(forwardLeftBottom);  // 4
	out_CornerPoints.push_back(forwardRightBottom); // 5
	out_CornerPoints.push_back(forwardRightTop);    // 6
	out_CornerPoints.push_back(forwardLeftTop);     // 7}
}
//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
Vec3 AABB3::GetCenter() const
{

	return (m_mins + m_maxs) * 0.5f;

}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void AABB3::SetCenter(Vec3 newCenter)
{

	Vec3 centerDifference = newCenter - GetCenter();

	m_mins += centerDifference;
	m_maxs += centerDifference;

}

