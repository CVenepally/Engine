#include "Engine/Renderer/Light.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Core/XMLUtils.hpp"
#include <string>


//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
Light Light::CreateDirectionalLight(Vec3 const& direction, float intensity = 0.5f, Rgba8 const& color)
{
    Light directionalLight;
    directionalLight.m_lightType = static_cast<int>(LightType::DIRECTIONAL);
    directionalLight.m_color     = color.GetAsVec4();
    directionalLight.m_color.w   = intensity;
    directionalLight.m_direction = direction;

    return directionalLight;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
Light Light::CreatePointLight(Vec3 const& position, float innerRadius, float outerRadius, float intensity, Rgba8 const& color)
{
	Light pointLight;
	pointLight.m_lightType   = static_cast<int>(LightType::POINT);
	pointLight.m_color       = color.GetAsVec4();
    pointLight.m_color.w     = intensity;
	pointLight.m_position    = position;
    pointLight.m_innerRadius = innerRadius;
    pointLight.m_outerRadius = outerRadius;
	
    return pointLight;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
Light Light::CreateSpotLight(Vec3 const& position, Vec3 const& direction, float innerRadius, float outerRadius, float innerDot, float outerDot, float intensity, Rgba8 const& color)
{
	Light spotLight;
	spotLight.m_lightType   = static_cast<int>(LightType::SPOT);
	spotLight.m_color       = color.GetAsVec4();
	spotLight.m_color.w     = intensity;
	spotLight.m_position    = position;
	spotLight.m_direction   = direction;
	spotLight.m_innerRadius = innerRadius;
	spotLight.m_outerRadius = outerRadius;
    spotLight.m_innerDot    = innerDot;
	spotLight.m_outerDot    = outerDot;

	return spotLight;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
Light Light::CreateFromXML(XmlElement const& lightElement)
{
    UNUSED(lightElement);
    return Light();
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void Light::SetLightType(LightType newType)
{
    m_lightType = static_cast<int>(newType);
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void Light::SetPosition(Vec3 const& newPosition)
{
    m_position = newPosition;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void Light::SetDirection(Vec3 const& newDirection)
{
    m_direction = newDirection;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void Light::SetColor(Rgba8 const& newColor, bool changeIntensity)
{
    Vec4 newColorAsVec4 = newColor.GetAsVec4();
    m_color.x = newColorAsVec4.x;
    m_color.y = newColorAsVec4.y;
    m_color.z = newColorAsVec4.z;
    
    if(changeIntensity)
    {
        m_color.z = newColorAsVec4.w;
    }
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void Light::SetColor(float r, float g, float b)
{
	m_color.x = r;
	m_color.y = g;
	m_color.z = b;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void Light::SetIntensity(float newIntesity)
{
    m_color.w = newIntesity;
   // m_color.w = GetClampedZeroToOne(m_color.w);
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void Light::SetSpotInnerDot(float innerDot)
{
    m_innerDot = innerDot;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void Light::SetSpotOuterDot(float outerDot)
{
    m_outerDot = outerDot;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void Light::SetFallOffStartRadius(float innerRadius)
{
    m_innerRadius = innerRadius;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void Light::SetFallOffEndRadius(float outerRadius)
{
    m_outerRadius = outerRadius;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
LightType Light::GetLightType() const
{
    return static_cast<LightType>(m_lightType);
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
Vec3 Light::GetPosition() const
{
    return m_position;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
Vec3 Light::GetDirection() const
{
    return m_direction;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
float Light::GetSpotInnerDot() const
{
    return m_innerDot;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
float Light::GetSpotOuterDot() const
{
    return m_outerDot;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
float Light::GetFallOffStartRadius() const
{
    return m_innerRadius;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
float Light::GetFallOffEndRadius() const
{
    return m_outerRadius;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
float Light::GetIntesity() const
{
    return m_color.w;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
Vec4 Light::GetColor() const
{
    return m_color;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
Rgba8 Light::GetColorAsRGBA() const
{
    Rgba8 color;
    color.r = static_cast<uchar>(Lerp(0.f, 255.f, m_color.x));
    color.g = static_cast<uchar>(Lerp(0.f, 255.f, m_color.y));
    color.b = static_cast<uchar>(Lerp(0.f, 255.f, m_color.z));
    color.a = static_cast<uchar>(Lerp(0.f, 255.f, m_color.w));

    return color;
}
