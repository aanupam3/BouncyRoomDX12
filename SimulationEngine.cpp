#include "Macros.h"
#include "SimulationEngine.h"

SimulationEngine::SimulationEngine()
{
}

SimulationEngine::~SimulationEngine()
{
}

bool SimulationEngine::Init(Scene& scene)
{
	std::vector<ModelInstance>& objects = scene.GetObjects();
	for (ModelInstance& object : objects)
	{
		std::vector<WorldNode>& objectNodes = object.GetNodes();

		for (UINT nodeIndex = 0; nodeIndex < objectNodes.size(); nodeIndex++)
		{
			WorldNode& nodeWithMesh = objectNodes[nodeIndex];
			if (nodeWithMesh.MeshIndex == -1) { continue; }

			nodeWithMesh.WVPMatrixVector.resize(DIRECTXM_VECTOR_SIZE); // size is important here as its used by Render::Init() to create the WVP buffer for this mesh to be uploaded to GPU
		}
	}

	return true;
}

bool SimulationEngine::Update(Scene& scene)
{
	Camera& mainCamera = scene.GetCamera();
	const DirectX::XMMATRIX& cameraWorldMatrix = mainCamera.GetTransform().GetTransformationMatrix();
	const DirectX::XMMATRIX& projectionMatrix = mainCamera.GetProjectionMatrix();
	const DirectX::XMMATRIX viewMatrix{ DirectX::XMMatrixInverse(nullptr, cameraWorldMatrix) };
	const DirectX::XMMATRIX vpMatrix = DirectX::XMMatrixMultiply(viewMatrix, projectionMatrix);

	std::vector<ModelInstance>& objects = scene.GetObjects();
	for (ModelInstance& object : objects)
	{
		std::vector<WorldNode>& objectNodes = object.GetNodes();

		for (UINT nodeIndex = 0; nodeIndex < objectNodes.size(); nodeIndex++)
		{
			WorldNode& nodeWithMesh = objectNodes[nodeIndex];
			if (nodeWithMesh.MeshIndex == -1) { continue; }

			nodeWithMesh.WVPMatrixVector = Utils::xmMatrixToVector(DirectX::XMMatrixMultiply(nodeWithMesh.NodeTransform.GetTransformationMatrix(), vpMatrix));
		}
	}

	return true;
}

bool SimulationEngine::Shutdown()
{
	return true;
}
