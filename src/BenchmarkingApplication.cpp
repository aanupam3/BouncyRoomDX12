#include "BenchmarkingApplication.h"
#include "Macros.h"

BenchmarkingApplication::BenchmarkingApplication(std::shared_ptr<Benchmarker> benchmarker) : m_benchmarker(benchmarker)
{
}

bool BenchmarkingApplication::Init(Scene& scene)
{
	int numBalls = -1;
	std::cout << "\nEnter number of objects to start with: ";
	std::cin >> m_currentNumberOfBalls;
	std::cout << "\n";

	if (m_currentNumberOfBalls <= 0)
	{
		std::cout << "Invalid number of objects input, shutting down " << "\n";
		return false;
	}

	const std::string roomBasePath{ "./models/simple_room/" };
	const std::string objectBasePath{ "./models/tennis_ball/" };

	scene.LightPosition = { 0, 25, 0 };

	std::vector<Model>& models = scene.GetModels();
	models.clear();
	models.reserve(2);
	models.emplace_back(roomBasePath, "SimpleRoom");
	models.emplace_back(objectBasePath, "TennisBall");

	SetScene(scene, m_currentNumberOfBalls);
	scene.state = Scene::RUNNING;

	//m_benchmarker->IsFirstRender = true;
	//m_benchmarker->StartTime(m_benchmarker->LoadingMetricsData.LoadTimeToFirstRenderedFrame);
	////std::cout << "Start Load epoch time for first frame: " << m_benchmarker->LoadingMetricsData.LoadTimeToFirstRenderedFrame <<"\n";
	//StartTime(m_benchmarker->LoadingMetricsData.InitTime)
	//StopTime(m_benchmarker->LoadingMetricsData.InitTime);
	return true;
}

bool BenchmarkingApplication::Update(Scene& scene)
{
	if (scene.state == Scene::READY)
	{
		SetScene(scene, CalculateNewNumObjects());

		std::cout << "\n--------- Running Updated Scene ------------\n";
	}

	// after stablization, stop at the beginning of update to track the last full CPU frame time
	if (m_benchmarker->CurrentWarmupFrameCount >= m_benchmarker->TotalWarmupFramesCount)
	{
		m_benchmarker->StopTime(m_benchmarker->SSData.CpuFrameTimes[m_benchmarker->CurrentMeasurementFrameCount]);
		m_benchmarker->CurrentMeasurementFrameCount++;
		//std::cout << "CPU time measured: " << m_benchmarker->SSData.CpuFrameTimes[m_benchmarker->CurrentMeasurementFrameCount] << "\n";
	}

	m_benchmarker->CurrentWarmupFrameCount++;

	if (m_benchmarker->CurrentMeasurementFrameCount >= m_benchmarker->TotalMeasurementFramesCount)
	{
		std::cout << "- Number of objects: " << m_currentNumberOfBalls << "\n";
		m_benchmarker->Report();

		// If all benchmark passes (including UpperBound - LowerBound < 1%) then we can stop
		if (CheckBenchmarkPass())
		{
			int maxNumObjectsToPass = (m_upperBoundObjectCount + m_lowerBoundObjectCount) / 2;
			std::cout << "\n======= Final Result =============\n";
			std::cout << "Max Number of Objects to Pass = " << maxNumObjectsToPass << "\n";
			return false;
		}

		// Otherwise, let the redering engine clean up the scene (wait for command queue to empty out)
		// Once rendering engine is done, it should set the scene state back to READY
		scene.state = Scene::RESETTING;
		return true;
	}

	MoveCamera(scene.GetCamera());

	if (m_benchmarker->CurrentWarmupFrameCount >= m_benchmarker->TotalWarmupFramesCount)
	{
		m_benchmarker->StartTime(m_benchmarker->SSData.CpuFrameTimes[m_benchmarker->CurrentMeasurementFrameCount]);
	}

	return true;
}


int BenchmarkingApplication::CalculateNewNumObjects()
{
	if (m_isAbove60Fps)
	{
		m_lowerBoundObjectCount = m_currentNumberOfBalls;

		if (!m_failedFirstTime) // Keep doubling until we fail the first time
		{
			m_currentNumberOfBalls *= 2;
			std::cout << "\n" << m_lowerBoundObjectCount << " balls PASS, now testing double: " << m_currentNumberOfBalls << "\n";
		}
		else
		{
			m_currentNumberOfBalls = (m_lowerBoundObjectCount + m_upperBoundObjectCount) / 2;
			std::cout << "\n" << m_lowerBoundObjectCount << " balls PASS, now testing midpoint: " << m_currentNumberOfBalls << "\n";
		}
	}
	else // failed
	{
		if (m_failedFirstTime == false) { m_failedFirstTime = true; }

		m_upperBoundObjectCount = m_currentNumberOfBalls;
		m_currentNumberOfBalls = (m_lowerBoundObjectCount + m_upperBoundObjectCount) / 2;

		std::cout << "\n" << m_upperBoundObjectCount << " balls FAIL, now testing midpoint: " << m_currentNumberOfBalls << "\n";
	}
	return m_currentNumberOfBalls;
}


bool BenchmarkingApplication::CheckBenchmarkPass()
{
	// CPU frame time <= 16.67 ms
	if (m_benchmarker->SSCalculatedMetrics.MedianCpuFrameTime <= kPassingFrameTime)
	{
		m_isAbove60Fps = true;

		// (UpperBound - LowerBound)/LowerBound < 1%
		float objectCountRange = (m_upperBoundObjectCount - m_lowerBoundObjectCount) / (1.0f * m_lowerBoundObjectCount);
		if (objectCountRange < kPassingObjectCountRangePercentage) { return true; }
	}
	else
	{
		m_isAbove60Fps = false;
	}

	return false;
}

void BenchmarkingApplication::SetScene(Scene& scene, int numBalls)
{
	m_benchmarker->Reset();
	m_cameraRevolutionTheta = 0;

	std::vector<Model>& models = scene.GetModels();

	// Room
	Transform roomModelInstanceTransform{};
	Model& roomModel = models[0]; // hardcording for now
	roomModel.WorldRootTransformBuffersAllInstances.clear();
	scene.AddModelInstance(roomModel, roomModelInstanceTransform);

	// Balls
	Model& ballModel = models[1]; // hardcoding for now
	ballModel.WorldRootTransformBuffersAllInstances.clear();

	std::vector<std::array<float, MATRIX4X4_NUMELEMENTS>> ballTransformBuffers{ static_cast<UINT>(numBalls) };
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

		Transform ballTransform{ position, rotation };
		/*std::cout << "Creating object at: (" << posX << ", " << posY << ", " << posZ << ")\n";
		std::cout << "Rotation: (" << rotation.x << ", " << rotation.y << ", " << rotation.z << ")\n";*/
		ballTransformBuffers[i] = ballTransform.GetTransformMatrixArray();
	}
	scene.AddModelInstances(ballModel, ballTransformBuffers);
}

void BenchmarkingApplication::MoveCamera(Camera& sceneCamera)
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

	const DirectX::XMMATRIX rotation = DirectX::XMMatrixRotationRollPitchYaw(pitch, yaw, roll);
	const DirectX::XMMATRIX translation = DirectX::XMMatrixTranslation(newX, newY, newZ);
	Transform newCameraTransform{ rotation * translation };
	sceneCamera.SetTransform(newCameraTransform);

	m_cameraRevolutionTheta += radiansPerSec;
}

bool BenchmarkingApplication::Shutdown()
{
	return true;
}