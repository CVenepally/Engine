#pragma once
#include "Engine/Math/Vec2.hpp"
#include <vector>
#include <map>

class CubicHermiteCurve2D;
struct Hash2D;

class CubicBezierCurve2D
{
public:

	CubicBezierCurve2D() = default;
	explicit CubicBezierCurve2D(Vec2 startPos, Vec2 guidePos1, Vec2 guidePos2, Vec2 endPos);
	explicit CubicBezierCurve2D(CubicHermiteCurve2D hermite);
	Vec2  EvaluateAtParametric(float parametricZeroToOne) const;
	float GetApproximateLength(int subdivisions = 64) const;
	Vec2  EvaluateAtApproximateDistance(float distanceAlongCurve, int numSubdivisions = 64);

public:

	Vec2 m_startPos;
	Vec2 m_guidePos1;
	Vec2 m_guidePos2;
	Vec2 m_endPos;

};

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
class CubicHermiteCurve2D
{
public:

	CubicHermiteCurve2D() = default;
	explicit CubicHermiteCurve2D(Vec2 start, Vec2 end, Vec2 initialVelocity, Vec2 finalVelocity);
	explicit CubicHermiteCurve2D(CubicBezierCurve2D hermite);
	Vec2  EvaluateAtParametric(float parametricZeroToOne) const;
	float GetApproximateLength(int subdivisions = 64) const;
	Vec2  EvaluateAtApproximateDistance(float distanceAlongCurve, int numSubdivisions = 64);

public:

	Vec2 m_startPoint;
	Vec2 m_endPoint;

	Vec2 m_initialVelocity;
	Vec2 m_finalVelocity;

};

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
class Spline
{
public:
	
	Spline() = default;
	Spline(std::vector<Vec2> positions);

	void GetHermiteCurves(std::vector<CubicHermiteCurve2D>& out_hermiteCurves);

	Vec2  EvaluateAtParametric(float parametricZeroToOne) const;
	float GetApproximateLength(int subdivisions = 64) const;
	Vec2  EvaluateAtApproximateDistance(float distanceAlongCurve, int numSubdivisions = 64);

public:
	
	std::vector<Vec2> m_positions;
	std::vector<Vec2> m_velocities;
	std::vector<CubicHermiteCurve2D> m_hermiteCurves;

};

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
class Curve1D
{
public:
	Curve1D();
	~Curve1D();

	virtual float Evaluate(float t) = 0;
};

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
class LinearCurve1D : public Curve1D
{
public:
	LinearCurve1D();
	explicit LinearCurve1D(float start, float end);
	~LinearCurve1D();

	virtual float	Evaluate(float t) override;

// 	void			operator=(LinearCurve1D const& copyFrom);
	bool			operator==(LinearCurve1D const& otherCurve);

public:
	float m_start	= 0.f;
	float m_end		= 1.f;
};

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
struct Hash2D
{
	std::size_t operator()(LinearCurve1D const& curve) const noexcept
	{
		uint64_t h1 = static_cast<uint64_t>(curve.m_start);
		uint64_t h2 = static_cast<uint64_t>(curve.m_end);
		return static_cast<size_t>((h1 ^ (h2 << 1)) * 0x9e3779b97f4a7c15ull);
	}
};

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
class PieceWiseCurves : public Curve1D
{
public:
	PieceWiseCurves();
	~PieceWiseCurves();

	virtual float	Evaluate(float t) override;
	void			AddCurve(LinearCurve1D const& curve, float startTime);

	float			Evaluate_new(float t);
	void			AddCurve_new(LinearCurve1D const& curve);

public:
	std::map<float, LinearCurve1D> m_subCurves;
	std::vector<LinearCurve1D> m_subCurvesList;
};


//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
float ComputeCubicBezier1D(float a, float b, float c, float d, float t);
float ComputeQuinticBezier1D(float a, float b, float c, float d, float e, float f, float t);

