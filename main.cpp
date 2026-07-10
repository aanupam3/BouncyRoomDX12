#include "Macros.h"
#include "Model.h"
#include "stdafx.h"

LRESULT CALLBACK WindowProc(HWND hWnd, UINT msgId, WPARAM wParam, LPARAM lParam);

int g_frameIndex;// current render target view we are on
UINT g_rtvDescriptorSize; // size of rtv description (in pixels?), i.e., all front and back buffers are same size

std::vector<Model*> models;

bool InitializeWindow(HINSTANCE hInstance,
	int ShowWnd,
	bool fullscreen)
{
	if (fullscreen)
	{
		HMONITOR hmon = MonitorFromWindow(hwnd,
			MONITOR_DEFAULTTONEAREST);
		MONITORINFO mi = { sizeof(mi) };
		GetMonitorInfo(hmon, &mi);

		/*Width = mi.rcMonitor.right - mi.rcMonitor.left;
		Height = mi.rcMonitor.bottom - mi.rcMonitor.top;*/
	}

	WNDCLASSEX wc;

	wc.cbSize = sizeof(WNDCLASSEX);
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = WndProc;
	wc.cbClsExtra = NULL;
	wc.cbWndExtra = NULL;
	wc.hInstance = hInstance;
	wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 2);
	wc.lpszMenuName = NULL;
	wc.lpszClassName = WindowName;
	wc.hIconSm = LoadIcon(NULL, IDI_APPLICATION);

	if (!RegisterClassEx(&wc))
	{
		MessageBoxA(NULL, "Error registering class",
			"Error", MB_OK | MB_ICONERROR);
		return false;
	}

	hwnd = CreateWindowEx(NULL,
		WindowName,
		WindowTitle,
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT,
		Width, Height,
		NULL,
		NULL,
		hInstance,
		NULL);

	if (!hwnd)
	{
		MessageBoxA(NULL, "Error creating window",
			"Error", MB_OK | MB_ICONERROR);
		return false;
	}

	if (fullscreen)
	{
		SetWindowLong(hwnd, GWL_STYLE, 0);
	}

	ShowWindow(hwnd, ShowWnd);
	UpdateWindow(hwnd);

	return true;
}

void InitConsole()
{
	AllocConsole();

	FILE* f;
	freopen_s(&f, "CONOUT$", "w", stdout);
	freopen_s(&f, "CONOUT$", "w", stderr);
	freopen_s(&f, "CONIN$", "r", stdin);

	printf("Console initialized\n");
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR lpCmdLine, int nShowCmd)
{

	InitConsole();
	std::cout << "Hello from console\n";

	if (!InitializeWindow(hInstance, nShowCmd, FullScreen))
	{
		MessageBoxA(0, "Window Initialization - Failed",
			"Error", MB_OK);
		return 1;
	}

	if (!Init())
	{
		MessageBoxA(0, "Failed to initialize direct3d 12",
			"Error", MB_OK);
		Running = false;
		Cleanup();
		return 1;
	}

	MSG msg;
	ZeroMemory(&msg, sizeof(MSG));


	while (Running)
	{
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			if (msg.message == WM_QUIT)
			{
				break;
			}

			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else {
			//PIXBeginEvent(PIX_COLOR_DEFAULT, "Frame %llu", g_frameIndex);

			//PIXBeginEvent(PIX_COLOR_DEFAULT, "Update");
			Update();
			//PIXEndEvent();

			Render(); // execute the command queue (rendering the scene is the result of the gpu executing the command lists)

			//PIXEndEvent();
		}
	}

	WaitForPreviousFrame();

	// close the fence event
	CloseHandle(fenceEvent);

	// clean up everything
	Cleanup();

	return 0;
}

LRESULT CALLBACK WndProc(HWND hwnd,
	UINT msg,
	WPARAM wParam,
	LPARAM lParam)
{
	switch (msg)
	{
	case WM_KEYDOWN:
		if (wParam == VK_ESCAPE) {
			if (MessageBoxA(0, "Are you sure you want to exit?",
				"Really?", MB_YESNO | MB_ICONQUESTION) == IDYES)
			{
				Running = false;
				DestroyWindow(hwnd);
			}
		}
		return 0;

	case WM_DESTROY: // x button on top right corner of window was pressed
		Running = false;
		PostQuitMessage(0);
		return 0;
	}
	return DefWindowProc(hwnd,
		msg,
		wParam,
		lParam);
}

bool CreateFactory()
{
	HRESULT hr;

	hr = CreateDXGIFactory1(IID_PPV_ARGS(&dxgiFactory));

	PROMPTFAIL(hr, "Create DXGI Factory failed");

	return true;
}


bool CreateDevice()
{
	HRESULT hr{};

	ComPtr<ID3D12Debug> debugController;
	if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
	{
		debugController->EnableDebugLayer();
	}

	// find the right adapter by checking to see if we can create a device with it
	IDXGIAdapter1* adapter;
	UINT adapterId = 0;
	bool adapterFound = false;
	while (dxgiFactory->EnumAdapters1(adapterId, &adapter) != DXGI_ERROR_NOT_FOUND)
	{
		DXGI_ADAPTER_DESC1 dxgiAdapterDesc{};
		hr = adapter->GetDesc1(&dxgiAdapterDesc);
		if (FAILED(hr))
		{
			std::string fullMsg = "Failed to get DXGI Adapter Desc" + Utils::HrToAString(hr);
			MessageBoxA(0, fullMsg.c_str(), "Error", MB_OK);

			adapterId++;
			continue;
		}

		if (dxgiAdapterDesc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
		{
			adapterId++;
			continue;
		}

		// use the adapter to try and create a device
		hr = D3D12CreateDevice(adapter, D3D_FEATURE_LEVEL_11_0, __uuidof(ID3D12Device), NULL);
		if (SUCCEEDED(hr))
		{
			adapterFound = true;
			break;
		}

		adapterId++;
	}

	if (adapterFound)
	{
		D3D12CreateDevice(adapter, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&device));
		return true;
	}
	else
	{
		std::cout << "ERROR: Failed to find suitable adapter to create device";
		return false;
	}
}


bool CreateCommandQueue()
{
	HRESULT hr;
	D3D12_COMMAND_QUEUE_DESC commandQueueDesc{};
	commandQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	commandQueueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
	commandQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	commandQueueDesc.NodeMask = 0;

	hr = device->CreateCommandQueue(&commandQueueDesc, IID_PPV_ARGS(&commandQueue));

	PROMPTFAIL(hr, "Failed to create direct command queue with error ");

	return true;
}

bool CreateSwapChain()
{
	HRESULT hr;
	DXGI_SWAP_CHAIN_DESC swapChainDesc = {};

	swapChainDesc.BufferCount = g_frameBufferCount;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.OutputWindow = hwnd;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD; // faster as it avoids an extra copy with DWM
	swapChainDesc.Windowed = TRUE;

	DXGI_MODE_DESC backBufferDesc = {};
	backBufferDesc.Width = Width; // 0 defaults to window size
	backBufferDesc.Height = Height;  // 0 defaults to window size
	backBufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.BufferDesc = backBufferDesc;

	sampleDesc.Count = 1;
	swapChainDesc.SampleDesc = sampleDesc; // minimum 1 needed for sampling the back buffer

	IDXGISwapChain* tempSwapChain{};
	hr = dxgiFactory->CreateSwapChain(commandQueue, &swapChainDesc, &tempSwapChain);

	PROMPTFAIL(hr, "Failed to create swap chain");

	swapChain = static_cast<IDXGISwapChain3*>(tempSwapChain);
	g_frameIndex = swapChain->GetCurrentBackBufferIndex();

	return true;
}

bool CreateRTVAndDescriptorHeap()
{
	HRESULT hr;

	D3D12_DESCRIPTOR_HEAP_DESC rtvDescriptorHeapDesc;
	rtvDescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	rtvDescriptorHeapDesc.NumDescriptors = g_frameBufferCount; // one render target for each back buffer
	rtvDescriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	rtvDescriptorHeapDesc.NodeMask = 0;

	hr = device->CreateDescriptorHeap(&rtvDescriptorHeapDesc, IID_PPV_ARGS(&rtvDescriptorHeap));

	PROMPTFAIL(hr, "Failed to Create RTV Descriptor Heap with error ");

	g_rtvDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	// Why can't I use pointer math to get the handle of the first descriptor from the heap pointer itself?
	// The heap contains metadata about the heap, so you don't know where the descriptor begins
	// The heap owns descriptor storage; the runtime maps that storage to 
	// CPU/GPU-visible address spaces and hands you opaque handles when you ask.
	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvDescriptorHandle{ rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart() };

	for (int i = 0; i < g_frameBufferCount; i++)
	{
		// Get the swap chain buffer locations
		hr = swapChain->GetBuffer(i, IID_PPV_ARGS(&renderTargets[i]));

		{
			const std::string msg = "Failed to Get swap chain buffer " + std::to_string(i);
			PROMPTFAIL(hr, msg.c_str());
		}

		// Assign the rtv descriptor handle to the swap chain buffers
		device->CreateRenderTargetView(renderTargets[i], nullptr, rtvDescriptorHandle);

		rtvDescriptorHandle.Offset(1, g_rtvDescriptorSize);
	}
	return true;
}

bool CreateCommandAllocators()
{
	HRESULT hr;

	// Need as many as frame buffer count since we can't reset while one is executing, and there will be
	// multiple executing simultaneously as we attempt to fill the backbuffers while the front is scanning out
	for (int i = 0; i < g_frameBufferCount; i++)
	{
		hr = device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&commandAllocators[i]));

		PROMPTFAIL(hr, "Failed to Create Command Allocator at index " + std::to_string(i));
	}

	return true;
}

bool CreateCommandList()
{
	HRESULT hr;

	hr = device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocators[g_frameIndex], NULL, IID_PPV_ARGS(&commandList));

	PROMPTFAIL(hr, "Failed to create command list");

	commandList->Close();

	return true;
}

bool CreateFences()
{
	HRESULT hr;

	// Need as many fences as the frame buffer count (since single threaded)
	for (int i = 0; i < g_frameBufferCount; i++)
	{
		hr = device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fencesGPU[i]));

		const std::string msg = "Failed to Create Fence for frame buffer " + std::to_string(i);
		PROMPTFAIL(hr, msg.c_str());

		fenceValuesCPU[i] = 0;
	}

	fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
	if (fenceEvent == FALSE)
	{
		return false;
	}

	return true;
}

void CreateViewport()
{
	// Fill out the Viewport
	viewport.TopLeftX = 0;
	viewport.TopLeftY = 0;
	viewport.Width = Width;
	viewport.Height = Height;
	viewport.MinDepth = 0.01f;
	viewport.MaxDepth = 1000.0f;

	// Fill out a scissor rect
	scissorRect.left = 0;
	scissorRect.top = 0;
	scissorRect.right = Width;
	scissorRect.bottom = Height;
}

bool CreateRootSignature(MeshPrimitive* meshPrimitive)
{
	D3D12_ROOT_PARAMETER rootParams[2]{};

	D3D12_DESCRIPTOR_RANGE rootWVPMatricesDescRange{};
	rootWVPMatricesDescRange.BaseShaderRegister = 0; //b0
	rootWVPMatricesDescRange.NumDescriptors = 1;
	rootWVPMatricesDescRange.OffsetInDescriptorsFromTableStart = 0;
	rootWVPMatricesDescRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;

	D3D12_ROOT_DESCRIPTOR_TABLE rootWVPMatricesDescTable{};
	rootWVPMatricesDescTable.NumDescriptorRanges = 1;
	rootWVPMatricesDescTable.pDescriptorRanges = &rootWVPMatricesDescRange;

	rootParams[0].DescriptorTable = rootWVPMatricesDescTable;
	rootParams[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParams[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	// ------------- texture srv -----------------------------------------------------------
	D3D12_DESCRIPTOR_RANGE rootTextSrvDescriptorRange{};
	rootTextSrvDescriptorRange.BaseShaderRegister = 0; //t0
	rootTextSrvDescriptorRange.NumDescriptors = static_cast<UINT>(meshPrimitive->Textures.size());
	rootTextSrvDescriptorRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
	rootTextSrvDescriptorRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;

	D3D12_ROOT_DESCRIPTOR_TABLE rootTexSrvDescriptorTable{};
	rootTexSrvDescriptorTable.NumDescriptorRanges = 1;
	rootTexSrvDescriptorTable.pDescriptorRanges = &rootTextSrvDescriptorRange;

	rootParams[1].DescriptorTable = rootTexSrvDescriptorTable;
	rootParams[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParams[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	// ----- sampler --------------------------------------------------------------------
	D3D12_STATIC_SAMPLER_DESC textureSampler{};
	textureSampler.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
	textureSampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	textureSampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	textureSampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	textureSampler.ShaderRegister = 0; //s0
	textureSampler.RegisterSpace = 0;

	//D3D12_DESCRIPTOR_RANGE rootTextSamplerDescriptorRange{};
	//rootTextSamplerDescriptorRange.BaseShaderRegister = 0; //t0
	//rootTextSamplerDescriptorRange.NumDescriptors = 1;
	//rootTextSamplerDescriptorRange.OffsetInDescriptorsFromTableStart = 0;
	//rootTextSamplerDescriptorRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER;

	//D3D12_ROOT_DESCRIPTOR_TABLE rootSamplerDescriptorTable{};
	//rootSamplerDescriptorTable.NumDescriptorRanges = 1;
	//rootSamplerDescriptorTable.pDescriptorRanges = &rootTextSrvDescriptorRange;

	/*rootParams[2].DescriptorTable = rootSamplerDescriptorTable;
	rootParams[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParams[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;*/

	// ----------------- root signature definition ------------------------------------------
	D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc;
	rootSignatureDesc.NumParameters = 2;
	rootSignatureDesc.pParameters = rootParams;
	rootSignatureDesc.NumStaticSamplers = 1;
	rootSignatureDesc.pStaticSamplers = &textureSampler;
	rootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

	ID3DBlob* signature;
	ID3DBlob* errorBlob;
	HRESULT hr = D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &errorBlob);

	if (FAILED(hr))
	{
		const char* errorMsg = (const char*)errorBlob->GetBufferPointer();
		MessageBoxA(0, errorMsg, "Error", MB_OK);
	}
	PROMPTFAIL(hr, "Failed to assign root signature blob! ");

	hr = device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&rootSignature));

	PROMPTFAIL(hr, "Failed to create root signature! ");

	return true;
}


// Needs root signature and input layout
bool CreatePipelineStateObject(MeshPrimitive* meshPrimitive)
{
	HRESULT hr;
	D3D12_GRAPHICS_PIPELINE_STATE_DESC graphicsPipelineStateDesc{};
	graphicsPipelineStateDesc.InputLayout = meshPrimitive->inputLayout;
	graphicsPipelineStateDesc.pRootSignature = rootSignature;
	//graphicsPipelineStateDesc.Flags = D3D12_PIPELINE_STATE_FLAG_TOOL_DEBUG;
	/*graphicsPipelineStateDesc.VS = vertexShaderBytecode;
	graphicsPipelineStateDesc.PS = pixelShaderBytecode;*/
	graphicsPipelineStateDesc.VS = { meshPrimitive->Shaders[0].BinData, meshPrimitive->Shaders[0].BinSize };
	graphicsPipelineStateDesc.PS = { meshPrimitive->Shaders[1].BinData, meshPrimitive->Shaders[1].BinSize };
	graphicsPipelineStateDesc.NumRenderTargets = 1;
	graphicsPipelineStateDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	graphicsPipelineStateDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	graphicsPipelineStateDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	graphicsPipelineStateDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	graphicsPipelineStateDesc.SampleMask = 0xffffff; // point sampling
	graphicsPipelineStateDesc.SampleDesc = sampleDesc;

	// When using triangle strip primitive topology, vertex positions are interpreted as vertices of a continuous triangle “strip”.
	// There is a special index value that represents the desire to have a discontinuity in the strip, the cut index value. 
	// This enum lists the supported cut values.
	//graphicsPipelineStateDesc.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;

	hr = device->CreateGraphicsPipelineState(&graphicsPipelineStateDesc, IID_PPV_ARGS(&pipelineStateObject));
	//std::cout << "Pipeline state object: " << pipelineStateObject << "\n";
	PROMPTFAIL(device->GetDeviceRemovedReason(), "Failed to create graphics pipeline state object. ");

	return true;
}

bool Update()
{
	return true;
}

bool WaitForPreviousFrame()
{
	HRESULT hr;

	g_frameIndex = swapChain->GetCurrentBackBufferIndex();

	if (fencesGPU[g_frameIndex]->GetCompletedValue() < fenceValuesCPU[g_frameIndex])
	{
		hr = fencesGPU[g_frameIndex]->SetEventOnCompletion(fenceValuesCPU[g_frameIndex], fenceEvent);

		PROMPTFAIL(hr, "Failed to set event on completion for frame index " + std::to_string(g_frameIndex));

		hr = WaitForSingleObject(fenceEvent, INFINITE);

		PROMPTFAIL(hr, "Failed to wait for fence event for frame index " + std::to_string(g_frameIndex));
	}

	fenceValuesCPU[g_frameIndex]++;

	return true;

}

bool UpdatePipeline()
{
	WaitForPreviousFrame();

	HRESULT hr = commandAllocators[g_frameIndex]->Reset();

	PROMPTFAIL(hr, "Failed to reset command allocator");

	// reset, now ready for recording
	hr = commandList->Reset(commandAllocators[g_frameIndex], pipelineStateObject);

	PROMPTFAIL(hr, "Failed to reset command LIST");

	// change from present state to render target state for recording
	D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
		renderTargets[g_frameIndex],
		D3D12_RESOURCE_STATE_PRESENT,
		D3D12_RESOURCE_STATE_RENDER_TARGET
	);
	commandList->ResourceBarrier(1, &barrier);

	// need to get the descriptor handle so we can set it as the render target in output merger stage of pipeline
	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvDescriptorHandle
	{
		rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart() ,
		g_frameIndex,
		g_rtvDescriptorSize
	};

	// If RTsSingleHandleToDescriptorRange (3rd arg) is TRUE, 
	// pRenderTargetDescriptors (2nd arg) points to a contiguous descriptor range (GPU offsets by descriptor size); 
	// if FALSE, it points to an array of handles (extra indirection, less efficient).
	commandList->OMSetRenderTargets(1, &rtvDescriptorHandle, FALSE, nullptr);

	// set color of render target when clearing it
	const float clearColor[] = { 0.6f, 0.2f, 0.4f, 1.0f };
	commandList->ClearRenderTargetView(rtvDescriptorHandle, clearColor, 0, nullptr);

	commandList->RSSetViewports(1, &viewport);
	commandList->RSSetScissorRects(1, &scissorRect);
	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	//std::vector<ID3D12DescriptorHeap*>* textureSRVDescriptorHeaps = new std::vector<ID3D12DescriptorHeap*>();
	for (Model* model : models)
	{
		const std::vector<Mesh*>& modelMeshes = model->GetMeshes();
		for (Mesh* mesh : modelMeshes)
		{
			for (int i = 0; i < mesh->Primitives.size(); i++)
			{
				MeshPrimitive* meshPrimitive = mesh->Primitives[i];
				CHECK_FAIL(CreateRootSignature(meshPrimitive));
				CHECK_FAIL(CreatePipelineStateObject(meshPrimitive));
				commandList->SetGraphicsRootSignature(rootSignature);
				commandList->SetPipelineState(pipelineStateObject);

				int slot = 0;
				for (D3D12_VERTEX_BUFFER_VIEW& vertexBufferView : meshPrimitive->VertexBufferViews)
				{
					//std::cout << "Slot: " << slot << " is at " << vertexBufferView.BufferLocation <<"\n";
					commandList->IASetVertexBuffers(slot, 1, &vertexBufferView);
					slot++;
				}
				commandList->IASetIndexBuffer(&meshPrimitive->IndexBufferView);

				commandList->SetDescriptorHeaps(1, &meshPrimitive->MainShaderVisibleDescriptorHeap);

				D3D12_GPU_DESCRIPTOR_HANDLE descriptorHandle = meshPrimitive->MainShaderVisibleDescriptorHeap->GetGPUDescriptorHandleForHeapStart();
				commandList->SetGraphicsRootDescriptorTable(0, descriptorHandle);

				descriptorHandle.ptr += device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
				commandList->SetGraphicsRootDescriptorTable(1, descriptorHandle);

				commandList->DrawIndexedInstanced(meshPrimitive->NumIndices, 1, 0, 0, 0);
			}
		}
	}

	// change resource state back to present state for rendering
	D3D12_RESOURCE_BARRIER barrier2 = CD3DX12_RESOURCE_BARRIER::Transition(
		renderTargets[g_frameIndex],
		D3D12_RESOURCE_STATE_RENDER_TARGET,
		D3D12_RESOURCE_STATE_PRESENT
	);
	commandList->ResourceBarrier(1, &barrier2);
	hr = commandList->Close();

	PROMPTFAIL(hr, "Failed to close command list ");

	return true;
}

bool Render()
{
	HRESULT hr = S_OK;
	if (UpdatePipeline() == false)
	{
		PROMPTFAIL(hr, "Failed to record!\n");
	}

	ID3D12CommandList* commandLists[]{ commandList };

	commandQueue->ExecuteCommandLists(_countof(commandLists), commandLists);

	hr = commandQueue->Signal(fencesGPU[g_frameIndex], fenceValuesCPU[g_frameIndex]);

	PROMPTFAIL(hr, "Failed to signal fence with error ");

	// presents the current back buffer
	hr = swapChain->Present(0, 0);

	const std::string msg = "Failed to present backbuffer at index " + std::to_string(g_frameIndex);
	PROMPTFAIL(hr, msg.c_str());

	return true;
}

bool Reset()
{
	WaitForPreviousFrame();

	HRESULT hr;

	// Resets write_offset = 0 (this is used for tracking what command we are on in the gpu)
	// Immediately invalidates all command streams
	// Does not wait for GPU
	hr = commandAllocators[g_frameIndex]->Reset();

	PROMPTFAIL(hr, "Failed to reset command allocator at frame index " + g_frameIndex);

	hr = commandList->Reset(commandAllocators[g_frameIndex], nullptr);

	PROMPTFAIL(hr, "Failed to reset command list with error ");

	commandList->Close();

	return true;
}

bool CreateTextureResource(Texture* texture)
{
	// For UMA or NUMA, Textures should be on the DEFAULT heap (and not UPLOAD/GPU_UPLOAD) if frequently read
	// because the movement from UPLOAD->DEFAULT swizzles the texture, which improves caching

	D3D12_RESOURCE_DESC textureDefaultResourceDesc{};
	textureDefaultResourceDesc.Alignment = 0;
	textureDefaultResourceDesc.DepthOrArraySize = 1;
	textureDefaultResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	textureDefaultResourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
	textureDefaultResourceDesc.Format = texture->Format;
	textureDefaultResourceDesc.Height = texture->Height;
	textureDefaultResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	textureDefaultResourceDesc.MipLevels = 1;
	textureDefaultResourceDesc.SampleDesc = { 1,0 };
	textureDefaultResourceDesc.Width = texture->Width;

	D3D12_HEAP_PROPERTIES textureDefaultHeapProperties{ D3D12_HEAP_TYPE_GPU_UPLOAD };
	HRESULT hr = device->CreateCommittedResource(
		&textureDefaultHeapProperties,
		D3D12_HEAP_FLAG_NONE,
		&textureDefaultResourceDesc,
		D3D12_RESOURCE_STATE_COMMON,
		nullptr,
		IID_PPV_ARGS(&texture->GPUResource));

	PROMPTFAIL(hr, "Failed to create committed resource for texture " + texture->Name + " default heap");

	return true;
}

bool CreateBufferResource(ID3D12Resource*& bufferResource, size_t bufferSize)
{
	// For UMA, buffers can be on the UPLOAD heap and get comparable GPU-side access times/performance as DEFAULT
	// with the advantage being one less required copy
	// This is not true for Textures due to the swizzled nature of textures in a DEFAULT heap.

	D3D12_RESOURCE_DESC bufferResourceDesc{};
	bufferResourceDesc.Alignment = 0;
	bufferResourceDesc.DepthOrArraySize = 1;
	bufferResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	bufferResourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
	bufferResourceDesc.Format = DXGI_FORMAT_UNKNOWN;
	bufferResourceDesc.Height = 1;
	bufferResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	bufferResourceDesc.MipLevels = 1;
	bufferResourceDesc.SampleDesc = { 1,0 };
	bufferResourceDesc.Width = bufferSize;

	//D3D12_HEAP_PROPERTIES bufferHeapProperties{ D3D12_HEAP_TYPE_GPU_UPLOAD };
	CD3DX12_HEAP_PROPERTIES bufferHeapProperties(D3D12_HEAP_TYPE_UPLOAD);
	HRESULT hr = device->CreateCommittedResource(
		&bufferHeapProperties,
		D3D12_HEAP_FLAG_NONE,
		&bufferResourceDesc,
		D3D12_RESOURCE_STATE_COMMON,
		nullptr,
		IID_PPV_ARGS(&bufferResource));

	//std::cout << "Buffer resource when creating: " << bufferResource << "\n";
	PROMPTFAIL(hr, "Failed to create buffer resource");

	return true;
}

bool SetDescriptors(Mesh* mesh)
{
	HRESULT hr;

	Node& meshNode = *(mesh->MeshNode);

	// Create the common handle for the WVP matrix
	D3D12_CONSTANT_BUFFER_VIEW_DESC wvpMatrixCBVDesc{};
	wvpMatrixCBVDesc.BufferLocation = meshNode.WVPMatrixGPUResource->GetGPUVirtualAddress();
	wvpMatrixCBVDesc.SizeInBytes = 256 * (1 + static_cast<UINT>(meshNode.WVPMatrixVector.size()) / 256);

	std::cout << "Mesh " << mesh->Name << " CBV size in bytes " << wvpMatrixCBVDesc.SizeInBytes << "\n";

	for (int i = 0; i < mesh->Primitives.size(); i++)
	{
		MeshPrimitive* meshPrimitive = mesh->Primitives[i];
		// Create the descriptor heap to hold descriptors for all mesh resources -----------------------------------------
		D3D12_DESCRIPTOR_HEAP_DESC mainSrvDescriptorHeapDesc{};
		mainSrvDescriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		mainSrvDescriptorHeapDesc.NumDescriptors = static_cast<UINT>(meshPrimitive->Textures.size()) + 1; // +1 for mesh's WVP matrix
		mainSrvDescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		hr = device->CreateDescriptorHeap(&mainSrvDescriptorHeapDesc, IID_PPV_ARGS(&meshPrimitive->MainShaderVisibleDescriptorHeap));
		PROMPTFAIL(hr, "Failed to create main descriptor heap for mesh " + std::string(mesh->Name) + " primitive at index " + std::to_string(i));

		int descriptorNumber = 0;
		UINT descriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		CD3DX12_CPU_DESCRIPTOR_HANDLE wvpMatrixCBVHandle(
			meshPrimitive->MainShaderVisibleDescriptorHeap->GetCPUDescriptorHandleForHeapStart(),
			descriptorNumber, descriptorSize); // offsets the Heapstart by the descriptorNumber*descriptorSize

		// Create the view
		device->CreateConstantBufferView(&wvpMatrixCBVDesc, wvpMatrixCBVHandle);

		// Setup SRV for shader interaction with Resource -------------------------------------
		D3D12_TEX2D_SRV texture2DSrvMips{};
		texture2DSrvMips.MipLevels = 1;

		D3D12_SHADER_RESOURCE_VIEW_DESC textureSrvDesc{};
		textureSrvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		textureSrvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		textureSrvDesc.Texture2D = texture2DSrvMips;
		textureSrvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;

		for (Texture* texture : meshPrimitive->Textures)
		{
			descriptorNumber++;
			CD3DX12_CPU_DESCRIPTOR_HANDLE textureSRVHandle(
				meshPrimitive->MainShaderVisibleDescriptorHeap->GetCPUDescriptorHandleForHeapStart(),
				descriptorNumber, descriptorSize);

			device->CreateShaderResourceView(
				texture->GPUResource,
				&textureSrvDesc,
				textureSRVHandle
			);
		}
	}

}

// Assumes n*256-byte aligned rows in a texture
bool UploadTexture(Texture* texture, bool useWriteToSubResource = true)
{
	HRESULT hr;

	if (texture->GPUResource == nullptr)
	{
		std::cout << "Cannot upload texture, gpu resource does not exist!";
		return false;
	}

	if (useWriteToSubResource)
	{
		//D3D12_BOX texBox = { 0, 0, 0, texture->Width, texture->Height, 1 };
		hr = texture->GPUResource->WriteToSubresource(0, &texture->TexBox, texture->PixelData, texture->Width * texture->NumChannels, 1);

		PROMPTFAIL(hr, "Failed to write texture " + std::string(texture->Name) + " to subresource, ");
	}
	else
	{
		PROMPTFAIL(0, "Upload heap to default heap approach not set up for uploading textures");
	}

	return true;
}

// Assumes buffer is n*256-byte aligned and the resource property is GPU_UPLOAD
bool UploadBuffer(ID3D12Resource* bufferResource, byte* bufferData, size_t bufferSize)
{
	//UINT alignedBufferSize = 256 * (1 + static_cast<UINT>((bufferSize - 1) / 256));
	/*

	if(!bufferData)
	{
		PROMPTFAIL(0, "Buffer source data is null for upload!");
	}*/

	/*if (!bufferResource)
	{
		PROMPTFAIL(0, "Buffer resource is null for upload!");
	}*/

	/*bufferResource->Map(0, nullptr, nullptr);
	D3D12_BOX bufferBox{ 0, 0, 0, alignedBufferSize, 1, 1 };
	HRESULT hr = bufferResource->WriteToSubresource(0, &bufferBox, bufferData, alignedBufferSize, 1);
	bufferResource->Unmap(0, nullptr);

	PROMPTFAIL(hr, "Failed to write buffer to subresource");*/

	//return true;

	// This works well for UMA since the data is all on System RAM instead of a discrete VRAM

	void* mapped = nullptr;

	D3D12_RANGE readRange = { 0, 0 }; // CPU will not read

	HRESULT hr = bufferResource->Map(0, &readRange, &mapped);

	PROMPTFAIL(hr, "Failed to map buffer resource");

	memcpy(mapped, bufferData, bufferSize);

	D3D12_RANGE writtenRange = {
		0,
		bufferSize
	};

	bufferResource->Unmap(0, &writtenRange);

	return true;
}


bool Init()
{
	CHECK_FAIL(CreateFactory());
	CHECK_FAIL(CreateDevice()); // needs factory
	CHECK_FAIL(CreateCommandAllocators()); // needs device
	CHECK_FAIL(CreateCommandQueue()); // needs device
	CHECK_FAIL(CreateSwapChain()); // needs factory and device and command queue
	CHECK_FAIL(CreateRTVAndDescriptorHeap()); // needs device and swapchain
	CHECK_FAIL(CreateCommandList()); // needs device and command allocator
	CHECK_FAIL(CreateFences()); // needs device

	//CHECK_FAIL(CompileShaders());
	CreateViewport();

	commandList->Reset(commandAllocators[g_frameIndex], nullptr);

	// Upload the binary file to the GPU ----------------------------------------------------
	Model* oakTreeModel = new Model(oakTreeModelBasePath, "OakTree");
	//Model* cubeModel = new Model(cubeModelBasePath, "cube");

	models.push_back(oakTreeModel);
	//models.push_back(cubeModel);

	constexpr float pitch = DirectX::XMConvertToRadians(0); // X rotation
	constexpr float yaw = DirectX::XMConvertToRadians(0); // Y rotation
	constexpr float roll = DirectX::XMConvertToRadians(0); // Z rotation
	DirectX::XMMATRIX rotation = DirectX::XMMatrixRotationRollPitchYaw(pitch, yaw, roll);

	DirectX::XMMATRIX translation = DirectX::XMMatrixTranslation(
		0.0f,
		0.0f,
		0.0f
	);
	DirectX::XMMATRIX cameraWorldMatrix = rotation * translation;
	/*std::cout << "\nCamera matrix is:";
	Utils::printMatrix(cameraWorldMatrix);*/
	/*const DirectX::XMMATRIX cameraWorldMatrix
	{
		0.7f, 0.7f, 0.0f, 0.0f,
		-0.7f, 0.7f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		-20.0f, 20.0f, -60.0f, 1.0f,
	};*/
	const DirectX::XMMATRIX viewMatrix{ DirectX::XMMatrixInverse(nullptr, cameraWorldMatrix) };
	//std::cout << "\nView matrix is:";
	//Utils::printMatrix(viewMatrix);

	const float n = 0.01f; // near clipping plane
	const float f = 1000.0f; // far clipping plane
	// assume fov = 90, so zoom = 1 along width
	const DirectX::XMMATRIX projectionMatrix
	{
		1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 1.0f * Width / Height, 0.0f, 0.0f,
		0.0f, 0.0f, f / (f - n), 1.0f,
		0.0f, 0.0f, -n * f / (f - n), 0.0f
	};
	//std::cout << "\nProjection matrix is:";
	//Utils::printMatrix(projectionMatrix);

	const DirectX::XMMATRIX vpMatrix = DirectX::XMMatrixMultiply(viewMatrix, projectionMatrix);
	//std::cout << "\nVP matrix is:";
	//Utils::printMatrix(vpMatrix);

	/*std::vector<float>* allWVPMatrices = new std::vector<float>();
	std::vector<byte>* allTextureData = new std::vector<byte>();*/

	for (Model* model : models)
	{
		const std::vector<Mesh*>& modelMeshes = model->GetMeshes();

		//if (!model->UploadModelBinary(device, commandList)) { return false; }
		const ModelBinData* modelBinData = model->GetBinData();
		CHECK_FAIL(CreateBufferResource(model->ModelBinResource, modelBinData->binDataSize));
		//std::cout << "Buffer resource after creating: " << model->ModelBinResource << "\n";
		CHECK_FAIL(UploadBuffer(model->ModelBinResource, modelBinData->binData, modelBinData->binDataSize));

		model->SetData();

		oakTreeModel->SetWorldPosition(0.0f, -10.0f, 50.0f);
		oakTreeModel->SetWorldRotationDegrees(-90, 0, 0);
		oakTreeModel->SetWorldScale(2.0f);

		oakTreeModel->ScaleBy(0.5f, 0.5f, 0.5f);

		for (Mesh* mesh : modelMeshes)
		{
			/*DirectX::XMMATRIX& meshWorldMatrix = mesh->worldMatrix;
				std::cout << "\nWorld matrix :";
				Utils::printMatrix(meshWorldMatrix);*/
				//wvpMatrix = DirectX::XMMatrixTranspose(DirectX::XMMatrixMultiply(*worldMatricesForRenderableNodes[i], vpMatrix));

			Node& meshNode = *(mesh->MeshNode);

			/*std::cout << "\World Space Transform matrix :";
			Utils::printMatrix(meshNode.WorldSpaceTransformMatrix);*/

			DirectX::XMMATRIX wvpMatrix{ DirectX::XMMatrixMultiply(meshNode.WorldSpaceTransformMatrix, vpMatrix) };
			model->SetWVPMatrixForMesh(mesh, wvpMatrix);
			std::cout << "\nWVP matrix :";
			Utils::printMatrix(meshNode.WVPMatrix);
			/*std::cout << "\n";
			for (float value : mesh->WVPMatrixVector)
			{
				std::cout << value << ",";
			}
			std::cout << "\n";*/

			CHECK_FAIL(CreateBufferResource(meshNode.WVPMatrixGPUResource, meshNode.WVPMatrixVector.size() * sizeof(float)));
			CHECK_FAIL(UploadBuffer(meshNode.WVPMatrixGPUResource, reinterpret_cast<byte*>(meshNode.WVPMatrixVector.data()), meshNode.WVPMatrixVector.size() * sizeof(float)));

			for (int i = 0; i < mesh->Primitives.size(); i++)
			{
				MeshPrimitive* meshPrimitive = mesh->Primitives[i];
				const std::vector<Texture*>& meshPrimitiveTextures = meshPrimitive->Textures;
				for (Texture* texture : meshPrimitiveTextures)
				{
					CHECK_FAIL(CreateTextureResource(texture));
					CHECK_FAIL(UploadTexture(texture));
				}
			}

			SetDescriptors(mesh);
		}
	}

	//commandList->CopyResource(textureDefaultHeap, textureUploadHeap);
	commandList->Close();
	ID3D12CommandList* commandLists[]{ commandList };
	commandQueue->ExecuteCommandLists(_countof(commandLists), commandLists);

	fenceValuesCPU[g_frameIndex]++;
	HRESULT hr = commandQueue->Signal(fencesGPU[g_frameIndex], fenceValuesCPU[g_frameIndex]);
	PROMPTFAIL(hr, "Failed to signal fence with error ");

	return true;
}

void Cleanup()
{
	HRESULT hr;

	for (int i = 0; i < g_frameBufferCount; i++)
	{
		const UINT64 value = ++fenceValuesCPU[i];
		commandQueue->Signal(fencesGPU[i], value);
		fencesGPU[i]->SetEventOnCompletion(value, fenceEvent);
		WaitForSingleObject(fenceEvent, INFINITE);
	}

	BOOL isFullscreen = FALSE;
	if (swapChain)
	{
		if (swapChain->GetFullscreenState(&isFullscreen, NULL))
		{
			swapChain->SetFullscreenState(FALSE, NULL);
		}
	}

	SAFE_RELEASE(swapChain);
	SAFE_RELEASE(rtvDescriptorHeap);
	SAFE_RELEASE(commandQueue);
	SAFE_RELEASE(commandList);
	SAFE_RELEASE(device);
	SAFE_RELEASE(pipelineStateObject);
	SAFE_RELEASE(rootSignature);

	for (int i = 0; i < g_frameBufferCount; i++)
	{
		SAFE_RELEASE(renderTargets[i]);
		SAFE_RELEASE(commandAllocators[i]);
		SAFE_RELEASE(fencesGPU[i]);
	}
}



//bool CompileShaders()
//{
	//
	//HRESULT hr;

	//ID3DBlob* vertexShader;
	//ID3DBlob* errorBufferVS;

	//hr = D3DCompileFromFile(
	//	VERTEXSHADER,
	//	nullptr, // macros (e.g., {"ABCD", "3"})
	//	nullptr, // any #includes in the shader script
	//	"main", // name of the entry function in the shader
	//	"vs_5_1", // shader model used for compilation (e.g,. shader model 5.0)
	//	D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION, // compile option flags
	//	0, // effect flags
	//	&vertexShader,
	//	&errorBufferVS);

	//if(FAILED(hr))
	//{
	//	LOG_HR_AND_RETURN_FAIL(hr, "Failed to compile vertex shader!");
	//	const char* errorMsg = (const char*)errorBufferVS->GetBufferPointer();
	//	MessageBoxA(0, errorMsg, "Error", MB_OK);
	//}

	//vertexShaderBytecode.BytecodeLength = vertexShader->GetBufferSize();
	//vertexShaderBytecode.pShaderBytecode = vertexShader->GetBufferPointer();

	//ID3DBlob* errorBufferPS;

	//ID3DBlob* pixelShader;
	//hr = D3DCompileFromFile(
	//	PIXELSHADER,
	//	nullptr,
	//	nullptr,
	//	"main",
	//	"ps_5_1",
	//	D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,
	//	0,
	//	&pixelShader,
	//	&errorBufferPS);

	//if (FAILED(hr))
	//{
	//	//LOG_HR_AND_RETURN_FAIL(hr, "Failed to compile pixel shader!");
	//	const char* errorMsg = (const char*)errorBufferPS->GetBufferPointer();
	//	MessageBoxA(0, errorMsg, "Error", MB_OK);
	//}
	//
	//pixelShaderBytecode.BytecodeLength = pixelShader->GetBufferSize();
	//pixelShaderBytecode.pShaderBytecode = pixelShader->GetBufferPointer();

//	return true;
//}

// ---- create committed texture upload and default resources -------------------------
		//ID3D12Resource* textureUploadHeap{};
		//D3D12_HEAP_PROPERTIES textureUploadHeapProperties{ D3D12_HEAP_TYPE_UPLOAD };
		//
		//D3D12_RESOURCE_DESC textureUploadResourceDesc{};
		//textureUploadResourceDesc.Alignment = 0;
		//textureUploadResourceDesc.DepthOrArraySize = 1;
		//textureUploadResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		//textureUploadResourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
		//textureUploadResourceDesc.Format = DXGI_FORMAT_UNKNOWN;
		//textureUploadResourceDesc.Height = 1;
		//textureUploadResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		//textureUploadResourceDesc.MipLevels = 1;
		//textureUploadResourceDesc.SampleDesc = { 1,0 };
		//textureUploadResourceDesc.Width = texSizeBytes; // assumes 256-byte aligned

		/*hr = device->CreateCommittedResource(
			&textureUploadHeapProperties,
			D3D12_HEAP_FLAG_NONE,
			&textureUploadResourceDesc,
			D3D12_RESOURCE_STATE_COMMON,
			nullptr,
			IID_PPV_ARGS(&textureUploadHeap));

		PROMPTFAIL(hr, "Failed to create committed resource for texture upload heap");*/

		//bool CreateVertexBuffer()
		//{
		//	HRESULT hr;
		//
		//	// CPU side access (via write-combine?)
		//	CD3DX12_HEAP_PROPERTIES verticesUploadHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
		//
		//	// only GPU has accesss
		//	CD3DX12_HEAP_PROPERTIES verticesDefaultHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
		//
		//	/*uploadHeapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;
		//	uploadHeapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_WRITE_COMBINE;
		//	uploadHeapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_L0;*/
		//
		//	// Can also use CD3DX12_RESOURCE_DESC::Buffer() for a one line alternative
		//	D3D12_RESOURCE_DESC vertexBufferResourceDesc =
		//	{
		//		D3D12_RESOURCE_DIMENSION_BUFFER,  // Dimension
		//		0,  // Alignment
		//		vertexBufferSize,  // Width
		//		1,  // Height
		//		1,  // DepthOrArraySize
		//		1, // MipLevels
		//		DXGI_FORMAT_UNKNOWN,  // Format
		//		{1, 0}, // DXGI_SAMPLE_DESC {Sample Count, Sample Quality)
		//		D3D12_TEXTURE_LAYOUT_ROW_MAJOR,  // Layout
		//		D3D12_RESOURCE_FLAG_NONE // Flags
		//	};
		//
		//	// If this was a render target or depth stencil, we could set this value to the value that the 
		//	// depth/stencil buffer or render target would usually get cleared to. 
		//	// The GPU can do some optimizations to increase the performance of clearing the resource. 
		//	// Our resource is a vertex buffer, so we set this value to nullptr
		//	D3D12_CLEAR_VALUE* clearValue = nullptr;
		//
		//	hr = device->CreateCommittedResource(
		//		&verticesUploadHeapProperties,
		//		D3D12_HEAP_FLAG_NONE,
		//		&vertexBufferResourceDesc,
		//		D3D12_RESOURCE_STATE_GENERIC_READ,
		//		nullptr,
		//		IID_PPV_ARGS(&verticesUploadHeap));
		//
		//	verticesUploadHeap->SetName("Upload Heap Vertex Buffer");
		//
		//	PROMPTFAIL(hr, "Failed to create upload heap resource");
		//
		//	hr = device->CreateCommittedResource(
		//		&verticesDefaultHeapProperties,
		//		D3D12_HEAP_FLAG_NONE,
		//		&vertexBufferResourceDesc,
		//		D3D12_RESOURCE_STATE_COPY_DEST,
		//		nullptr,
		//		IID_PPV_ARGS(&verticesDefaultHeap));
		//
		//	verticesDefaultHeap->SetName("Default Heap Vertex Buffer");
		//
		//	PROMPTFAIL(hr, "Failed to create default heap resource");
		//
		//	D3D12_SUBRESOURCE_DATA subResourceData{};
		//	subResourceData.pData = reinterpret_cast<BYTE*>(vList);
		//	subResourceData.RowPitch = vertexBufferSize;
		//	subResourceData.SlicePitch = vertexBufferSize;
		//
		//	hr = commandList->Reset(commandAllocators[g_frameIndex], nullptr);
		//
		//	PROMPTFAIL(hr, "Failed to reset command list with error ");
		//
		//	UpdateSubresources(commandList, verticesDefaultHeap, verticesUploadHeap, 0, 0, 1, &subResourceData);
		//
		//	D3D12_RESOURCE_BARRIER defaultHeapTransitionBarrier = CD3DX12_RESOURCE_BARRIER::Transition(verticesDefaultHeap,
		//		D3D12_RESOURCE_STATE_COPY_DEST,
		//		D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
		//
		//	commandList->ResourceBarrier(1, &defaultHeapTransitionBarrier);
		//
		//	commandList->Close();
		//	ID3D12CommandList* ppCommandList[] = { commandList };
		//	commandQueue->ExecuteCommandLists(_countof(ppCommandList), ppCommandList);
		//
		//	fenceValuesCPU[g_frameIndex]++;
		//	hr = commandQueue->Signal(fencesGPU[g_frameIndex], fenceValuesCPU[g_frameIndex]);
		//
		//	PROMPTFAIL(hr, "Failed to set signal for fence during default heap setup");
		//
		//	vertexBufferView.BufferLocation = verticesDefaultHeap->GetGPUVirtualAddress();
		//	vertexBufferView.StrideInBytes = sizeof(Vertex);
		//	vertexBufferView.SizeInBytes = vertexBufferSize;
		//
		//	return true;
		//}
		//
		//bool CreateIndexBuffer()
		//{
		//	DWORD indexBuffer[numVertices];
		//
		//	for (int i = 0; i < numVertices; i++)
		//	{
		//		indexBuffer[i] = i;
		//	}
		//
		//	indexBufferSize = sizeof(indexBuffer);
		//
		//	HRESULT hr;
		//
		//	CD3DX12_HEAP_PROPERTIES uploadHeapDesc = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
		//	D3D12_RESOURCE_DESC uploadHeapResourceDesc = D3D12_RESOURCE_DESC();
		//	uploadHeapResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		//	uploadHeapResourceDesc.SampleDesc = { 1,0 };
		//	uploadHeapResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		//	uploadHeapResourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
		//	uploadHeapResourceDesc.Width = indexBufferSize;
		//	uploadHeapResourceDesc.Height = 1;
		//	uploadHeapResourceDesc.DepthOrArraySize = 1;
		//	uploadHeapResourceDesc.Alignment = 0;
		//	uploadHeapResourceDesc.Format = DXGI_FORMAT_UNKNOWN;
		//	uploadHeapResourceDesc.MipLevels = 1;
		//
		//	//D3D12_RESOURCE_DESC uploadHeapResourceDesc2 = CD3DX12_RESOURCE_DESC::Buffer(indexBufferSize);
		//
		//	hr = device->CreateCommittedResource(
		//		&uploadHeapDesc,
		//		D3D12_HEAP_FLAG_NONE,
		//		&uploadHeapResourceDesc,
		//		D3D12_RESOURCE_STATE_GENERIC_READ,
		//		nullptr,
		//		IID_PPV_ARGS(&indicesUploadHeap)
		//	);
		//
		//	indicesUploadHeap->SetName("Upload Heap Index Buffer");
		//
		//	PROMPTFAIL(hr, "Failed to create upload heap for index buffer");
		//
		//	CD3DX12_HEAP_PROPERTIES defaultHeapDesc = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
			//D3D12_RESOURCE_DESC defaultHeapResourceDesc = D3D12_RESOURCE_DESC();
			//defaultHeapResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
			//defaultHeapResourceDesc.SampleDesc = { 1,0 };
			//defaultHeapResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
			//defaultHeapResourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
			//defaultHeapResourceDesc.Width = indexBufferSize;
			//defaultHeapResourceDesc.Height = 1;
			//defaultHeapResourceDesc.DepthOrArraySize = 1;
			//defaultHeapResourceDesc.Alignment = 0;
			//defaultHeapResourceDesc.Format = DXGI_FORMAT_UNKNOWN;
			//defaultHeapResourceDesc.MipLevels = 1;
		//
		//	//D3D12_RESOURCE_DESC defaultHeapResourceDesc2 = CD3DX12_RESOURCE_DESC::Buffer(indexBufferSize);
		//
		//	hr = device->CreateCommittedResource(
		//		&defaultHeapDesc,
		//		D3D12_HEAP_FLAG_NONE,
		//		&defaultHeapResourceDesc,
		//		D3D12_RESOURCE_STATE_COPY_DEST,
		//		nullptr,
		//		IID_PPV_ARGS(&indicesDefaultHeap)
		//	);
		//
		//	indicesDefaultHeap->SetName("Default Heap Index Buffer");
		//
		//	PROMPTFAIL(hr, "Failed to create default heap for index buffer");
		//
		//	hr = commandList->Reset(commandAllocators[g_frameIndex], 0);
		//
		//	PROMPTFAIL(hr, "Failed to reset command list when setting up index buffer");
		//
		//	D3D12_SUBRESOURCE_DATA indexBufferSubresourceData{};
		//	indexBufferSubresourceData.pData = reinterpret_cast<BYTE*>(indexBuffer);
		//	indexBufferSubresourceData.RowPitch = indexBufferSize;
		//	indexBufferSubresourceData.SlicePitch = indexBufferSize;
		//
		//	UpdateSubresources(commandList, indicesDefaultHeap, indicesUploadHeap, 0, 0, 1, &indexBufferSubresourceData);
		//
		//	D3D12_RESOURCE_BARRIER defaultHeapIndexBufferBarrier = CD3DX12_RESOURCE_BARRIER::Transition(
		//		indicesDefaultHeap,
		//		D3D12_RESOURCE_STATE_COPY_DEST,
		//		D3D12_RESOURCE_STATE_INDEX_BUFFER);
		//	commandList->ResourceBarrier(1, &defaultHeapIndexBufferBarrier);
		//
		//	commandList->Close();
		//
		//	ID3D12CommandList* ppCommandLists[] = { commandList };
		//	commandQueue->ExecuteCommandLists(1, ppCommandLists);
		//
		//	fenceValuesCPU[g_frameIndex]++;
		//	hr = commandQueue->Signal(fencesGPU[g_frameIndex], fenceValuesCPU[g_frameIndex]);
		//	PROMPTFAIL(hr, "Failed to signal command list when setting up index buffer");
		//
		//	indexBufferView.BufferLocation = indicesDefaultHeap->GetGPUVirtualAddress();
		//	indexBufferView.Format = DXGI_FORMAT_R32_UINT;
		//	indexBufferView.SizeInBytes = indexBufferSize;
		//
		//	return true;
		//}

		// Describes our vertices inside the vertex buffer to the input assembler.
		// (Can also be used to describe indices of the vertices via an index buffer
		//bool CreateInputLayout()
		//{
		//	//D3D12_INPUT_ELEMENT_DESC inputElementDesc[1];
		//	//inputElementDesc[0].Format = DXGI_FORMAT_R32G32B32_FLOAT;
		//	//inputElementDesc[0].SemanticName = "POSITION";
		//	//
		//	//// 0 is the first slot. 
		//	//// You may bind multiple vertex buffers to the input assembler. 
		//	//// Each vertex buffer is bound to a slot.
		//	//inputElementDesc[0].InputSlot = 0;
		//
		//	//inputElementDesc[0].InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
		//	//
		//	//// Number of instances to draw before going to the next element
		//	//// Since we set our input slot class to be PER_VERTEX_DATA, this will be 0
		//	//inputElementDesc[0].InstanceDataStepRate = 0;
		//
		//	//// This is the offset in bytes from the beginning of the vertex structure to the start of this attribute. 
		//	//// The first attribute will always be 0 here. 
		//	//// We only have one attribute, position, so we set this to 0. 
		//	//// But if we had another, like "COLOR", 
		//	//// then we will need to set this to 12 for the color element. 
		//	//// because we have 3 floats for the position, each of them is 4 bytes, so 4x3 is 12. 
		//	//inputElementDesc[0].AlignedByteOffset = 0;
		//
		//	inputLayoutList[0] = { 
		//		"POSITION", 
		//		0, 
		//		DXGI_FORMAT_R32G32B32_FLOAT, 
		//		0, 
		//		0, 
		//		D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 
		//		0 };
		//	
		//	inputLayoutList[1].SemanticName = "COLOR";
		//	inputLayoutList[1].AlignedByteOffset = 12;
		//	inputLayoutList[1].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
		//	inputLayoutList[1].InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
		//
		//	inputLayoutList[2].SemanticName = "TEXCOORD";
		//	inputLayoutList[2].AlignedByteOffset = 28;
		//	inputLayoutList[2].Format = DXGI_FORMAT_R32G32_FLOAT;
		//	inputLayoutList[2].InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
		//
		//	// we can get the number of elements in an array by "sizeof(array) / sizeof(arrayElementType)"
		//	inputLayout.NumElements = sizeof(inputLayoutList) / sizeof(D3D12_INPUT_ELEMENT_DESC);
		//	inputLayout.pInputElementDescs = inputLayoutList;
		//
		//	return true;
		//}
		//byte colorRand = 0;
		//int rowRand = 0;
		//byte* subArray;
		//void SetTextures()
		//{
		//	colorRand = std::rand() % 256;
		//	rowRand = std::rand() % (texHeight);
		//	//std::cout << std::to_string(indexRand) << " with color " << std::to_string(colorRand) << "\n";
		//	//paddedTextureBuffer[indexRand] = colorRand;
		//
		//	//memset(&paddedTextureBuffer[indexRand], colorRand, 256);
		//
		//	PIXBeginEvent(PIX_COLOR_DEFAULT, "Memset & padding");
		//	textureBox.top = rowRand;
		//	textureBox.bottom = rowRand + 1;
		//	memset(&textureBuffer[rowRand * texWidth * numChannels], colorRand, texWidth * numChannels);
		//	memcpy(&subArray[0], &textureBuffer[rowRand * texWidth * numChannels], texWidth * numChannels);
		//	//memcpy(&paddedTextureBuffer[rowPitch * rowRand], &textureBuffer[rowRand * texWidth * numChannels], texWidth * numChannels);
		//	PIXEndEvent();
		//	/*colorRand++;
		//
		//	if (colorRand >= 255)
		//	{
		//		colorRand = colorRand % 255;
		//	}*/
		//}

		//void InitTextures()
		//{
		//	textureBuffer = new byte[textureBufferSize];
		//	paddedTextureBuffer = new byte[paddedTextureBufferSize];
		//	subArray = new byte[texWidth * numChannels];
		//	std::srand(std::time(nullptr));
		//
		//	int currentPixel = 0;
		//	for (int i = 0; i < textureBufferSize - 4; i += 4)
		//	{
		//		bool isEven = (currentPixel % 2 == 0);
		//		byte color = isEven ? 255 : 0;
		//		textureBuffer[i + 0] = color; //i
		//		textureBuffer[i + 1] = color; //i
		//		textureBuffer[i + 2] = color; //i
		//		textureBuffer[i + 3] = 255;
		//		currentPixel++;
		//	}
		//
		//	for (int row = 0; row < texHeight; row++)
		//	{
		//		memcpy(&paddedTextureBuffer[rowPitch * row], &textureBuffer[row * texWidth * numChannels], texWidth * numChannels);
		//	}
		//
		//	SetTextures();
		//}
		//
		//bool InitTextureResources()
		//{
		//	HRESULT hr;
		//
		//	D3D12_HEAP_PROPERTIES textureUploadHeapProperties{ D3D12_HEAP_TYPE_UPLOAD };
		//
		//	DXGI_SAMPLE_DESC sampleDesc{};
		//	sampleDesc.Count = 1;
		//	sampleDesc.Quality = 0;
		//
		//	D3D12_RESOURCE_DESC textureUploadResourceDesc{};
		//	textureUploadResourceDesc.Alignment = 0;
		//	textureUploadResourceDesc.Width = paddedTextureBufferSize;
		//	textureUploadResourceDesc.Height = 1;
		//	textureUploadResourceDesc.DepthOrArraySize = 1;
		//	textureUploadResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		//	textureUploadResourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
		//	textureUploadResourceDesc.Format = DXGI_FORMAT_UNKNOWN;
		//	textureUploadResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		//	textureUploadResourceDesc.MipLevels = 1;
		//	textureUploadResourceDesc.SampleDesc = sampleDesc;
		//
		//	hr = device->CreateCommittedResource(
		//		&textureUploadHeapProperties,
		//		D3D12_HEAP_FLAG_NONE,
		//		&textureUploadResourceDesc,
		//		D3D12_RESOURCE_STATE_COMMON,
		//		nullptr,
		//		IID_PPV_ARGS(&textureUploadHeap));
		//
		//	PROMPTFAIL(hr, "Failed to create committed texture upload resource");
		//
		//	D3D12_HEAP_PROPERTIES textureDefaultHeapProperties{};
		//#ifdef USE_BAR
		//	textureDefaultHeapProperties.Type = D3D12_HEAP_TYPE_GPU_UPLOAD;
		//#else
		//	textureDefaultHeapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;
		//#endif
		//
		//	D3D12_RESOURCE_DESC textureDefaultResourceDesc{};
		//	textureDefaultResourceDesc.Alignment = 0;
		//	textureDefaultResourceDesc.DepthOrArraySize = 1;
		//	textureDefaultResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		//	textureDefaultResourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
		//	textureDefaultResourceDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		//	textureDefaultResourceDesc.Height = texHeight;
		//	textureDefaultResourceDesc.Width = texWidth;
		//	textureDefaultResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
		//	textureDefaultResourceDesc.MipLevels = 1;
		//	textureDefaultResourceDesc.SampleDesc = sampleDesc;
		//
		//	hr = device->CreateCommittedResource(
		//		&textureDefaultHeapProperties,
		//		D3D12_HEAP_FLAG_NONE,
		//		&textureDefaultResourceDesc,
		//		D3D12_RESOURCE_STATE_COMMON,
		//		nullptr,
		//		IID_PPV_ARGS(&textureDefaultHeap));
		//
		//	PROMPTFAIL(hr, "Failed to create committed texture default resource");
		//
		//	return true;
		//}
		//
		//bool UploadTextureBAR()
		//{
			//textureDefaultHeap->Map(0, nullptr, nullptr);
			////HRESULT hr = textureDefaultHeap->WriteToSubresource(0, &textureBox, paddedTextureBuffer, rowPitch, 1);
			//HRESULT hr = textureDefaultHeap->WriteToSubresource(0, &textureBox, subArray, rowPitch, 1);
			//textureDefaultHeap->Unmap(0, nullptr);
		//	PROMPTFAIL(hr, "Failed to directly write to subresource from CPU to GPU");
		//	return true;
		//}
		//
		//bool UploadTexture()
		//{
		//	HRESULT hr;
		//
		//	BYTE* pTextureUploadHeap{};
		//	textureUploadHeap->Map(0, nullptr, reinterpret_cast<void**>(&pTextureUploadHeap));
		//
		//	memcpy(pTextureUploadHeap, paddedTextureBuffer, paddedTextureBufferSize);
		//
		//	textureUploadHeap->Unmap(0, nullptr);
		//
		//	//---------------Texture Src (buffer) Footprints------------------------
		//	D3D12_SUBRESOURCE_FOOTPRINT textureSrcFootprint{};
		//	textureSrcFootprint.Depth = 1;
		//	textureSrcFootprint.Width = texWidth;
		//	textureSrcFootprint.Height = texHeight;
		//	textureSrcFootprint.RowPitch = rowPitch;
		//	textureSrcFootprint.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		//
		//	D3D12_PLACED_SUBRESOURCE_FOOTPRINT textureSrcPlacedFootprint{};
		//	textureSrcPlacedFootprint.Footprint = textureSrcFootprint;
		//	textureSrcPlacedFootprint.Offset = 0;
		//
		//	D3D12_TEXTURE_COPY_LOCATION textureSrcCopyLocation{};
		//	textureSrcCopyLocation.PlacedFootprint = textureSrcPlacedFootprint;
		//	textureSrcCopyLocation.pResource = textureUploadHeap;
		//	textureSrcCopyLocation.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
		//
		//	D3D12_TEXTURE_COPY_LOCATION textureDstCopyLocation{};
		//	textureDstCopyLocation.SubresourceIndex = 0;
		//	textureDstCopyLocation.pResource = textureDefaultHeap;
		//	textureDstCopyLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
		//
		//	//commandList->Reset(commandAllocators[g_frameIndex], pipelineStateObject);
		//
		//	commandList->CopyTextureRegion(&textureDstCopyLocation, 0, 0, 0, &textureSrcCopyLocation, nullptr);
		//
		//	//commandList->Close();
		//
		//	/*ID3D12CommandList* ppCommandLists[] = { commandList };
		//	commandQueue->ExecuteCommandLists(1, ppCommandLists);
		//	fenceValuesCPU[g_frameIndex]++;
		//	hr = commandQueue->Signal(fencesGPU[g_frameIndex], fenceValuesCPU[g_frameIndex]);
		//
		//	PROMPTFAIL(hr, "Failed to set signal for texture upload");*/
		//
		//	//----- SRV for Texture in Default Heap--------------
		//
		//	return true;
		//}
		//
		//bool CreateTextureResourceView()
		//{
		//	D3D12_TEX2D_SRV textureBufferSRV{};
		//	textureBufferSRV.MipLevels = 1;
		//
		//	D3D12_SHADER_RESOURCE_VIEW_DESC textureResourceViewDesc{};
		//	textureResourceViewDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		//	textureResourceViewDesc.Texture2D = textureBufferSRV;
		//	textureResourceViewDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		//	textureResourceViewDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		//
		//	D3D12_DESCRIPTOR_HEAP_DESC textureSrvDescriptorHeapDesc{};
		//	textureSrvDescriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		//	textureSrvDescriptorHeapDesc.NodeMask = 0;
		//	textureSrvDescriptorHeapDesc.NumDescriptors = 1;
		//	textureSrvDescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		//
		//	HRESULT hr = device->CreateDescriptorHeap(&textureSrvDescriptorHeapDesc, IID_PPV_ARGS(&textureSRVDescriptorHeap));
		//
		//	PROMPTFAIL(hr, "Failed to create texture SRV descriptor heap");
		//
		//	device->CreateShaderResourceView(
		//		textureDefaultHeap,
		//		&textureResourceViewDesc,
		//		textureSRVDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
		//
		//	return true;
		//}