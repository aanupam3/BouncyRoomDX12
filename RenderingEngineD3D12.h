#include "d3dx12.h"
#include "IRenderingEngine.h"
#include "Scene.h"
#include "wincodec.h"
#include <chrono>
#include <d3d12.h>
#include <D3Dcompiler.h>
#include <DirectXMath.h>
#include <dxgi1_4.h>
#include <dxgidebug.h>
#include <pix3.h>
#include <wrl.h>

using Microsoft::WRL::ComPtr;


#pragma once
class RenderingEngineD3D12 : public IRenderingEngine
{
public:
	RenderingEngineD3D12(RenderWindow& renderWindow, GraphicsAPI graphicsAPI = GraphicsAPI::Direct3D12)
		: m_renderWindow(renderWindow), m_graphicsAPI(graphicsAPI) {
	}

	bool Init(Scene&) override;
	bool Render(Scene&) override;
	bool Shutdown() override;

	GraphicsAPI GetRenderingEngineType() { return m_graphicsAPI; };

private:
	RenderWindow& m_renderWindow;
	GraphicsAPI& m_graphicsAPI; // currently only supports D3D12

	bool CreateBufferResource(ComPtr<ID3D12Resource>& bufferResource, size_t bufferSize);
	bool CreateCommandAllocators();
	bool CreateCommandList();
	bool CreateCommandQueue();
	bool CreateDepthStencilBuffer();
	bool CreateDevice();
	bool CreateFactory();
	bool CreateFences();
	bool CreatePipelineStateObject(const MeshPrimitive& meshPrimitive);
	bool CreateRootSignature(const MeshPrimitive& meshPrimitive);
	bool CreateRTVAndDescriptorHeap();
	bool CreateSwapChain();
	bool CreateTextureResource(Texture& texture);
	void CreateViewport();
	bool Reset();
	bool SetShaderVisibleDescriptors(ModelInstance& instance, WorldNode& nodeWithMesh);
	bool UploadBuffer(ID3D12Resource* bufferResource, byte* bufferData, size_t bufferSize);
	bool UpdatePipeline(Scene&);
	bool UploadTexture(const Texture& texture, bool useWriteToSubResource);
	bool WaitForPreviousFrame();

	int m_currentFrameIndex{};// current render target view we are on
	const static int kFrameBufferCount = 3;

	ComPtr<ID3D12CommandAllocator> m_commandAllocators[kFrameBufferCount]{};
	ComPtr<ID3D12CommandQueue> m_commandQueue{};
	ComPtr<ID3D12GraphicsCommandList> m_commandList{};
	ComPtr<ID3D12Resource> m_depthStencilBuffer{};
	ComPtr<ID3D12DescriptorHeap> m_depthStencilDescriptorHeap{};
	ComPtr<ID3D12Device> m_device{};
	ComPtr<IDXGIFactory4> m_dxgiFactory{};
	ComPtr<ID3D12PipelineState> m_pipelineStateObject{};
	ComPtr<ID3D12Resource> m_renderTargets[kFrameBufferCount]{};
	ComPtr<ID3D12RootSignature> m_rootSignature{};
	ComPtr<ID3D12DescriptorHeap> m_rtvDescriptorHeap{};
	ComPtr<IDXGISwapChain3> m_swapChain{};

	UINT m_rtvDescriptorSize{};
	D3D12_VIEWPORT m_viewport{};
	D3D12_RECT m_scissorRect{};

	ComPtr<ID3D12Fence1> m_fencesGPU[kFrameBufferCount];
	int m_fenceValuesCPU[kFrameBufferCount];
	HANDLE WINAPI m_fenceEvent;
};

