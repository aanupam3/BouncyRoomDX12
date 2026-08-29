#include "Macros.h"
#include "SimulationEngine.h"

SimulationEngine::SimulationEngine(std::shared_ptr<Benchmarker> benchmarker) : m_benchmarker(benchmarker)
{
}

SimulationEngine::~SimulationEngine()
{
}

bool SimulationEngine::Init(Scene& scene)
{
	//std::vector<ModelInstance>& objects = scene.GetObjects();
	//for (ModelInstance& object : objects)
	//{
	//	std::vector<Node>& objectNodes = object.GetNodesModelSpace();


	//}

	Camera& mainCamera = scene.GetCamera();
	mainCamera.SetVPMatrix();

	return true;
}

bool SimulationEngine::Update(Scene& scene)
{
	if (scene.state == Scene::RESETTING)
	{
		// Don't do anything while scene is resetting
		return true;
	}
	std::vector<Model>& models = scene.GetModels();


	// ------------------- COLLISION CHECK ---------------------------------


	// ------------------- OBJECT MOVEMENT UPDATE --------------------------


	// ------------------- CAMERA MOVEMENT UPDATE --------------------------
	Camera& mainCamera = scene.GetCamera();
	mainCamera.SetVPMatrix();

	return true;
}

bool SimulationEngine::Shutdown()
{
	return true;
}
