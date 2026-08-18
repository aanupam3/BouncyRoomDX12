# BouncyRoomDX12
A low-level graphics+physics engine project with the goal of incrementally increasing the maximum number of bouncy objects in a room, while maintaining 60fps at stable state.

See [[#Hardware]] and [[#Scene]] sections below for current setup.
# Recent Performance
## Latest Commit
- Max Bouncy Objects Rendered @ 60 FPS: {num}
- Avg Triangles per Object: {num}
- Avg Triangles per Frame: {num}
- Draw Calls per Frame: {num}
- Commit: 
- PrimaryBottleneck: CPU | GPU | Mixed

### Video
(Coming Soon)

## Previous Commit
- Max Bouncy Objects Rendered @ 60 FPS: {num}
- Triangles per Object: {num}
- Triangles per Frame: {num}
- Draw Calls per Frame: {num}
- Commit: 
- PrimaryBottleneck: CPU | GPU | Mixed

See [[#History (latest to oldest)]] for max objects in past commits
# Current Metrics when Rendering Max Objects at 60fps
AvgCpuFrameTime
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
AvgInstancesRendered
# Scene
**Resolution**: 1920 x 1080
**Camera**: Perspective, fixed path revolution
**Lights**: NONE
**Shadows**: NONE
**Physics**: NONE
**VSync**: OFF
**Present mode**: Windowed
**Build configuration**: RELEASE + DX12 Debug mode off

## Models
| Model    | Num. Instances | Vertices | Triangles | Textures (Resolution) |
| -------- | -------------- | -------- | --------- | --------------------- |
| Oak Tree |                |          |           |                       |
|          |                |          |           |                       |
| Total    |                |          |           |                       |

# Current Test Sequence

1. **Warm up**
   - Run 300 frames with `x` objects.

2. **Exponential Search**
   - Double the number of objects after each passing benchmark.
   - Continue until the first failing object count is found.
   - Record:
	   - `LowerBound` = highest passing object count
	   - `UpperBound` = lowest failing object count

3. **Measurement**
   - After changing object count, ignore the first 3 seconds.
   - Wait until the rolling average CPU frame time over the most recent 100 frames stabilizes within ±1 ms.
   - Measure the next `N` frames.

   A benchmark passes when:
   - Median CPU frame time < 16 ms
   - Median GPU frame time < 16 ms
   - P95 CPU frame time < 16.67 ms
   - P95 GPU frame time < 16.67 ms
   - Frame-budget miss rate < 1%

4. **Binary Search**
   - Test the midpoint between `LowerBound` and `UpperBound`.
   - If it passes:
	   - Set `LowerBound = midpoint`
   - If it fails:
	   - Set `UpperBound = midpoint`
   - Repeat until `(UpperBound - LowerBound) / LowerBound <= 1%`
	   - Save [[#Current Metrics when Rendering Max Objects at 60fps]]

# Current Code Flow Architecture
[!Basic Architecture](docs/images/BasicArchitecture.png)

# Hardware
## CPU
- **OS**: Windows 11 Home
- **Model**: 12th Gen Intel(R) Core(TM) i7-1255U 
- **Physical** Cores: 10
- **Logical** **Processors**: 12
- **Base Clock**: 2.6 kHz (same as max)
- **RAM**: 16 GB LPDDR5-5200
- **L2 Cache Size**: 6656 kB (6 MB)
- **L3 Cache Size**: 12288 kB (12 MB)

## GPU
- Model: Intel(R) Iris(R) Xe Graphics
- Driver Version: 32.0.101.7084
- Dedicated VRAM: 128 MB
- Shared System Memory: 8104 MB
- Max D3D Feature Level: 12_1
- Max Shader Model: 6.7
- UMA: Yes
- Cache-Coherent UMA: Yes
- Num SIMD Lanes: 1536
- Resource Binding Tier: 3
- Resource Heap Tier: 2
- Mesh Shader Tier: Not Supported
- Raytracing Tier: Not Supported
- Variable Rate Shading Tier: 1

# Commit Syntax
- [STRATEGY] {optimization strategy}: {max num of objects at 60fps}
	- e.g., [STRATEGY] Added Instancing: 300
- [SCENE] {updates to scene, such as lighting/shadows/physics/objects etc}: {max num of objects at 60fps}
	- e.g., [SCENE] Added Fixed Directional Lighting: 250
- [CAPABILITY] {new support/QoL related features added like Vulkan/XR etc.}: {max num of objects at 60fps}
- [HARDWARE] {updates to current hardware, like VR Headset/Graphics Card/RAM}: {max num of objects at 60fps on updated hardware}
	- e.g., [HARDWARE] Now using Nvidia RTX-5080: 3000
- [FIX] {bug fixes}
- [BENCHMARKER] {new added metrics or changes to benchmarking/reporting}
	- e.g., [BENCHMARKER] Added Measuring 1% GPU Frame time: 250
- [DOCUMENTATION] {readme updates, comments etc}
- [CLEANUP] {refactors to improve modularity/scalability, removing unused code/comments}
### Format for Commit Description for all commits
{Details on commit syntax}

CPU Median: {x} ms
GPU Median: {y} ms
Bottleneck: CPU/GPU/Mixed

# History (latest to oldest)
Basically a changelog/list of each commit using the above Commit Syntax
