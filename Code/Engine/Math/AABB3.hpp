#pragma once
#include "Engine/Math/Vec3.hpp"

#include <vector>
//------------------------------------------------------------------------------------------------------------------

enum AABB3Points
{
	AABB3_BACK_RIGHT_BOTTOM,
	AABB3_BACK_RIGHT_TOP,
	AABB3_BACK_LEFT_TOP,
	AABB3_BACK_LEFT_BOTTOM,

	AABB3_FORWARD_LEFT_BOTTOM,
	AABB3_FORWARD_RIGHT_BOTTOM,
	AABB3_FORWARD_RIGHT_TOP,
	AABB3_FORWARD_LEFT_TOP,
	
	AABB3_COUNT
};

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
struct AABB3
{
public:
	Vec3 m_mins;
	Vec3 m_maxs;

public:
	AABB3() {}
	~AABB3() {}

	AABB3(AABB3 const& copyFrom);
	explicit AABB3(float minX, float minY, float minZ, float maxX, float maxY, float maxZ);
	explicit AABB3(Vec3 mins, Vec3 maxs);

	void GetCornerPoints(std::vector<Vec3>& out_CornerPoints) const;
	Vec3 GetCenter() const;

	void SetCenter(Vec3 newCenter);
};