#pragma once
#include "IApplication.h"
#include "Utils.h"
#include <chrono>
#include <d3d12.h>
#include <vector>

class Benchmarker : public IApplication
{

private:
	struct SteadyStateCalculatedMetrics
	{
		double MaxCpuFrameTime;
		double MinCpuFrameTime;
		double MedianCpuFrameTime;
		double P95CpuFrameTime;
		double P99CpuFrameTime;
		double MedianGpuFrameTime;
		double P95GpuFrameTime;
		int FramesAbove16ms;
		float AvgVisibleObjects;
		float AvgDrawCalls;

		/*AvgCpuFrameTime
		MedianCpuFrameTime
		P95CpuFrameTime
		P99CpuFrameTime

		AvgGpuFrameTime
		MedianGpuFrameTime
		P95GpuFrameTime
		P99GpuFrameTime

		TotalFramesMeasured
		FramesBelow60FPS
		FrameBudgetMissRate

		AvgRenderedObjects
		AvgVisibleObjects
		AvgDrawCalls
		AvgTrianglesRendered
		AvgInstancesRendered*/

	} m_steadyStateCalculatedMetrics;

	enum MeasuringState
	{
		DISABLED,
		MEASURING_LOADING,
		MEASURING_STEADY_STATE,
		FINISHED,
	} m_measuringState;


	void CalculateSteadyStateMetrics();
	void CalculateLoadingMetrics();


public:
	Benchmarker(int stabilizationFrameCount, int measurementFrameCount);
	~Benchmarker() { Shutdown(); }

	struct LoadingMetrics
	{
		double InitTime;
		double LoadTimeToFirstRenderedFrame;
		double GlTFParsingTime;
		double GPUResourceCreationPlusUploadTime;
		double ShaderCompilationTime;
	} LoadingMetricsData;

	struct SteadyStateFrameMetrics
	{
		std::vector<double> CpuFrameTimes;
		std::vector<double> GpuFrameTimes;
		std::vector<int> NumDrawCalls;
		std::vector<int> NumVisibleObjects;
		std::vector<int> NumTrianglesSubmitted;
	};

	SteadyStateFrameMetrics SSData{};
	int LastStabilizationFrameNumber{};
	int LastMeasurementFrameNumber{};
	int NumInstancesToRender{};
	UINT64 GpuTimestampFrequency{};

	ID3D12QueryHeap* TimestampQueryHeap{};
	ID3D12Resource* TimestampDataResource{};

	int CurrentMeasurementFrameNumber = 0;
	bool IsFirstRender{};
	bool IsRunning = false;

	float m_cameraRevolutionTheta = 0;
	const float m_radiansPerSec = -0.001f;

	bool Init(Scene&) override;
	bool Update(Scene&) override;
	void Report();
	bool Shutdown() override;

	// --------------- Static functions -------------------------------------------------------

	// Gets and stores the current time since epoch in milliseconds, into the passed metric
	static void StartTime(double& metric) { metric = Utils::GetCurrentTimeMs(); }

	// Calculates and stores the time duration for the metric, in the metric
	static void StopTime(double& metric) { metric = Utils::GetCurrentTimeMs() - metric; }
};


/*  Loading Phase Metrics:
	Process startup to first window
	Startup to first rendered frame
	Startup to first fully populated frame
	Asset discovery time
	File I/O time
	glTF parsing time
	Image decode time
	Mesh processing time
	GPU-resource creation time
	Upload time
	Pipeline-state creation time
	Shader compilation time
	Descriptor creation time
*/