#pragma once
#include "Utils.h"
#include <chrono>
#include <d3d12.h>
#include <vector>

class Benchmarker
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

	void Init(int stabilizationFrameCount, int measurementFrameCount, int numObjectsToRender);

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