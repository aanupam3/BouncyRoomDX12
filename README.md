# MeshRendererDX12
A low-level rendering engine with the goal of incrementally increasing the maximum number of a given set of visible objects, while maintaining 60fps at stable state.

See [[#Hardware]] and [[#Scene]] sections below for current setup.

# Max Objects Rendered at 60fps (for Latest Commit)
Latest Commit's Max Objects:
- Num: {num} 
- Commit: 
- PrimaryBottleneck: CPU | GPU | Mixed

Previous Commit:
- Num: {num} 
- Commit: {(Commit Name)}
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

AvgDrawCalls
AvgTrianglesRendered
AvgInstancesRendered
AvgDrawCalls

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
| Cube     |                |          |           |                       |
|          |                |          |           |                       |
| Total    |                |          |           |                       |

# Current Test Sequence
1. **Warm up**:  300 frames with 10 objects
2. **Action**: Double Number of Objects 
3. **Measurement**: Wait till rolling average frame time taken over most recent 100 frames stabilizes (+- 1ms) beginning 3s after objects loaded
4. **Reaction**:
	- If stable rolling average frame time is below 15 ms, repeat **Action**
	- If stable rolling average frame time is above 17 ms, take midpoint of current objects with last know value below 15 ms (e.g., (80 + 40)/2 = 60), repeat **Measurement**
	- If stable rolling average frame time is between 15 and 17, save metrics under [[#Current Metrics when Rendering Max Objects at 60fps]]


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
- Max Shader Model: 6_7
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
- [SUPPORT] {new support/QoL related features added like Vulkan/XR/Metrics etc.}: {max num of objects at 60fps}
	- e.g., [FEATURE] Added Measuring 1% GPU Frame time: 250
- [HARDWARE] {updates to current hardware, like VR Headset/Graphics Card/RAM}: {max num of objects at 60fps on updated hardware}
	- e.g., [HARDWARE] Now using Nvidia RTX-5080: 3000
- [FIX] {bug fixes}

### Format for Commit Description for all commits
{Details on STRATEGY | SCENE | SUPPORT | HARDWARE | FIX}

CPU Median: {x} ms
GPU Median: {y} ms
Bottleneck: CPU/GPU/Mixed

# History (latest to oldest)
Basically a changelog/list of each commit using the above Commit Syntax
