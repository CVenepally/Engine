#pragma once

struct Vec3;
struct Vec2;
struct IntVec2;

struct IntVec3
{

    static const IntVec3 ZERO;
    static const IntVec3 ONE;
    static const IntVec3 FORWARD;
    static const IntVec3 BACKWARD;
    static const IntVec3 RIGHT;
    static const IntVec3 LEFT;
    static const IntVec3 UP;
    static const IntVec3 DOWN;

public:

    int x = 0;
    int y = 0;
    int z = 0;

public:

    IntVec3();
    ~IntVec3() = default;

    IntVec3(const IntVec3& copyFrom);

    explicit IntVec3(int initialX, int initialY, int initialZ);
    explicit IntVec3(int initialXYZ);
    explicit IntVec3(Vec3 const& vec3);
    explicit IntVec3(Vec2 const& vec2);
    explicit IntVec3(IntVec2 const& intVec2);
    
    float   GetLength() const;
    float   GetLengthXY() const;
    int     GetLengthSquared() const;
    int     GetLengthXYSquared() const;

    float   GetAngleAboutZRadians() const;
    float   GetAngleAboutZDegrees() const;
    
    IntVec3 const GetRotatedAboutZRadians(float deltaRadians) const;
    IntVec3 const GetRotatedAboutZDegrees(float deltaDegrees) const;
    IntVec3 const GetClamped(float maxLength) const;
    IntVec3 const GetNormalized() const;
    IntVec3 GetXY() const;
    IntVec2 GetXY2D() const;
    
    Vec3 const GetAsVec3() const;

    void SetFromText(char const* textIntVec3, char delimiter = ',');

    IntVec3 const operator+(IntVec3 const& vecToAdd) const;
    IntVec3 const operator-(IntVec3 const& vecToSubtract) const;
	
    bool operator==(IntVec3 const& vecToCompare) const;
    bool operator!=(IntVec3 const& vecToCompare) const;
    void operator=(const IntVec3& copyFrom);
};