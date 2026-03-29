#pragma once
#include "Engine/Math/Vec3.hpp"
#include "Engine/Math/Vec4.hpp"
#include "Engine/Core/Rgba8.hpp"

#include "ThirdParty/tinyXML2/tinyxml2.h"

typedef tinyxml2::XMLElement	XmlElement;
//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
enum class LightType
{
	NONE = -1,

	DIRECTIONAL,
	POINT,
	SPOT
};

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
// #ToDo: This will completely break Doomenstein! Go back and fix it!! 
struct Light
{
public:
	
	int			m_lightType		= static_cast<int>(LightType::NONE);	
	Vec3		m_position		= Vec3::ZERO;

	Vec3		m_direction		= Vec3::ZERO;
	float		m_padding0		= 0.f;

	Vec4		m_color			= Vec4::ONE; // alpha is intensity

	float		m_innerRadius	= 0.f;
	float		m_outerRadius	= 0.f;
	float		m_innerDot		= -1.f;
	float		m_outerDot		= -2.f;

public:
	Light() = default;
	~Light() = default;

	// create
	static Light CreateDirectionalLight(Vec3 const& direction, float intensity, Rgba8 const& color = Rgba8::WHITE);
	static Light CreatePointLight(Vec3 const& position, float innerRadius, float outerRadius, float intensity, Rgba8 const& color = Rgba8::WHITE);
	static Light CreateSpotLight(Vec3 const& position, Vec3 const& direction, float innerRadius, float outerRadius, float innerDot, float outerDot, float intensity, Rgba8 const& color = Rgba8::WHITE);
	static Light CreateFromXML(XmlElement const& lightElement);
	
	// sets
	void SetLightType(LightType newType);
	void SetPosition(Vec3 const& newPosition);
	void SetDirection(Vec3 const& newDirection);
	void SetColor(Rgba8 const& newColor, bool changeIntensity = false);
	void SetColor(float r, float g, float b);
	void SetIntensity(float newIntesity);
	void SetSpotInnerDot(float innerDot);
	void SetSpotOuterDot(float outerDot);
	void SetFallOffStartRadius(float innerRadius);
	void SetFallOffEndRadius(float outerRadius);

	// gets
	LightType	GetLightType() const;
	Vec3		GetPosition() const;
	Vec3		GetDirection() const;
	float		GetSpotInnerDot() const;
	float		GetSpotOuterDot() const;
	float		GetFallOffStartRadius() const;
	float		GetFallOffEndRadius() const;
	float		GetIntesity() const;
	Vec4		GetColor() const;
	Rgba8		GetColorAsRGBA() const;
};