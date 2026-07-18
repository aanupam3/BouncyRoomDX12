#include "Benchmarker.h"
#include <algorithm>

void Benchmarker::CalculateSteadyStateMetrics()
{
	// Sort for Max, Min, Median
	std::vector<double> sortedCpuTimes
		= Utils::ExtractFromStructList(SteadyStateFrameMetricsData,
			&Benchmarker::SteadyStateFrameMetrics::CpuTime);

	std::cout << "Sorted cpu times count: " << sortedCpuTimes.size() << "\n";
	std::sort(sortedCpuTimes.begin(), sortedCpuTimes.end());

	m_steadyStateCalculatedMetrics.MinCpuTime = sortedCpuTimes.front();
	m_steadyStateCalculatedMetrics.MaxCpuTime = sortedCpuTimes.back();
	const std::size_t size = sortedCpuTimes.size();
	const std::size_t middle = size / 2;

	if (size % 2 == 0)
	{
		m_steadyStateCalculatedMetrics.MedianCpuFrameTime =
			(sortedCpuTimes[middle - 1] + sortedCpuTimes[middle]) / 2.0;
	}
	else
	{
		m_steadyStateCalculatedMetrics.MedianCpuFrameTime = sortedCpuTimes[middle];
	}
}


void Benchmarker::Init(int stabilizationFrameCount, int measurementFrameCount)
{
	StabilizationFrameCount = stabilizationFrameCount;
	MeasurementFrameCount = measurementFrameCount + stabilizationFrameCount;
}

void Benchmarker::Report()
{
	std::cout << "\n----------- Loading Metrics --------------\n";
	std::cout << "Init Duration: " << LoadingMetricsData.InitTime / 1000.0 << "s \n";
	std::cout << "Time till first frame rendered: " << LoadingMetricsData.LoadTimeToFirstRenderedFrame / 1000.0 << "s \n";

	std::cout << "\n----------- Steady State Metrics --------------\n";
	CalculateSteadyStateMetrics();
	std::cout << "Median Cpu Time: " << m_steadyStateCalculatedMetrics.MedianCpuFrameTime << "ms \n";
	std::cout << "Min Cpu Time: " << m_steadyStateCalculatedMetrics.MinCpuTime << "ms \n";
	std::cout << "Max Cpu Time: " << m_steadyStateCalculatedMetrics.MaxCpuTime << "ms \n";
}
