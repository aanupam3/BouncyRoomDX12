#include "DefaultApplication.h"
#include "Macros.h"

bool DefaultApplication::Init(Scene& scene)
{
	int numBalls = -1;
	std::cout << "\nEnter number of objects to render: ";
	std::cin >> numBalls;
	std::cout << "\n";

	if (numBalls <= 0)
	{
		std::cout << "Invalid number of objects input, shutting down " << "\n";
		return false;
	}

	const std::string roomBasePath{ "./models/simple_room/" };
	const std::string objectBasePath{ "./models/tennis_ball/" };

	std::vector<Model>& models = scene.GetModels();
	models.clear();
	models.reserve(2);
	models.emplace_back(roomBasePath, "SimpleRoom");
	models.emplace_back(objectBasePath, "TennisBall");

	std::vector<ModelInstance>& objects = scene.GetObjects();
	objects.clear();
	objects.reserve(numBalls + 1);

	// Room
	DirectX::XMFLOAT3 position{ 0, 0, 0 };
	DirectX::XMFLOAT3 rotation{ 0, 0, 0 };
	DirectX::XMFLOAT3 scale{ 1, 1, 1 };
	Model& roomModel = models[0]; // hardcording for now
	objects.emplace_back(roomModel, Transform(position, rotation, scale));

	// Balls
	Model& ballModel = models[1]; // hardcoding for now
	objects.emplace_back(ballModel, Transform(position, rotation));
	for (int i = 0; i < numBalls; i++)
	{
		float maxX = 40;
		float posX = maxX * (std::rand() / (1.0f * RAND_MAX)) * pow(-1, i);

		float maxZ = -10;
		float posZ = maxZ * (std::rand() / (1.0f * RAND_MAX)) * pow(-1, i);

		float maxY = 20;
		float posY = maxY * (std::rand() / (1.0f * RAND_MAX));
		DirectX::XMFLOAT3 position{ posX, posY, posZ };

		float rotYRadians = 0;// std::rand();
		DirectX::XMFLOAT3 rotation{ 0, rotYRadians, 0 };

		/*std::cout << "Creating object at: (" << posX << ", " << posY << ", " << posZ << ")\n";
		std::cout << "Rotation: (" << rotation.x << ", " << rotation.y << ", " << rotation.z << ")\n";*/
		objects.emplace_back(ballModel, Transform(position, rotation));
	}

	return true;
}

bool DefaultApplication::Update(Scene& scene)
{
	constexpr float radiansPerSec = -0.001f;
	constexpr float r = 28.0f;

	float alpha = (PI - m_cameraRevolutionTheta) / 2.0f;
	float hyp = 2 * r * std::sin(m_cameraRevolutionTheta / 2.0f);

	float newX = -hyp * sin(alpha);
	constexpr float newY = 10.0f;
	float newZ = hyp * cos(alpha) - 24;

	constexpr float pitch = DirectX::XMConvertToRadians(0);// DirectX::XMConvertToRadians(45); // X rotation
	const float yaw = m_cameraRevolutionTheta; // Y rotation
	constexpr float roll = DirectX::XMConvertToRadians(0); // Z rotation

	Camera& sceneCamera = scene.GetCamera();
	const DirectX::XMMATRIX rotation = DirectX::XMMatrixRotationRollPitchYaw(pitch, yaw, roll);
	const DirectX::XMMATRIX translation = DirectX::XMMatrixTranslation(newX, newY, newZ);
	Transform newCameraTransform{ rotation * translation };
	sceneCamera.SetTransform(newCameraTransform);

	m_cameraRevolutionTheta += radiansPerSec;
	return true;
}

bool DefaultApplication::Shutdown()
{
	return true;
}