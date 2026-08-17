#include "Scene.h"

#pragma once
class ISimulationEngine
{
public:
	virtual bool Init(Scene&) = 0;
	virtual bool Update(Scene&) = 0;
	virtual bool Shutdown() = 0;
};