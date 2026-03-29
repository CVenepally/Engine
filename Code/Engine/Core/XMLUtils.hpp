#pragma once
#include "Engine/Core/EngineCommon.hpp"

#include <string>
#include <vector>
//------------------------------------------------------------------------------------------------------------------
typedef std::vector<std::string> Strings;

struct Vec2;
struct Vec3;
struct Vec4;
struct FloatRange;
struct IntRange;
struct IntVec2;
struct Rgba8;
struct EulerAngles;

//------------------------------------------------------------------------------------------------------------------
int			ParseXmlAttribute(XmlElement const& element, char const* attribureName, int defaultValue);
float		ParseXmlAttribute(XmlElement const& element, char const* attribureName, float defaultValue);
char		ParseXmlAttribute(XmlElement const& element, char const* attribureName, char defaultValue);
bool		ParseXmlAttribute(XmlElement const& element, char const* attribureName, bool defaultValue);
Rgba8		ParseXmlAttribute(XmlElement const& element, char const* attribureName, Rgba8 const& defaultValue);
Vec2		ParseXmlAttribute(XmlElement const& element, char const* attribureName, Vec2 const& defaultValue);
Vec3		ParseXmlAttribute(XmlElement const& element, char const* attribureName, Vec3 const& defaultValue);
Vec4		ParseXmlAttribute(XmlElement const& element, char const* attribureName, Vec4 const& defaultValue);
FloatRange	ParseXmlAttribute(XmlElement const& element, char const* attribureName, FloatRange const& defaultValue);
IntRange	ParseXmlAttribute(XmlElement const& element, char const* attribureName, IntRange const& defaultValue);
EulerAngles ParseXmlAttribute(XmlElement const& element, char const* attribureName, EulerAngles const& defaultValue);
IntVec2		ParseXmlAttribute(XmlElement const& element, char const* attribureName, IntVec2 const& defaultValue);
std::string ParseXmlAttribute(XmlElement const& element, char const* attribureName, std::string const& defaultValue);
std::string ParseXmlAttribute(XmlElement const& element, char const* attribureName, char const* defaultValue);
Strings		ParseXmlAttribute(XmlElement const& element, char const* attribureName, Strings defaultValue, char delimiterSplitOn = ',');

void		ParseXmlAttribute(XmlElement const& element, char const* attribureName, std::vector<uint8_t>& out_vector, char delimiterSplitOn = ',');
void		ParseXmlAttribute(XmlElement const& element, char const* attribureName, std::vector<uint16_t>& out_vector, char delimiterSplitOn = ',');
void		ParseXmlAttribute(XmlElement const& element, char const* attribureName, std::vector<int8_t>& out_vector, char delimiterSplitOn = ',');
