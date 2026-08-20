#pragma once
#include "IApplication.h"
#include "Utils.h"
#include <chrono>
#include <d3d12.h>
#include <vector>

class Benchmarker
{
public:
	Benchmarker() = default;
	Benchmarker(int stabilizationFrameCount, int measurementFrameCount);
	~Benchmarker() = default;

	void Reset();
	void Report();

	struct LoadingMetrics
	{
		double InitTime;
		double LoadTimeToFirstRenderedFrame;
		double GlTFParsingTime;
		double GPUResourceCreationPlusUploadTime;
		double ShaderCompilationTime;
	} LoadingMetricsData{};

	struct SteadyStateFrameMetrics
	{
		std::vector<double> CpuFrameTimes;
		std::vector<double> GpuFrameTimes;
		std::vector<int> NumDrawCalls;
		std::vector<int> NumVisibleObjects;
		std::vector<int> NumTrianglesSubmitted;
	} SSData{};

	struct SteadyStateCalculatedMetrics
	{
		double MedianCpuFrameTime{};
		double P95CpuFrameTime{};
		double P99CpuFrameTime{};
		double MedianGpuFrameTime{};
		double P95GpuFrameTime{};
		double P99GpuFrameTime{};
		int MedianDrawCalls{};
		int FramesAbove16ms{};
		float AvgVisibleObjects{};

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

	} SSCalculatedMetrics{};

	int TotalWarmupFramesCount{};
	int TotalMeasurementFramesCount{};

	UINT64 GpuTimestampFrequency{};
	ComPtr<ID3D12QueryHeap> TimestampQueryHeap{};
	ComPtr<ID3D12Resource> TimestampDataResource{};

	int CurrentWarmupFrameCount = 0;
	int CurrentMeasurementFrameCount = 0;
	bool IsFirstRender{};

	// --------------- Static functions -------------------------------------------------------

	// Gets and stores the current time since epoch in milliseconds, into the passed metric
	static void StartTime(double& metric) { metric = Utils::GetCurrentTimeMs(); }

	// Calculates and stores the time duration for the metric, in the metric
	static void StopTime(double& metric) { metric = Utils::GetCurrentTimeMs() - metric; }

private:


	void CalculateSteadyStateMetrics();
	void CalculateLoadingMetrics();

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
};