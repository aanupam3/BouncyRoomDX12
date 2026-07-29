#include "Benchmarker.h"
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


void Benchmarker::Init(int stabilizationFrameCount, int measurementFrameCount, int numInstancesToRender)
{
	LastStabilizationFrameNumber = stabilizationFrameCount;
	LastMeasurementFrameNumber = measurementFrameCount + stabilizationFrameCount;
	NumInstancesToRender = numInstancesToRender;

	SSData.CpuFrameTimes.resize(measurementFrameCount);
	SSData.GpuFrameTimes.resize(measurementFrameCount);
	SSData.NumDrawCalls.resize(measurementFrameCount);
	SSData.NumVisibleObjects.resize(measurementFrameCount);
	SSData.NumTrianglesSubmitted.resize(measurementFrameCount);
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
