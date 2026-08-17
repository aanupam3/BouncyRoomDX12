#include "Benchmarker.h"
#include "Macros.h"
#include "Scene.h"
#include <algorithm>

void Benchmarker::CalculateSteadyStateMetrics()
{
	// Sort for Max, Min, Median
	std::cout << "Cpu frames measured: " << SSData.CpuFrameTimes.size() << "\n";
	std::sort(SSData.CpuFrameTimes.begin(), SSData.CpuFrameTimes.end());

	m_steadyStateCalculatedMetrics.MinCpuFrameTime = SSData.CpuFrameTimes.front();
	m_steadyStateCalculatedMetrics.MaxCpuFrameTime = SSData.CpuFrameTimes.back();
	const std::size_t size = SSData.CpuFrameTimes.size();
	const std::size_t middle = size / 2;

	m_steadyStateCalculatedMetrics.MedianCpuFrameTime =
		size % 2 == 0
		? (SSData.CpuFrameTimes[middle - 1] + SSData.CpuFrameTimes[middle]) / 2.0
		: SSData.CpuFrameTimes[middle];

	std::cout << "Gpu frames measured: " << SSData.GpuFrameTimes.size() << "\n";
	std::sort(SSData.GpuFrameTimes.begin(), SSData.GpuFrameTimes.end());

	//m_steadyStateCalculatedMetrics.MinGpuFrameTime = SSData.GpuFrameTimes.front();
	//m_steadyStateCalculatedMetrics.MaxGpuFrameTime = SSData.GpuFrameTimes.back();

	m_steadyStateCalculatedMetrics.MedianGpuFrameTime =
		size % 2 == 0
		? (SSData.GpuFrameTimes[middle - 1] + SSData.GpuFrameTimes[middle]) / 2.0
		: SSData.GpuFrameTimes[middle];
}

Benchmarker::Benchmarker(int stabilizationFrameCount, int measurementFrameCount)
{
	LastStabilizationFrameNumber = stabilizationFrameCount;
	LastMeasurementFrameNumber = measurementFrameCount + stabilizationFrameCount;

	SSData.CpuFrameTimes.resize(measurementFrameCount);
	SSData.GpuFrameTimes.resize(measurementFrameCount);
	SSData.NumDrawCalls.resize(measurementFrameCount);
	SSData.NumVisibleObjects.resize(measurementFrameCount);
	SSData.NumTrianglesSubmitted.resize(measurementFrameCount);
}


bool Benchmarker::Update(Scene& scene)
{
	return true;
	//IsFirstRender = true;
	//StartTime(LoadingMetricsData.LoadTimeToFirstRenderedFrame);
	////std::cout << "Start Load epoch time for first frame: " << LoadingMetricsData.LoadTimeToFirstRenderedFrame <<"\n";


	//StartTime(LoadingMetricsData.InitTime);

	//StopTime(LoadingMetricsData.InitTime);

	//int overallFrameNumber = 0;

	//if (overallFrameNumber > LastMeasurementFrameNumber)
	//{
	//	Report();
	//}

	//if (overallFrameNumber > LastStabilizationFrameNumber)
	//{
	//	//std::cout << SSData.CpuFrameTimes[measurementFrameNumber] << "\n";
	//	StartTime(SSData.CpuFrameTimes[CurrentMeasurementFrameNumber]);
	//	//std::cout << "Cpu epoch time actual for frame: " << measurementFrameNumber << " is " << SSData.CpuFrameTimes[measurementFrameNumber] << "\n";
	//}


	//if (overallFrameNumber > LastStabilizationFrameNumber)
	//{
	//	StopTime(SSData.CpuFrameTimes[CurrentMeasurementFrameNumber]);
	//	CurrentMeasurementFrameNumber++;
	//	//std::cout << "Cpu ms duration for frame: " << frameCount << " is " << frameMetrics.CpuFrameTime << "\n";
	//}

	//overallFrameNumber++;

	//const float r = 50.0f;
	//float alpha = (PI - m_cameraRevolutionTheta) / 2.0f;
	//float hyp = 2 * r * std::sin(m_cameraRevolutionTheta / 2.0f);

	//float newX = -hyp * sin(alpha);
	//float newY = 50.0f;
	//float newZ = hyp * cos(alpha);

	//const float pitch = DirectX::XMConvertToRadians(45); // X rotation
	//const float yaw = m_cameraRevolutionTheta; // Y rotation
	//const float roll = DirectX::XMConvertToRadians(0); // Z rotation

	//Camera& sceneCamera = scene.GetCamera();
	//const DirectX::XMMATRIX rotation = DirectX::XMMatrixRotationRollPitchYaw(pitch, yaw, roll);
	//const DirectX::XMMATRIX translation = DirectX::XMMatrixTranslation(newX, newY, newZ);
	//Transform newCameraTransform{ rotation * translation };
	//sceneCamera.SetTransform(newCameraTransform);

	//m_cameraRevolutionTheta += m_radiansPerSec;
}


bool Benchmarker::Init(Scene& scene)
{
	// Starting number of objects ----------------------------------------------------------------------------
	//std::cout << "------- Benchmarking Enabled --------------\n";
	//int numStartingObjects = 50; // start here and move up
	//std::cout << "Starting Objects to Render: " << numStartingObjects << "\n";

	//const std::string modelBasePath{ "./models/OakTree/" };
	//scene.AddModel({ modelBasePath, "OakTree" });

	//scene.GetObjects().resize(numStartingObjects);

	//for (int i = 0; i < numStartingObjects; i++)
	//{
	//	float maxX = 120;
	//	float posX = maxX * (std::rand() / (1.0f * RAND_MAX)) * pow(-1, i);

	//	float maxZ = 500;
	//	float posZ = maxZ * (std::rand() / (1.0f * RAND_MAX)) * pow(-1, i);

	//	float posY = -15.0f;
	//	DirectX::XMFLOAT3 position{ posX, posY, posZ };

	//	float rotYRadians = std::rand();
	//	DirectX::XMFLOAT3 rotation{ 0, rotYRadians, 0 };

	//	/*std::cout << "Creating object at: (" << posX << ", " << posY << ", " << posZ << ")\n";
	//	std::cout << "Rotation: (" << rotation.x << ", " << rotation.y << ", " << rotation.z << ")\n";*/
	//	ModelInstance* oakTreeModelInstance = new ModelInstance(oakTreeModel, Transform(position, rotation));


	//	/*Model* oakTreeModel2 = new Model(oakTreeModelBasePath, "OakTree2");
	//	oakTreeModel2->SetWorldPosition(-40.0f, -15.0f, 55.0f);
	//	oakTreeModel2->SetWorldRotationDegrees(-90, 0, 0);
	//	m_models.push_back(oakTreeModel2);

	//	Model* oakTreeModel3 = new Model(oakTreeModelBasePath, "OakTree3");
	//	oakTreeModel3->SetWorldPosition(40.0f, -15.0f, 65.0f);
	//	oakTreeModel3->SetWorldRotationDegrees(-90, 0, 0);
	//	m_models.push_back(oakTreeModel3);*/

	//	/*Model* cubeModel = new Model(cubeModelBasePath, "cube");
	//	cubeModel->SetWorldPosition(0.0f, -10.0f, 50.0f);
	//	cubeModel->SetWorldRotationDegrees(0, 0, 0);
	//	cubeModel->SetWorldScale(1.5f);
	//	m_models.push_back(cubeModel);*/


	//	m_objects.push_back(oakTreeModelInstance);
	//}


	return true;
}

void Benchmarker::Report()
{
	std::cout << "\n----------- Loading Metrics --------------\n";
	std::cout << "Init Duration: " << LoadingMetricsData.InitTime / 1000.0 << "s \n";
	std::cout << "Time till first frame rendered: " << LoadingMetricsData.LoadTimeToFirstRenderedFrame / 1000.0 << "s \n";

	std::cout << "\n----------- Steady State Metrics --------------\n";
	CalculateSteadyStateMetrics();
	std::cout << "Median Cpu Time: " << m_steadyStateCalculatedMetrics.MedianCpuFrameTime << "ms \n";
	std::cout << "Min Cpu Time: " << m_steadyStateCalculatedMetrics.MinCpuFrameTime << "ms \n";
	std::cout << "Max Cpu Time: " << m_steadyStateCalculatedMetrics.MaxCpuFrameTime << "ms \n";
	std::cout << "Median GPU Time: " << m_steadyStateCalculatedMetrics.MedianGpuFrameTime << "ms \n";
}

bool Benchmarker::Shutdown()
{
	return true;
}
