#include "DefaultApplication.h"
#include "Macros.h"

bool DefaultApplication::Init(Scene& scene)
{
	int numObjects = -1;
	std::cout << "Enter number of objects to render: ";
	std::cin >> numObjects;
	std::cout << "\n";

	if (numObjects <= 0)
	{
		std::cout << "Invalid number of objects input, shutting down " << "\n";
		return false;
	}

	const std::string modelBasePath{ "./models/OakTree/" };
	//scene.AddModel({ modelBasePath, "OakTree" });
	scene.GetModels().emplace_back(modelBasePath, "OakTree");
	Model& oakTreeModel = scene.GetModels()[0]; // hardcoding for now

	std::vector<ModelInstance>& objects = scene.GetObjects();
	objects.clear();
	objects.reserve(numObjects);

	for (int i = 0; i < numObjects; i++)
	{
		float maxX = 120;
		float posX = maxX * (std::rand() / (1.0f * RAND_MAX)) * pow(-1, i);

		float maxZ = 500;
		float posZ = maxZ * (std::rand() / (1.0f * RAND_MAX)) * pow(-1, i);

		float posY = -15.0f;
		DirectX::XMFLOAT3 position{ posX, posY, posZ };

		float rotYRadians = 0;// std::rand();
		DirectX::XMFLOAT3 rotation{ 0, rotYRadians, 0 };

		std::cout << "Creating object at: (" << posX << ", " << posY << ", " << posZ << ")\n";
		std::cout << "Rotation: (" << rotation.x << ", " << rotation.y << ", " << rotation.z << ")\n";
		objects.emplace_back(oakTreeModel, Transform(position, rotation));
	}

	return true;
}

bool DefaultApplication::Update(Scene& scene)
{
	const float r = 50.0f;
	float alpha = (PI - m_cameraRevolutionTheta) / 2.0f;
	float hyp = 2 * r * std::sin(m_cameraRevolutionTheta / 2.0f);

	float newX = -hyp * sin(alpha);
	float newY = 50.0f;
	float newZ = hyp * cos(alpha);

	constexpr float pitch = DirectX::XMConvertToRadians(45); // X rotation
	const float yaw = m_cameraRevolutionTheta; // Y rotation
	constexpr float roll = DirectX::XMConvertToRadians(0); // Z rotation

	Camera& sceneCamera = scene.GetCamera();
	const DirectX::XMMATRIX rotation = DirectX::XMMatrixRotationRollPitchYaw(pitch, yaw, roll);
	const DirectX::XMMATRIX translation = DirectX::XMMatrixTranslation(newX, newY, newZ);
	Transform newCameraTransform{ rotation * translation };
	sceneCamera.SetTransform(newCameraTransform);

	m_cameraRevolutionTheta += m_radiansPerSec;
	return true;
}

bool DefaultApplication::Shutdown()
{
	return true;
}


/*Model* oakTreeModel2 = new Model(oakTreeModelBasePath, "OakTree2");
oakTreeModel2->SetWorldPosition(-40.0f, -15.0f, 55.0f);
oakTreeModel2->SetWorldRotationDegrees(-90, 0, 0);
m_models.push_back(oakTreeModel2);

Model* oakTreeModel3 = new Model(oakTreeModelBasePath, "OakTree3");
oakTreeModel3->SetWorldPosition(40.0f, -15.0f, 65.0f);
oakTreeModel3->SetWorldRotationDegrees(-90, 0, 0);
m_models.push_back(oakTreeModel3);*/

/*Model* cubeModel = new Model(cubeModelBasePath, "cube");
cubeModel->SetWorldPosition(0.0f, -10.0f, 50.0f);
cubeModel->SetWorldRotationDegrees(0, 0, 0);
cubeModel->SetWorldScale(1.5f);
m_models.push_back(cubeModel);*/
