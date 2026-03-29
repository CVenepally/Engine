#pragma once

struct KeyButtonState
{

public:
	bool m_isPressed = false;
	bool m_wasPressedLastFrame = false;
	float m_holdSeconds = 0.f;

};