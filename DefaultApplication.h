#include "IApplication.h"

#pragma once
class DefaultApplication : public IApplication
{
	// Inherited via IApplication
	bool Init(Scene&) override;
	bool Update(Scene&) override;
	bool Shutdown() override;

private:
	float m_cameraRevolutionTheta = 0.0f;
};

