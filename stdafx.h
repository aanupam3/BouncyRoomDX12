#pragma once


#include "d3dx12.h"
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

// Handle to the window
HWND hwnd = NULL;

// name of the window (not the title)
LPCTSTR WindowName = L"D3D12Practice";

// title of the window
LPCTSTR WindowTitle = L"D3D12 Practice Window";

// width and height of the window
const UINT Width = 800;
const UINT Height = 600;

// is window full screen?
bool FullScreen = false;

// we will exit the program when this becomes false
bool Running = true;

LRESULT CALLBACK WndProc(HWND hWnd,
	UINT msg,
	WPARAM wParam,
	LPARAM lParam);

const int g_frameBufferCount = 3;


#define VERTEXSHADER L"VertexShader.hlsl"
#define PIXELSHADER L"PixelShader.hlsl"

struct Vertex
{
	DirectX::XMFLOAT3 pos;
	DirectX::XMFLOAT4 color;
	DirectX::XMFLOAT2 uv;
};

bool Init();
bool Update();
bool UpdatePipeline();
bool Render();
bool WaitForPreviousFrame();
bool Reset();
void Cleanup();

IDXGIFactory4* dxgiFactory;
ID3D12Device* device;
IDXGISwapChain3* swapChain;
ID3D12Resource* renderTargets[g_frameBufferCount];
ID3D12DescriptorHeap* rtvDescriptorHeap;
ID3D12Resource* depthStencilBuffer;
ID3D12DescriptorHeap* depthStencilDescriptorHeap;
ID3D12CommandQueue* commandQueue;
ID3D12CommandAllocator* commandAllocators[g_frameBufferCount];
ID3D12GraphicsCommandList* commandList;
ID3D12Fence1* fencesGPU[g_frameBufferCount];
int fenceValuesCPU[g_frameBufferCount];
HANDLE WINAPI fenceEvent;
DXGI_SAMPLE_DESC sampleDesc{};

D3D12_SHADER_BYTECODE vertexShaderBytecode{};
D3D12_SHADER_BYTECODE pixelShaderBytecode{};

ID3D12PipelineState* pipelineStateObject;
ID3D12RootSignature* rootSignature;
D3D12_VIEWPORT viewport;
D3D12_RECT scissorRect;

//std::vector<D3D12_VERTEX_BUFFER_VIEW>* vertexPositionBufferViews;
//std::vector<D3D12_VERTEX_BUFFER_VIEW>* vertexNormalsBufferViews;
//std::vector <D3D12_VERTEX_BUFFER_VIEW>* vertexUVBufferViews;
//std::vector<D3D12_INDEX_BUFFER_VIEW>* indexBufferViews;

//std::vector<int>* numIndices;
//std::vector<DirectX::XMMATRIX>* wvpMatrices;
//int totalMeshes{};
//int totalTextures{};
//
//std::vector<ID3D12DescriptorHeap*>* textureSrvDescriptorHeaps;
//std::vector<ID3D12DescriptorHeap*>* samplerDescriptorHeaps;

const std::string oakTreeModelBasePath{ "./models/OakTree/" };

const std::string cubeModelBasePath{ "./models/cube/" };

//constexpr float PI = 3.14159265358979f;
//constexpr float frequency = 1.0f; // 1 Hz = 1 cycle per second
//constexpr float r = 1.0f;
//float elapsedTime = 0.0f;
//float timeStep = 0.01f;
//float baseTheta = 0;
//struct BaseTrigTheta {
//    float sinBaseTheta;
//    float cosBaseTheta;
//};
//BaseTrigTheta baseTrigTheta;
//std::chrono::high_resolution_clock::time_point launchTime = std::chrono::high_resolution_clock::now();
//
//static float timeSum = 0.0f;
//static int counter = 0;

//#define USE_BAR
//const int texWidth = 3840;
//const int texHeight = 2160;
//const int numChannels = 4;
//const int textureBufferSize = texWidth * texHeight * numChannels;
//byte* textureBuffer = nullptr;
//
//const UINT rowMajorWidthBytes
//= (((texWidth * numChannels) / D3D12_TEXTURE_DATA_PITCH_ALIGNMENT) + 1) * D3D12_TEXTURE_DATA_PITCH_ALIGNMENT;
//const UINT rowPitch = rowMajorWidthBytes * sizeof(byte);
//const size_t paddedTextureBufferSize = rowPitch * texHeight;
//byte* paddedTextureBuffer = nullptr;
//
//D3D12_BOX textureBox{ 0, 0, 0, texWidth, texHeight, 1 };
////textureBox.left = 0;
////textureBox.top = 0;
////textureBox.right = texWidth;
////textureBox.bottom = texHeight;
////textureBox.back = 1;
//
//ID3D12Resource* textureUploadHeap;
//ID3D12Resource* textureDefaultHeap;
//ID3D12Resource* samplerUploadHeapResource;
//ID3D12Resource* samplerDefaultHeapResource;
//ID3D12DescriptorHeap* textureSRVDescriptorHeap;