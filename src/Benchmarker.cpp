#include "Benchmarker.h"
#include "Macros.h"
#include "Scene.h"
#include <algorithm>

void Benchmarker::CalculateSteadyStateMetrics()
{
	// Sort for Max, Min, Median
	std::cout << "Total frames measured: " << SSData.CpuFrameTimes.size() << "\n";
	const std::size_t size = SSData.CpuFrameTimes.size();
	const std::size_t middle = size / 2;
	const std::size_t p95Index = static_cast<std::size_t>(std::ceil(0.95 * size)) - 1;
	const std::size_t p99Index = static_cast<std::size_t>(std::ceil(0.99 * size)) - 1;

	// CPU frame times --------------------------------------------------------
	std::sort(SSData.CpuFrameTimes.begin(), SSData.CpuFrameTimes.end());
	SSCalculatedMetrics.MedianCpuFrameTime =
		size % 2 == 0
		? (SSData.CpuFrameTimes[middle - 1] + SSData.CpuFrameTimes[middle]) / 2.0
		: SSData.CpuFrameTimes[middle];
	SSCalculatedMetrics.P95CpuFrameTime = SSData.CpuFrameTimes[p95Index];
	SSCalculatedMetrics.P99CpuFrameTime = SSData.CpuFrameTimes[p99Index];

	// GPU frame times --------------------------------------------------------
	std::sort(SSData.GpuFrameTimes.begin(), SSData.GpuFrameTimes.end());
	SSCalculatedMetrics.MedianGpuFrameTime =
		size % 2 == 0
		? (SSData.GpuFrameTimes[middle - 1] + SSData.GpuFrameTimes[middle]) / 2.0
		: SSData.GpuFrameTimes[middle];
	SSCalculatedMetrics.P95GpuFrameTime = SSData.GpuFrameTimes[p95Index];
	SSCalculatedMetrics.P99GpuFrameTime = SSData.GpuFrameTimes[p99Index];

	// Draw Calls --------------------------------------------------------
	std::sort(SSData.NumDrawCalls.begin(), SSData.NumDrawCalls.end());
	SSCalculatedMetrics.MedianDrawCalls =
		size % 2 == 0
		? (SSData.NumDrawCalls[middle - 1] + SSData.NumDrawCalls[middle]) / 2.0f
		: SSData.NumDrawCalls[middle];
}

Benchmarker::Benchmarker(int stabilizationFrameCount, int measurementFrameCount)
{
	TotalWarmupFramesCount = stabilizationFrameCount;
	TotalMeasurementFramesCount = measurementFrameCount;

	Reset();
}

void Benchmarker::Reset()
{
	SSCalculatedMetrics = {};
	CurrentWarmupFrameCount = 0;
	CurrentMeasurementFrameCount = 0;

	SSData.CpuFrameTimes.clear();
	SSData.CpuFrameTimes.resize(TotalMeasurementFramesCount);

	SSData.GpuFrameTimes.clear();
	SSData.GpuFrameTimes.resize(TotalMeasurementFramesCount);

	SSData.NumDrawCalls.clear();
	SSData.NumDrawCalls.resize(TotalMeasurementFramesCount);

	SSData.NumVisibleObjects.clear();
	SSData.NumVisibleObjects.resize(TotalMeasurementFramesCount);

	SSData.NumVisibleObjects.clear();
	SSData.NumTrianglesSubmitted.resize(TotalMeasurementFramesCount);
}

void Benchmarker::Report()
{
	//std::cout << "\n----------- Loading Metrics --------------\n";
	//std::cout << "Init Duration: " << LoadingMetricsData.InitTime / 1000.0 << "s \n";
	//std::cout << "Time till first frame rendered: " << LoadingMetricsData.LoadTimeToFirstRenderedFrame / 1000.0 << "s \n";
	//std::cout << "Steady State Metrics\n";

	CalculateSteadyStateMetrics();
	std::cout << "Median Draw Calls: " << SSCalculatedMetrics.MedianDrawCalls << "\n";
	std::cout << "Median CPU Time: " << SSCalculatedMetrics.MedianCpuFrameTime << "ms \n";
	std::cout << "P95 CPU Time: " << SSCalculatedMetrics.P95CpuFrameTime << "ms \n";
	std::cout << "P99 CPU Time: " << SSCalculatedMetrics.P99CpuFrameTime << "ms \n";
	std::cout << "Median GPU Time: " << SSCalculatedMetrics.MedianGpuFrameTime << "ms \n";
	std::cout << "P95 GPU Time: " << SSCalculatedMetrics.P95GpuFrameTime << "ms \n";
	std::cout << "P99 GPU Time: " << SSCalculatedMetrics.P99GpuFrameTime << "ms \n";
}
