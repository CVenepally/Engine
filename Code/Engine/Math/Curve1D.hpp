#pragma once


class Curve1D
{
public:

	Curve1D();
	~Curve1D();

	virtual void Evaluate(float t) = 0;
};