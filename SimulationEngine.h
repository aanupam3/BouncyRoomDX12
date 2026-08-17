#include "ISimulationEngine.h"
#include "Scene.h"

#pragma once
class SimulationEngine : public ISimulationEngine
{
public:
	SimulationEngine();
	~SimulationEngine();

	bool Init(Scene&) override;
	bool Update(Scene&) override;
	bool Shutdown() override;

private:


};

