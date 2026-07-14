#pragma once
#include "Utils.h"
#include <chrono>
#include <vector>

class Benchmarker
{

private:
	struct SteadyStateCalculatedMetrics
	{
		double MaxCpuTime;
		double MinCpuTime;
		double MedianCpuFrameTime;
		double P95CpuFrameTime;
		double MedianGpuFrameTime;
		double P95GpuFrameTime;
		int FramesAbove16ms;
		float AvgVisibleObjects;
		float AvgDrawCalls;
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
	struct LoadingMetrics
	{
		double InitTime;
		double LoadTimeToFirstRenderedFrame;
		double GlTFParsingTime;
		double GPUResourceCreationPlusUploadTime;
		double ShaderCompilationTime;
	} LoadingMetricsData;

	bool IsFirstRender{};

	struct SteadyStateFrameMetrics
	{
		double CpuTime;
		double GpuTime;
		int NumVisibleObjects;
		int NumDrawCalls;
	};

	std::vector<SteadyStateFrameMetrics> SteadyStateFrameMetricsData{};

	int StabilizationFrameCount{};
	int MeasurementFrameCount{};

	void Init(int stabilizationFrameCount, int measurementFrameCount);

	void Report();

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