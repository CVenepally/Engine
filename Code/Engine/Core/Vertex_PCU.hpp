#pragma once
#include "Engine/Math/Vec3.hpp"
#include "Engine/Core/Rgba8.hpp"
#include "Engine/Math/Vec2.hpp"





struct Vertex_PCU
{

public:
	
	Vec3	m_position = Vec3();
	Rgba8	m_color = Rgba8();
	Vec2	m_uvTexCoords = Vec2();

public:
	
	Vertex_PCU() {}

	explicit Vertex_PCU(Vec3 const& position, Rgba8 const& color, Vec2 const& uvTexCoords);
	explicit Vertex_PCU(Vec3 const& position, Rgba8 const& color);

	explicit Vertex_PCU(Vec2 const& position, Rgba8 const& color);
	explicit Vertex_PCU(Vec2 const& position, Rgba8 const& color, Vec2 const& uvTexCoords);

};
