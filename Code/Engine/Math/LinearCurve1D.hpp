#pragma once
#include "Engine/Math/Curve1D.hpp"

class LinearCurve1D : public Curve1D
{
public:

	Curve1D();
	~Curve1D();

	virtual void Evaluate(float t) = 0;
};