#include "Engine/Math/IntVec3.hpp"
#include "Engine/Math/Vec2.hpp"
#include "Engine/Math/IntVec2.hpp"
#include "Engine/Math/Vec3.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Core/StringUtils.hpp"

#include <math.h>


const IntVec3 IntVec3::ZERO		= IntVec3(0, 0, 0);
const IntVec3 IntVec3::ONE		= IntVec3(1, 1, 1);
const IntVec3 IntVec3::FORWARD  = IntVec3(1, 0, 0);
const IntVec3 IntVec3::BACKWARD = IntVec3(-1, 0, 0);
const IntVec3 IntVec3::RIGHT	= IntVec3(0, -1, 0);
const IntVec3 IntVec3::LEFT		= IntVec3(0, 1, 0);
const IntVec3 IntVec3::UP		= IntVec3(0, 0, 1);
const IntVec3 IntVec3::DOWN		= IntVec3(0, 0, -1);

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
IntVec3::IntVec3()
{
	x = 0;
	y = 0;
	z = 0;
}
//------------------------------------------------------------------------------------------------------------------
IntVec3::IntVec3(const IntVec3& copyFrom)
	: x(copyFrom.x)
	, y(copyFrom.y)
	, z(copyFrom.z) 
{}

//------------------------------------------------------------------------------------------------------------------
IntVec3::IntVec3(int initialX, int initialY, int initialZ)
	: x(initialX)
	, y(initialY)
	, z(initialZ)
{
}

//------------------------------------------------------------------------------------------------------------------
IntVec3::IntVec3(int initialXYZ)
	: x(initialXYZ)
	, y(initialXYZ)
	, z(initialXYZ)
{
}

//------------------------------------------------------------------------------------------------------------------
IntVec3::IntVec3(Vec3 const& vec3)
{
	x = static_cast<int>(vec3.x);
	y = static_cast<int>(vec3.y);
	z = static_cast<int>(vec3.z);
}

//------------------------------------------------------------------------------------------------------------------
IntVec3::IntVec3(Vec2 const& vec2)
{
	x = static_cast<int>(vec2.x);
	y = static_cast<int>(vec2.y);
	z = 0;
}

//------------------------------------------------------------------------------------------------------------------
IntVec3::IntVec3(IntVec2 const& intVec2)
{
	x = static_cast<int>(intVec2.x);
	y = static_cast<int>(intVec2.y);
	z = 0;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
float IntVec3::GetLength() const
{
	float xFloat = static_cast<float>(x);
	float yFloat = static_cast<float>(y);
	float zFloat = static_cast<float>(z);

	float length = static_cast<float>(sqrt((xFloat * xFloat) + (yFloat * yFloat)) + (zFloat * zFloat));

	return length;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
float IntVec3::GetLengthXY() const
{
	float xFloat = static_cast<float>(x);
	float yFloat = static_cast<float>(y);
	float zFloat = 0.f;

	float length = static_cast<float>(sqrt((xFloat * xFloat) + (yFloat * yFloat)) + (zFloat * zFloat));

	return length;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
int IntVec3::GetLengthSquared() const
{
	return (x * x) + (y * y) + (z * z);
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
int IntVec3::GetLengthXYSquared() const
{
	return (x * x) + (y * y);
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
float IntVec3::GetAngleAboutZRadians() const
{
	return atan2f(static_cast<float>(y), static_cast<float>(x));
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
float IntVec3::GetAngleAboutZDegrees() const
{
	return ATan2Degrees(static_cast<float>(y), static_cast<float>(x));
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
IntVec3 const IntVec3::GetRotatedAboutZRadians(float deltaRadians) const
{
	float length = GetLengthXY();
	float angle = atan2f(static_cast<float>(y), static_cast<float>(x));
	
	angle += deltaRadians;

	float rcos = length * cosf(angle);
	float rsin = length * sinf(angle);

	return IntVec3(static_cast<int>(rcos), static_cast<int>(rsin), z);
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
IntVec3 const IntVec3::GetRotatedAboutZDegrees(float deltaDegrees) const
{
	float length = GetLengthXY();
	float angle = ATan2Degrees(static_cast<float>(y), static_cast<float>(x));
	
	angle += deltaDegrees;

	float rcos = length * CosDegrees(angle);
	float rsin = length * SinDegrees(angle);

	return IntVec3(static_cast<int>(rcos), static_cast<int>(rsin), z);
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
IntVec3 const IntVec3::GetClamped(float maxLength) const
{
	float length = GetLength();
	
	if(length != 0.f)
	{
		if(length > maxLength)
		{
			float scale = maxLength / length;
			Vec3 clampedVec3 =  Vec3(static_cast<float>(x) * scale, static_cast<float>(y) * scale, static_cast<float>(z) * length);
			return IntVec3(clampedVec3);
		}
	}
	
	if(length == 0.f)
	{
		return IntVec3(0, 0, 0);
	}
	
	return IntVec3(x, y, z);
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
IntVec3 const IntVec3::GetNormalized() const
{
	float length = GetLength();
	
	float scale = 1 / length;

	if(length > 0)
	{
		Vec3 normalizedVec3 = Vec3(static_cast<float>(x) * scale, static_cast<float>(y) * scale, static_cast<float>(z) * scale);
		return IntVec3(normalizedVec3);
	}

	return IntVec3(0, 0, 0);
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
IntVec3 IntVec3::GetXY() const
{
	return IntVec3(x, y, 0);
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
IntVec2 IntVec3::GetXY2D() const
{
	return IntVec2(x, y);
}

//------------------------------------------------------------------------------------------------------------------
Vec3 const IntVec3::GetAsVec3() const
{
	return Vec3(static_cast<float>(x), static_cast<float>(y), static_cast<float>(z));
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void IntVec3::SetFromText(char const* textIntVec3, char delimiter)
{
	Strings vec3Strings = SplitStringOnDelimiter(textIntVec3, delimiter);

	x = atoi(vec3Strings[0].c_str());
	y = atoi(vec3Strings[1].c_str());
	z = atoi(vec3Strings[2].c_str());
}
//------------------------------------------------------------------------------------------------------------------
IntVec3 const IntVec3::operator+(IntVec3 const& vecToAdd) const
{
	return IntVec3(x + vecToAdd.x, y + vecToAdd.y, z + vecToAdd.z);
}

//------------------------------------------------------------------------------------------------------------------
IntVec3 const IntVec3::operator-(IntVec3 const& vecToSubtract) const
{
	return IntVec3(x - vecToSubtract.x, y - vecToSubtract.y, z - vecToSubtract.z);
}

//------------------------------------------------------------------------------------------------------------------
bool IntVec3::operator==(IntVec3 const& vecToCompare) const
{
	return x == vecToCompare.x && y == vecToCompare.y && z == vecToCompare.z;
}

//------------------------------------------------------------------------------------------------------------------
bool IntVec3::operator!=(IntVec3 const& vecToCompare) const
{
	return x != vecToCompare.x || y != vecToCompare.y || z != vecToCompare.z;
}

//------------------------------------------------------------------------------------------------------------------
void IntVec3::operator=(IntVec3 const& copyFrom)
{
	x = copyFrom.x;
	y = copyFrom.y;
	z = copyFrom.z;
}
