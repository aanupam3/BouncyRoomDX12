#include "Benchmarker.h"
#include "ISimulationEngine.h"
#include "Scene.h"

#pragma once
class SimulationEngine : public ISimulationEngine
{
public:
	SimulationEngine(std::shared_ptr<Benchmarker> benchmarker = nullptr);
	~SimulationEngine();

	bool Init(Scene&) override;
	bool Update(Scene&) override;
	bool Shutdown() override;

private:
	std::shared_ptr<Benchmarker> m_benchmarker;

};

