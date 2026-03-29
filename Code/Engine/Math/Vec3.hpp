#pragma once
#include <string>
#include <vector>

struct Vec2;
struct IntVec2;
struct IntVec3;

struct Vec3
{

public: 

	float x = 0.f;
	float y = 0.f;
	float z = 0.f;

	static const Vec3 ZERO;
	static const Vec3 ONE;
	static const Vec3 RIGHT;
	static const Vec3 LEFT;
	static const Vec3 UP;
	static const Vec3 DOWN;
	static const Vec3 FORWARD;
	static const Vec3 BACKWARD;

public:

	~Vec3() {}												                
	Vec3() {}												                
	
	Vec3( Vec3 const& copyFrom );							                
	
	explicit Vec3 (float initialX, float initialY, float initialZ );	
	explicit Vec3 (int initialX, int initialY, int initialZ );	
	explicit Vec3 (int initialXYZ);	
	explicit Vec3 (float initialXYZ);	
	explicit Vec3 (Vec2 const& vec2, float inZ = 0.f );	
	explicit Vec3 (IntVec2 const& intVec2, int inZ = 0);	
	explicit Vec3 (IntVec2 const& intVec2, float inZ = 0.f);	
	explicit Vec3 (IntVec3 const& intVec3);	
	explicit Vec3 (std::vector<float> const& floatArray);
	explicit Vec3 (std::vector<double> const& floatArray);
	explicit Vec3 (float* floatArray);

	static Vec3 const MakeFromPolarRadians(float pitchRadians, float yawRadians,float length = 1.f);
	static Vec3 const MakeFromPolarDegrees(float pitchDegrees, float yawDegrees,float length = 1.f);
	static Vec3 const MakeFromString(char const* textVec3, char delimiter = ',');
	static Vec3 const MakeFromWord(std::string const& vec3Word);

	float GetLength() const;
	float GetLengthXY() const;
	float GetLengthSquared() const;
	float GetLengthXYSquared() const;
	float GetAngleAboutZRadians() const;
	float GetAngleAboutZDegrees() const;
	
	Vec3 const GetRotatedAboutZRadians(float deltaRadians) const;
	Vec3 const GetRotatedAboutZDegrees(float deltaDegrees) const;
	Vec3 const GetClamped(float maxLength) const;
	Vec3 const GetNormalized() const;
	
	Vec3 GetXY() const;
	Vec2 GetXY2D() const;
	IntVec3 GetAsIntVec3() const;
	void GetAsFloatArray(float* out_array) const;
	
	void SetLength(float newLength);

	void Normalize();

	void SetFromText(char const* textVec3, char delimiter = ',');

	bool		operator==( Vec3 const& compare ) const;		
	bool		operator!=( Vec3 const& compare ) const;		
	
	Vec3 const	operator+( Vec3 const& vecToAdd ) const;
	Vec3 const	operator-( Vec3 const& vecToSubtract ) const;	
	Vec3 const	operator-() const;								
	Vec3 const	operator*( float uniformScale ) const;			
	Vec3 const	operator*( Vec3 const& vecToMultiply ) const;	
	Vec3 const	operator/( float inverseScale ) const;			

	
	void		operator+=( Vec3 const& vecToAdd );				
	void		operator-=( Vec3 const& vecToSubtract );		
	void		operator*=( const float uniformScale );			
	void		operator/=( const float uniformDivisor );		
	void		operator=( Vec3 const& copyFrom );				

	friend Vec3 const operator*( float uniformScale, Vec3 const& vecToScale );	
};


