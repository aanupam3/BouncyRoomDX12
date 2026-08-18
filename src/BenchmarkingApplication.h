#include "Benchmarker.h"
#include "IApplication.h"

#pragma once
class BenchmarkingApplication : public IApplication
{
public:
	BenchmarkingApplication(std::shared_ptr<Benchmarker> benchmarker);
	bool Init(Scene&) override;
	bool Update(Scene&) override;
	bool Shutdown() override;

	int CalculateNewNumObjects();
	void SetScene(Scene& scene, int numBalls);
	bool CheckBenchmarkPass();

	void MoveCamera(Camera& camera);

private:
	const float kPassingFrameTime = 16.67;
	const float kPassingObjectCountRangePercentage = 0.1f;

	std::shared_ptr<Benchmarker> m_benchmarker;
	float m_cameraRevolutionTheta = 0.0f;
	float m_currentNumberOfBalls{};
	bool m_failedFirstTime = false;
	bool m_isAbove60Fps{};
	int m_upperBoundObjectCount{ INT_MAX };
	int m_lowerBoundObjectCount{};
};

