#include "Macros.h"
#include "RenderingEngineD3D12.h"

bool RenderingEngineD3D12::Init(Scene& scene)
{
	CHECK_FAIL(CreateFactory());
	CHECK_FAIL(CreateDevice()); // needs factory
	CHECK_FAIL(CreateCommandAllocators()); // needs m_device
	CHECK_FAIL(CreateCommandQueue()); // needs m_device
	CHECK_FAIL(CreateSwapChain()); // needs factory and m_device and command queue
	CHECK_FAIL(CreateRTVAndDescriptorHeap()); // needs m_device and swapchain
	CHECK_FAIL(CreateDepthStencilBuffer()); // needs m_device
	CHECK_FAIL(CreateCommandList()); // needs m_device and command allocator
	CHECK_FAIL(CreateFences()); // needs m_device

	//CHECK_FAIL(CompileShaders());
	CreateViewport();

	m_commandList->Reset(m_commandAllocators[m_currentFrameIndex].Get(), nullptr);

	if (m_benchmarker)
	{
		D3D12_QUERY_HEAP_DESC timestampQueryHeapDesc{};
		timestampQueryHeapDesc.Type = D3D12_QUERY_HEAP_TYPE_TIMESTAMP;
		timestampQueryHeapDesc.Count = 2;
		HRESULT hr = m_device->CreateQueryHeap(&timestampQueryHeapDesc, IID_PPV_ARGS(m_benchmarker->TimestampQueryHeap.GetAddressOf()));
		PROMPTFAILHR(hr, "Failed to create timestamp query heap. ");

		m_commandQueue->GetTimestampFrequency(&m_benchmarker->GpuTimestampFrequency);
		std::cout << "GPU Frequency: " << m_benchmarker->GpuTimestampFrequency << "\n";

		D3D12_HEAP_PROPERTIES timestampDataResourceHeapProperties{ D3D12_HEAP_TYPE_READBACK };
		D3D12_RESOURCE_DESC timestampDataResourceDesc{};
		timestampDataResourceDesc.Alignment = 0;
		timestampDataResourceDesc.DepthOrArraySize = 1;
		timestampDataResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		timestampDataResourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
		timestampDataResourceDesc.Format = DXGI_FORMAT_UNKNOWN;
		timestampDataResourceDesc.Height = 1;
		timestampDataResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		timestampDataResourceDesc.MipLevels = 1;
		timestampDataResourceDesc.SampleDesc = { 1, 0 };
		timestampDataResourceDesc.Width = 2 * sizeof(UINT64);
		m_device->CreateCommittedResource(
			&timestampDataResourceHeapProperties,
			D3D12_HEAP_FLAG_NONE,
			&timestampDataResourceDesc,
			D3D12_RESOURCE_STATE_COMMON,
			nullptr,
			IID_PPV_ARGS(&m_benchmarker->TimestampDataResource)
		);
	}

	// Uploading all resources for base models. These are shared by each object of a given model
	std::vector<Model>& models = scene.GetModels();
	for (Model& model : models)
	{
		const ModelBinData* modelBinData = model.GetBinData();
		CHECK_FAIL(CreateBufferResource(model.ModelBinResource, modelBinData->binDataSize));
		CHECK_FAIL(UploadBuffer(model.ModelBinResource.Get(), modelBinData->binData, modelBinData->binDataSize));

		std::vector<Mesh>& modelMeshes = model.GetMeshes();

		for (UINT meshIndex = 0; meshIndex < modelMeshes.size(); meshIndex++)
		{
			Mesh& mesh = modelMeshes[meshIndex];

			std::cout << "Setting up GPU resources for model " << model.Name << " mesh: " << mesh.Name << "\n";

			// These need the model binary's address so that the buffer view's location can be assigned
			// that's why we set them here instead of in the Model constructor
			model.SetMeshVertexBufferViews(meshIndex);
			model.SetMeshIndexBufferView(meshIndex);

			for (int i = 0; i < mesh.Primitives.size(); i++)
			{
				MeshPrimitive& meshPrimitive = mesh.Primitives[i];
				std::vector<Texture>& meshPrimitiveTextures = meshPrimitive.Textures;
				for (Texture& texture : meshPrimitiveTextures)
				{
					CHECK_FAIL(CreateTextureResource(texture));
					CHECK_FAIL(UploadTexture(texture));
				}
			}
		}
	}

	m_commandList->Close();
	ID3D12CommandList* commandLists[]{ m_commandList.Get() };
	m_commandQueue->ExecuteCommandLists(_countof(commandLists), commandLists);

	m_fenceValuesCPU[m_currentFrameIndex]++;
	HRESULT hr2 = m_commandQueue->Signal(m_fencesGPU[m_currentFrameIndex].Get(), m_fenceValuesCPU[m_currentFrameIndex]);
	PROMPTFAILHR(hr2, "Failed to signal fence with error ");

	std::cout << "\n========== Engine Initialized ==============\n";

	return true;
}

bool RenderingEngineD3D12::CreateFactory()
{
	HRESULT hr;

	hr = CreateDXGIFactory1(IID_PPV_ARGS(&m_dxgiFactory));

	PROMPTFAILHR(hr, "Create DXGI Factory failed");

	return true;
}


bool RenderingEngineD3D12::CreateDevice()
{
	HRESULT hr{};

#ifdef _DEBUG
	ComPtr<ID3D12Debug> debugController;
	if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(debugController.GetAddressOf()))))
	{
		debugController->EnableDebugLayer();
		std::cout << "Debug Controller active\n";
	}
#endif

	// find the right adapter by checking to see if we can create a m_device with it
	IDXGIAdapter1* adapter;
	UINT adapterId = 0;
	bool adapterFound = false;
	while (m_dxgiFactory->EnumAdapters1(adapterId, &adapter) != DXGI_ERROR_NOT_FOUND)
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

		// use the adapter to try and create a m_device
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
		D3D12CreateDevice(adapter, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(m_device.GetAddressOf()));
		return true;
	}
	else
	{
		std::cout << "ERROR: Failed to find suitable adapter to create device";
		return false;
	}
}


bool RenderingEngineD3D12::CreateCommandQueue()
{
	HRESULT hr;
	D3D12_COMMAND_QUEUE_DESC commandQueueDesc{};
	commandQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	commandQueueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
	commandQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	commandQueueDesc.NodeMask = 0;

	hr = m_device->CreateCommandQueue(&commandQueueDesc, IID_PPV_ARGS(m_commandQueue.GetAddressOf()));

	PROMPTFAILHR(hr, "Failed to create direct command queue with error ");

	return true;
}

bool RenderingEngineD3D12::CreateSwapChain()
{
	HRESULT hr;
	DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
	swapChainDesc.BufferCount = kFrameBufferCount;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.Height = m_renderWindow.Height;
	swapChainDesc.Width = m_renderWindow.Width;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD; // flip is faster as it avoids an extra copy with DWM
	swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;
	swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.Scaling = DXGI_SCALING_NONE;
	swapChainDesc.SampleDesc = { 1, 0 }; // minimum 1 needed for sampling the back buffer


	//DXGI_MODE_DESC backBufferDesc = {};
	//backBufferDesc.Width = m_renderWindow.Width; // 0 defaults to window size
	//backBufferDesc.Height = m_renderWindow.Height;  // 0 defaults to window size
	//backBufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	/*swapChainDesc.BufferDesc = backBufferDesc;*/

	DXGI_SWAP_CHAIN_FULLSCREEN_DESC fullScreenDesc{};

	IDXGISwapChain1* tempSwapChain{};
	hr = m_dxgiFactory->CreateSwapChainForHwnd(m_commandQueue.Get(), m_renderWindow.WindowHandle, &swapChainDesc, NULL, NULL, &tempSwapChain);

	PROMPTFAILHR(hr, "Failed to create swap chain");

	m_swapChain = static_cast<IDXGISwapChain3*>(tempSwapChain);
	m_currentFrameIndex = m_swapChain->GetCurrentBackBufferIndex();

	return true;
}

bool RenderingEngineD3D12::CreateRTVAndDescriptorHeap()
{
	HRESULT hr;

	D3D12_DESCRIPTOR_HEAP_DESC rtvDescriptorHeapDesc;
	rtvDescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	rtvDescriptorHeapDesc.NumDescriptors = kFrameBufferCount; // one render target for each back buffer
	rtvDescriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	rtvDescriptorHeapDesc.NodeMask = 0;

	hr = m_device->CreateDescriptorHeap(&rtvDescriptorHeapDesc, IID_PPV_ARGS(m_rtvDescriptorHeap.GetAddressOf()));

	PROMPTFAILHR(hr, "Failed to Create RTV Descriptor Heap with error ");

	m_rtvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	// Why can't I use pointer math to get the handle of the first descriptor from the heap pointer itself?
	// The heap contains metadata about the heap, so you don't know where the descriptor begins
	// The heap owns descriptor storage; the runtime maps that storage to 
	// CPU/GPU-visible address spaces and hands you opaque handles when you ask.
	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvDescriptorHandle{ m_rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart() };

	for (int i = 0; i < kFrameBufferCount; i++)
	{
		// Get the swap chain buffer locations
		hr = m_swapChain->GetBuffer(i, IID_PPV_ARGS(m_renderTargets[i].GetAddressOf()));

		{
			const std::string msg = "Failed to Get swap chain buffer " + std::to_string(i);
			PROMPTFAILHR(hr, msg.c_str());
		}

		// Assign the rtv descriptor handle to the swap chain buffers
		m_device->CreateRenderTargetView(m_renderTargets[i].Get(), nullptr, rtvDescriptorHandle);

		rtvDescriptorHandle.Offset(1, m_rtvDescriptorSize);
	}
	return true;
}

bool RenderingEngineD3D12::CreateDepthStencilBuffer()
{
	HRESULT hr;

	D3D12_HEAP_PROPERTIES dsBufferProperties{ D3D12_HEAP_TYPE_DEFAULT };

	D3D12_RESOURCE_DESC dsBufferResourceDesc
		= CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_D32_FLOAT, m_renderWindow.Width, m_renderWindow.Height, 1, 0, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);
	/*dsBufferResourceDesc.Alignment = 0;
	dsBufferResourceDesc.DepthOrArraySize = 1;
	dsBufferResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	dsBufferResourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
	dsBufferResourceDesc.Format = DXGI_FORMAT_D32_FLOAT;
	dsBufferResourceDesc.Height = Height;
	dsBufferResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	dsBufferResourceDesc.MipLevels = 1;
	dsBufferResourceDesc.SampleDesc = { 1, 0 };
	dsBufferResourceDesc.Width = Width;*/

	D3D12_CLEAR_VALUE dsClearValue{};
	dsClearValue.DepthStencil.Depth = 1.0f;
	dsClearValue.DepthStencil.Stencil = 0;
	dsClearValue.Format = DXGI_FORMAT_D32_FLOAT;

	hr = m_device->CreateCommittedResource(
		&dsBufferProperties,
		D3D12_HEAP_FLAG_NONE,
		&dsBufferResourceDesc,
		D3D12_RESOURCE_STATE_DEPTH_WRITE,
		&dsClearValue,
		IID_PPV_ARGS(&m_depthStencilBuffer));

	PROMPTFAILHR(hr, "Failed to create Depth Stencil Buffer Resource. ");

	D3D12_DESCRIPTOR_HEAP_DESC dsBufferDescriptorHeapDesc{};
	dsBufferDescriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	dsBufferDescriptorHeapDesc.NumDescriptors = 1;
	dsBufferDescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;

	hr = m_device->CreateDescriptorHeap(&dsBufferDescriptorHeapDesc, IID_PPV_ARGS(m_depthStencilDescriptorHeap.GetAddressOf()));
	m_depthStencilDescriptorHeap->SetName(L"Depth Stencil Descriptor Heap");

	PROMPTFAILHR(hr, "Failed to create descriptor heap for depth stencil resource");

	D3D12_DEPTH_STENCIL_VIEW_DESC dsViewDesc{};
	dsViewDesc.Format = DXGI_FORMAT_D32_FLOAT;
	dsViewDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	dsViewDesc.Flags = D3D12_DSV_FLAG_NONE;

	m_device->CreateDepthStencilView(m_depthStencilBuffer.Get(), &dsViewDesc, m_depthStencilDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

	return true;
}

bool RenderingEngineD3D12::CreateCommandAllocators()
{
	HRESULT hr;

	// Need as many as frame buffer count since we can't reset while one is executing, and there will be
	// multiple executing simultaneously as we attempt to fill the backbuffers while the front is scanning out
	for (int i = 0; i < kFrameBufferCount; i++)
	{
		hr = m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(m_commandAllocators[i].GetAddressOf()));

		PROMPTFAILHR(hr, "Failed to Create Command Allocator at index " + std::to_string(i));
	}

	return true;
}

bool RenderingEngineD3D12::CreateCommandList()
{
	HRESULT hr;

	hr = m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_commandAllocators[m_currentFrameIndex].Get(), NULL, IID_PPV_ARGS(m_commandList.GetAddressOf()));

	PROMPTFAILHR(hr, "Failed to create command list");

	m_commandList->Close();

	return true;
}

bool RenderingEngineD3D12::CreateFences()
{
	HRESULT hr;

	// Need as many fences as the frame buffer count (since single threaded)
	for (int i = 0; i < kFrameBufferCount; i++)
	{
		hr = m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(m_fencesGPU[i].GetAddressOf()));

		const std::string msg = "Failed to Create Fence for frame buffer " + std::to_string(i);
		PROMPTFAILHR(hr, msg.c_str());

		m_fenceValuesCPU[i] = 0;
	}

	m_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
	if (m_fenceEvent == FALSE)
	{
		return false;
	}

	return true;
}

void RenderingEngineD3D12::CreateViewport()
{
	// Fill out the Viewport
	m_viewport.TopLeftX = 0;
	m_viewport.TopLeftY = 0;
	m_viewport.Width = m_renderWindow.Width;
	m_viewport.Height = m_renderWindow.Height;
	m_viewport.MinDepth = 0.01f;
	m_viewport.MaxDepth = 1.0f;

	// Fill out a scissor rect
	m_scissorRect.left = 0;
	m_scissorRect.top = 0;
	m_scissorRect.right = m_renderWindow.Width;
	m_scissorRect.bottom = m_renderWindow.Height;
}

bool RenderingEngineD3D12::CreateRootSignature(const MeshPrimitive& meshPrimitive)
{
	std::vector<D3D12_ROOT_PARAMETER> rootParams{};
	//rootParams.reserve(2);

	D3D12_DESCRIPTOR_RANGE rootWVPMatricesDescRange{};
	rootWVPMatricesDescRange.BaseShaderRegister = 0; //b0
	rootWVPMatricesDescRange.NumDescriptors = 1;
	rootWVPMatricesDescRange.OffsetInDescriptorsFromTableStart = 0;
	rootWVPMatricesDescRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;

	D3D12_ROOT_DESCRIPTOR_TABLE rootWVPMatricesDescTable{};
	rootWVPMatricesDescTable.NumDescriptorRanges = 1;
	rootWVPMatricesDescTable.pDescriptorRanges = &rootWVPMatricesDescRange;

	D3D12_ROOT_PARAMETER rootParamWVP{};
	rootParamWVP.DescriptorTable = rootWVPMatricesDescTable;
	rootParamWVP.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParamWVP.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	rootParams.push_back(rootParamWVP);

	// ------------- texture srv -----------------------------------------------------------
	if (meshPrimitive.Textures.size() > 0)
	{
		D3D12_DESCRIPTOR_RANGE rootTextSrvDescriptorRange{};
		rootTextSrvDescriptorRange.BaseShaderRegister = 0; //t0
		rootTextSrvDescriptorRange.NumDescriptors = static_cast<UINT>(meshPrimitive.Textures.size());
		rootTextSrvDescriptorRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
		rootTextSrvDescriptorRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;

		D3D12_ROOT_DESCRIPTOR_TABLE rootTexSrvDescriptorTable{};
		rootTexSrvDescriptorTable.NumDescriptorRanges = 1;
		rootTexSrvDescriptorTable.pDescriptorRanges = &rootTextSrvDescriptorRange;

		D3D12_ROOT_PARAMETER rootParamTextures{};
		rootParamTextures.DescriptorTable = rootTexSrvDescriptorTable;
		rootParamTextures.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		rootParamTextures.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
		rootParams.push_back(rootParamTextures);
	}

	D3D12_STATIC_SAMPLER_DESC textureSampler{}; // needs to be accessible rootSignatureDesc so has to be in the same scope as it

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
	D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc{};
	rootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
	rootSignatureDesc.NumParameters = rootParams.size();
	rootSignatureDesc.pParameters = rootParams.data();

	if (meshPrimitive.Textures.size() > 0)
	{
		textureSampler.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
		textureSampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		textureSampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		textureSampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		textureSampler.ShaderRegister = 0; //s0
		textureSampler.RegisterSpace = 0;
		rootSignatureDesc.NumStaticSamplers = 1;
		rootSignatureDesc.pStaticSamplers = &textureSampler;
	}

	ComPtr<ID3DBlob> signature{};
	ComPtr<ID3DBlob> errorBlob{};
	HRESULT hr = D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, signature.GetAddressOf(), errorBlob.GetAddressOf());

	if (FAILED(hr))
	{
		if (errorBlob)
		{
			const char* errorMsg = (const char*)errorBlob->GetBufferPointer();
			MessageBoxA(0, errorMsg, "Error", MB_OK);
		}
	}
	PROMPTFAILHR(hr, "Failed to assign root signature blob! ");

	hr = m_device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(m_rootSignature.GetAddressOf()));

	PROMPTFAILHR(hr, "Failed to create root signature! ");

	return true;
}


// Needs root signature and input layout
bool RenderingEngineD3D12::CreatePipelineStateObject(const MeshPrimitive& meshPrimitive)
{
	HRESULT hr;
	D3D12_GRAPHICS_PIPELINE_STATE_DESC graphicsPipelineStateDesc{};
	graphicsPipelineStateDesc.InputLayout = meshPrimitive.InputLayout;
	graphicsPipelineStateDesc.pRootSignature = m_rootSignature.Get();
	graphicsPipelineStateDesc.VS = { meshPrimitive.Shaders[0].BinData, meshPrimitive.Shaders[0].BinSize };
	graphicsPipelineStateDesc.PS = { meshPrimitive.Shaders[1].BinData, meshPrimitive.Shaders[1].BinSize };
	graphicsPipelineStateDesc.NumRenderTargets = 1;
	graphicsPipelineStateDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	graphicsPipelineStateDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	graphicsPipelineStateDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	graphicsPipelineStateDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);

	D3D12_DEPTH_STENCIL_DESC dsDesc{};
	dsDesc.DepthEnable = TRUE;
	dsDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
	dsDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	dsDesc.StencilEnable = FALSE;
	dsDesc.StencilReadMask = D3D12_DEFAULT_STENCIL_READ_MASK;
	dsDesc.StencilWriteMask = D3D12_DEFAULT_STENCIL_WRITE_MASK;
	const D3D12_DEPTH_STENCILOP_DESC defaultStencilOp =
	{ D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_COMPARISON_FUNC_ALWAYS };
	dsDesc.FrontFace = defaultStencilOp;
	dsDesc.BackFace = defaultStencilOp;

	graphicsPipelineStateDesc.DepthStencilState = dsDesc;
	graphicsPipelineStateDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
	graphicsPipelineStateDesc.SampleMask = 0xffffff; // point sampling
	graphicsPipelineStateDesc.SampleDesc = { 1, 0 };

	// When using triangle strip primitive topology, vertex positions are interpreted as vertices of a continuous triangle “strip”.
	// There is a special index value that represents the desire to have a discontinuity in the strip, the cut index value. 
	// This enum lists the supported cut values.
	//graphicsPipelineStateDesc.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;

	hr = m_device->CreateGraphicsPipelineState(&graphicsPipelineStateDesc, IID_PPV_ARGS(m_pipelineStateObject.GetAddressOf()));

	PROMPTFAILHR(m_device->GetDeviceRemovedReason(), "Failed to create graphics pipeline state object. ");

	return true;
}


// Assumes buffer is n*256-byte aligned and the resource property is UPLOAD/GPU_UPLOAD
bool RenderingEngineD3D12::UploadBuffer(ID3D12Resource* bufferResource, byte* bufferData, size_t bufferSize)
{
	void* mapped = nullptr;
	D3D12_RANGE readRange = { 0, 0 }; // CPU will not read
	HRESULT hr = bufferResource->Map(0, &readRange, &mapped);
	PROMPTFAILHR(hr, "Failed to map buffer resource");

	memcpy(mapped, bufferData, bufferSize);

	D3D12_RANGE writtenRange = { 0, bufferSize };

	bufferResource->Unmap(0, &writtenRange);

	return true;
}

bool RenderingEngineD3D12::WaitForPreviousFrame()
{
	HRESULT hr;

	m_currentFrameIndex = m_swapChain->GetCurrentBackBufferIndex();

	if (m_fencesGPU[m_currentFrameIndex]->GetCompletedValue() < m_fenceValuesCPU[m_currentFrameIndex])
	{
		hr = m_fencesGPU[m_currentFrameIndex]->SetEventOnCompletion(m_fenceValuesCPU[m_currentFrameIndex], m_fenceEvent);

		PROMPTFAILHR(hr, "Failed to set event on completion for frame index " + std::to_string(m_currentFrameIndex));

		hr = WaitForSingleObject(m_fenceEvent, INFINITE);

		PROMPTFAILHR(hr, "Failed to wait for fence event for frame index " + std::to_string(m_currentFrameIndex));
	}

	m_fenceValuesCPU[m_currentFrameIndex]++;

	return true;

}

bool RenderingEngineD3D12::UpdatePipeline(Scene& scene)
{
	WaitForPreviousFrame();

	HRESULT hr = m_commandAllocators[m_currentFrameIndex]->Reset();

	PROMPTFAILHR(hr, "Failed to reset command allocator");

	// reset, now ready for recording
	hr = m_commandList->Reset(m_commandAllocators[m_currentFrameIndex].Get(), m_pipelineStateObject.Get());
	PROMPTFAILHR(hr, "Failed to reset command LIST");

	if (m_benchmarker)
	{
		m_commandList->EndQuery(m_benchmarker->TimestampQueryHeap.Get(), D3D12_QUERY_TYPE_TIMESTAMP, 0);
	}

	// change from present state to render target state for recording
	D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
		m_renderTargets[m_currentFrameIndex].Get(),
		D3D12_RESOURCE_STATE_PRESENT,
		D3D12_RESOURCE_STATE_RENDER_TARGET
	);
	m_commandList->ResourceBarrier(1, &barrier);

	// need to get the descriptor handle so we can set it as the render target in output merger stage of pipeline
	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvDescriptorHandle
	{
		m_rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart() ,
		m_currentFrameIndex,
		m_rtvDescriptorSize
	};

	CD3DX12_CPU_DESCRIPTOR_HANDLE dsDescriptorHandle
	{
		m_depthStencilDescriptorHeap->GetCPUDescriptorHandleForHeapStart()
	};

	// If RTsSingleHandleToDescriptorRange (3rd arg) is TRUE, 
	// pRenderTargetDescriptors (2nd arg) points to a contiguous descriptor range (GPU offsets by descriptor size); 
	// if FALSE, it points to an array of handles (extra indirection, less efficient).
	m_commandList->OMSetRenderTargets(1, &rtvDescriptorHandle, FALSE, &dsDescriptorHandle);

	// set color of render target when clearing it
	const float clearColor[] = { 0.3f, 0.3f, 0.3f, 1.0f };
	m_commandList->ClearRenderTargetView(rtvDescriptorHandle, clearColor, 0, nullptr);
	m_commandList->ClearDepthStencilView(m_depthStencilDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

	m_commandList->RSSetViewports(1, &m_viewport);
	m_commandList->RSSetScissorRects(1, &m_scissorRect);
	m_commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	//std::vector<ID3D12DescriptorHeap*>* textureSRVDescriptorHeaps = new std::vector<ID3D12DescriptorHeap*>();
	std::vector<ModelInstance>& objects = scene.GetObjects();
	for (ModelInstance& object : objects)
	{
		std::vector<Mesh>& modelMeshes = object.BaseModel->GetMeshes();
		std::vector<WorldNode>& objectWorldNodes = object.GetNodes();
		for (Mesh& mesh : modelMeshes)
		{
			WorldNode& nodeWithMesh = objectWorldNodes[mesh.NodeIndex];
			//std::cout << mesh.Name << "Mesh Primitives Size: " << mesh.Primitives.size() << "\n";
			for (int i = 0; i < mesh.Primitives.size(); i++)
			{
				// Graphics update
				MeshPrimitive& meshPrimitive = mesh.Primitives[i];
				CHECK_FAIL(CreateRootSignature(meshPrimitive));
				CHECK_FAIL(CreatePipelineStateObject(meshPrimitive));
				m_commandList->SetGraphicsRootSignature(m_rootSignature.Get());
				m_commandList->SetPipelineState(m_pipelineStateObject.Get());

				int slot = 0;
				for (const D3D12_VERTEX_BUFFER_VIEW& vertexBufferView : meshPrimitive.VertexBufferViews)
				{
					//std::cout << "Slot: " << slot << " is at " << vertexBufferView.BufferLocation <<"\n";
					m_commandList->IASetVertexBuffers(slot, 1, &vertexBufferView);
					slot++;
				}
				m_commandList->IASetIndexBuffer(&meshPrimitive.IndexBufferView);

				m_commandList->SetDescriptorHeaps(1, nodeWithMesh.PrimitiveShaderVisibleDescriptorHeaps[i].GetAddressOf());

				D3D12_GPU_DESCRIPTOR_HANDLE descriptorHandle = nodeWithMesh.PrimitiveShaderVisibleDescriptorHeaps[i]->GetGPUDescriptorHandleForHeapStart();
				m_commandList->SetGraphicsRootDescriptorTable(0, descriptorHandle);

				if (meshPrimitive.Textures.size() > 0)
				{
					descriptorHandle.ptr += m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
					m_commandList->SetGraphicsRootDescriptorTable(1, descriptorHandle);
				}

				m_commandList->DrawIndexedInstanced(meshPrimitive.NumIndices, 1, 0, 0, 0);

				if (m_benchmarker)
				{
					m_benchmarker->SSData.NumDrawCalls[m_benchmarker->CurrentMeasurementFrameNumber]++;
				}
			}
		}
	}

	// change resource state back to present state for rendering
	D3D12_RESOURCE_BARRIER barrier2 = CD3DX12_RESOURCE_BARRIER::Transition(
		m_renderTargets[m_currentFrameIndex].Get(),
		D3D12_RESOURCE_STATE_RENDER_TARGET,
		D3D12_RESOURCE_STATE_PRESENT
	);
	m_commandList->ResourceBarrier(1, &barrier2);

	if (m_benchmarker)
	{
		m_commandList->EndQuery(m_benchmarker->TimestampQueryHeap.Get(), D3D12_QUERY_TYPE_TIMESTAMP, 1);

		D3D12_RESOURCE_BARRIER queryBarrier = CD3DX12_RESOURCE_BARRIER::Transition(
			m_benchmarker->TimestampDataResource.Get(),
			D3D12_RESOURCE_STATE_COMMON,
			D3D12_RESOURCE_STATE_COPY_DEST
		);
		m_commandList->ResourceBarrier(1, &queryBarrier);

		m_commandList->ResolveQueryData(m_benchmarker->TimestampQueryHeap.Get(), D3D12_QUERY_TYPE_TIMESTAMP, 0, 2, m_benchmarker->TimestampDataResource.Get(), 0);

		void* mapped = nullptr;
		D3D12_RANGE timestampRangeBegin{ 0,2 * sizeof(UINT64) };
		m_benchmarker->TimestampDataResource->Map(0, &timestampRangeBegin, &mapped);

		UINT64* mappedValues = (UINT64*)mapped;
		double gpuTimeFrame = 1000.0 * (mappedValues[1] - mappedValues[0]) / (double)m_benchmarker->GpuTimestampFrequency;
		m_benchmarker->SSData.GpuFrameTimes[m_benchmarker->CurrentMeasurementFrameNumber] = gpuTimeFrame;

	}
	hr = m_commandList->Close();

	return true;
}

bool RenderingEngineD3D12::Render(Scene& scene)
{
	if (scene.state == Scene::RESETTING)
	{
		WaitForPreviousFrame();
		for (int i = 0; i < kFrameBufferCount; i++)
		{
			const UINT64 value = ++m_fenceValuesCPU[i];
			m_commandQueue->Signal(m_fencesGPU[i].Get(), value);
			m_fencesGPU[i]->SetEventOnCompletion(value, m_fenceEvent);
			WaitForSingleObject(m_fenceEvent, INFINITE);
		}

		scene.state = Scene::READY;
		return true;
	}

	std::vector<ModelInstance>& objects = scene.GetObjects();
	for (ModelInstance& object : objects)
	{
		std::vector<WorldNode>& objectNodes = object.GetNodes();

		for (UINT nodeIndex = 0; nodeIndex < objectNodes.size(); nodeIndex++)
		{
			WorldNode& nodeWithMesh = objectNodes[nodeIndex];
			if (nodeWithMesh.MeshIndex == -1) { continue; }

			if (nodeWithMesh.WVPMatrixGPUResource.Get() == NULL)
			{
				// Aligning buffer to 256 bytes as it is refrenced by a CBV
				CHECK_FAIL(CreateBufferResource(nodeWithMesh.WVPMatrixGPUResource, ~255 & (255 + nodeWithMesh.WVPMatrixVector.size() * sizeof(float))));
				CHECK_FAIL(SetShaderVisibleDescriptors(object, nodeWithMesh));
			}

			CHECK_FAIL(UploadBuffer(nodeWithMesh.WVPMatrixGPUResource.Get(), reinterpret_cast<byte*>(nodeWithMesh.WVPMatrixVector.data()), nodeWithMesh.WVPMatrixVector.size() * sizeof(float)));
		}
	}

	HRESULT hr = S_OK;
	if (UpdatePipeline(scene) == false)
	{
		PROMPTFAILHR(hr, "Failed to record!\n");
	}

	ID3D12CommandList* commandLists[]{ m_commandList.Get() };

	m_commandQueue->ExecuteCommandLists(_countof(commandLists), commandLists);

	PROMPTFAILHR(hr, "Failed to close command list ");

	hr = m_commandQueue->Signal(m_fencesGPU[m_currentFrameIndex].Get(), m_fenceValuesCPU[m_currentFrameIndex]);

	PROMPTFAILHR(hr, "Failed to signal fence with error ");

	// presents the current back buffer
	hr = m_swapChain->Present(VSYNC, 0);

	const std::string msg = "Failed to present backbuffer at index " + std::to_string(m_currentFrameIndex);
	PROMPTFAILHR(hr, msg.c_str());

	/*if (m_benchmarker)
	{
		if (m_benchmarker->IsFirstRender)
		{
			Benchmarker::StopTime(m_benchmarker->LoadingMetricsData.LoadTimeToFirstRenderedFrame);
			m_benchmarker->IsFirstRender = false;
		}
	}*/

	return true;
}

bool RenderingEngineD3D12::Reset()
{
	WaitForPreviousFrame();

	HRESULT hr;

	// Resets write_offset = 0 (this is used for tracking what command we are on in the gpu)
	// Immediately invalidates all command streams
	// Does not wait for GPU
	hr = m_commandAllocators[m_currentFrameIndex]->Reset();

	PROMPTFAILHR(hr, "Failed to reset command allocator at frame index " + m_currentFrameIndex);

	hr = m_commandList->Reset(m_commandAllocators[m_currentFrameIndex].Get(), nullptr);

	PROMPTFAILHR(hr, "Failed to reset command list with error ");

	m_commandList->Close();

	return true;
}

bool RenderingEngineD3D12::CreateTextureResource(Texture& texture)
{
	// For UMA or NUMA, Textures should be on the DEFAULT heap (and not UPLOAD/GPU_UPLOAD) if frequently read
	// because the movement from UPLOAD->DEFAULT swizzles the texture, which improves caching

	D3D12_RESOURCE_DESC textureDefaultResourceDesc{};
	textureDefaultResourceDesc.Alignment = 0;
	textureDefaultResourceDesc.DepthOrArraySize = 1;
	textureDefaultResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	textureDefaultResourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
	textureDefaultResourceDesc.Format = texture.Format;
	textureDefaultResourceDesc.Height = texture.Height;
	textureDefaultResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	textureDefaultResourceDesc.MipLevels = 1;
	textureDefaultResourceDesc.SampleDesc = { 1,0 };
	textureDefaultResourceDesc.Width = texture.Width;

	D3D12_HEAP_PROPERTIES textureDefaultHeapProperties{ D3D12_HEAP_TYPE_GPU_UPLOAD };
	HRESULT hr = m_device->CreateCommittedResource(
		&textureDefaultHeapProperties,
		D3D12_HEAP_FLAG_NONE,
		&textureDefaultResourceDesc,
		D3D12_RESOURCE_STATE_COMMON,
		nullptr,
		IID_PPV_ARGS(texture.GPUResource.GetAddressOf()));

	PROMPTFAILHR(hr, "Failed to create committed resource for texture " + texture.Name + " default heap");

	return true;
}

bool RenderingEngineD3D12::CreateBufferResource(ComPtr<ID3D12Resource>& bufferResource, size_t bufferSize)
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

	//std::cout << "Buffer resource size: " << bufferSize << "\n";

	//D3D12_HEAP_PROPERTIES bufferHeapProperties{ D3D12_HEAP_TYPE_GPU_UPLOAD };
	CD3DX12_HEAP_PROPERTIES bufferHeapProperties(D3D12_HEAP_TYPE_UPLOAD);
	HRESULT hr = m_device->CreateCommittedResource(
		&bufferHeapProperties,
		D3D12_HEAP_FLAG_NONE,
		&bufferResourceDesc,
		D3D12_RESOURCE_STATE_COMMON,
		nullptr,
		IID_PPV_ARGS(bufferResource.GetAddressOf()));

	//std::cout << "Buffer resource when creating: " << bufferResource << "\n";
	PROMPTFAILHR(hr, "Failed to create buffer resource");

	return true;
}

// Assumes n*256-byte aligned rows in a texture
bool RenderingEngineD3D12::UploadTexture(Texture& texture, bool useWriteToSubResource)
{
	HRESULT hr;

	if (texture.GPUResource == nullptr)
	{
		std::cout << "Cannot upload texture, gpu resource does not exist!";
		return false;
	}

	if (useWriteToSubResource)
	{
		//D3D12_BOX texBox = { 0, 0, 0, texture->Width, texture->Height, 1 };
		hr = texture.GPUResource->WriteToSubresource(0, &texture.TexBox, texture.PixelData, texture.Width * texture.NumChannels, 1);

		PROMPTFAILHR(hr, "Failed to write texture " + std::string(texture.Name) + " to subresource, ");
	}
	else
	{
		PROMPTFAILHR(0, "Upload heap to default heap approach not set up for uploading textures");
	}

	return true;
}

bool RenderingEngineD3D12::SetShaderVisibleDescriptors(ModelInstance& object, WorldNode& nodeWithMesh)
{
	HRESULT hr;

	// Create the common desc for the WVP matrix
	D3D12_CONSTANT_BUFFER_VIEW_DESC wvpMatrixCBVDesc{};
	wvpMatrixCBVDesc.BufferLocation = nodeWithMesh.WVPMatrixGPUResource->GetGPUVirtualAddress();
	wvpMatrixCBVDesc.SizeInBytes = 256 * (1 + static_cast<UINT>(nodeWithMesh.WVPMatrixVector.size()) / 256);

	const std::vector<Mesh>& objectMeshes = object.BaseModel->GetMeshes();
	const Mesh& mesh = objectMeshes[nodeWithMesh.MeshIndex];
	int numMeshPrimitives = mesh.Primitives.size();
	nodeWithMesh.PrimitiveShaderVisibleDescriptorHeaps.resize(numMeshPrimitives);
	for (int i = 0; i < numMeshPrimitives; i++)
	{
		const MeshPrimitive& meshPrimitive = mesh.Primitives[i];

		// Create the descriptor heap with the correct size to hold descriptors for all mesh resources -----------------------------------------
		D3D12_DESCRIPTOR_HEAP_DESC mainSrvDescriptorHeapDesc{};
		mainSrvDescriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		mainSrvDescriptorHeapDesc.NumDescriptors = static_cast<UINT>(meshPrimitive.Textures.size()) + 1; // +1 for mesh's WVP matrix
		mainSrvDescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		hr = m_device->CreateDescriptorHeap(&mainSrvDescriptorHeapDesc, IID_PPV_ARGS(nodeWithMesh.PrimitiveShaderVisibleDescriptorHeaps[i].GetAddressOf()));
		PROMPTFAILHR(hr, "Failed to create main descriptor heap for mesh " + std::string(mesh.Name) + " primitive at index " + std::to_string(i));

		// Create the handle inside the heap (where the buffer view goes)
		UINT descriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		int descriptorNumber = 0;
		CD3DX12_CPU_DESCRIPTOR_HANDLE wvpMatrixCBVHandle(
			nodeWithMesh.PrimitiveShaderVisibleDescriptorHeaps[i]->GetCPUDescriptorHandleForHeapStart(),
			descriptorNumber, descriptorSize); // offsets the Heapstart by the descriptorNumber*descriptorSize

		// Create the CBV for each object's WVP matrix
		m_device->CreateConstantBufferView(&wvpMatrixCBVDesc, wvpMatrixCBVHandle);

		// Setup SRV for shader interaction with Resource -------------------------------------
		D3D12_TEX2D_SRV texture2DSrvMips{};
		texture2DSrvMips.MipLevels = 1;

		D3D12_SHADER_RESOURCE_VIEW_DESC textureSrvDesc{};
		textureSrvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		textureSrvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		textureSrvDesc.Texture2D = texture2DSrvMips;
		textureSrvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;

		for (const Texture& texture : meshPrimitive.Textures)
		{
			descriptorNumber++;
			CD3DX12_CPU_DESCRIPTOR_HANDLE textureSRVHandle(
				nodeWithMesh.PrimitiveShaderVisibleDescriptorHeaps[i]->GetCPUDescriptorHandleForHeapStart(),
				descriptorNumber, descriptorSize);

			m_device->CreateShaderResourceView(
				texture.GPUResource.Get(),
				&textureSrvDesc,
				textureSRVHandle
			);
		}
	}
	return true;
}

RenderingEngineD3D12::RenderingEngineD3D12(RenderWindow& renderWindow, std::shared_ptr<Benchmarker> benchmarker, GraphicsAPI graphicsAPI)
	: m_renderWindow(renderWindow), m_benchmarker(benchmarker), m_graphicsAPI(graphicsAPI)
{
}


bool RenderingEngineD3D12::Shutdown()
{
	HRESULT hr;

	WaitForPreviousFrame();

	for (int i = 0; i < kFrameBufferCount; i++)
	{
		const UINT64 value = ++m_fenceValuesCPU[i];
		m_commandQueue->Signal(m_fencesGPU[i].Get(), value);
		m_fencesGPU[i]->SetEventOnCompletion(value, m_fenceEvent);
		WaitForSingleObject(m_fenceEvent, INFINITE);
	}

	// close the fence event
	CloseHandle(m_fenceEvent);

	m_commandList->Close();

	//ID3D12CommandList* lists[] = { m_commandList.Get() };
	//m_commandQueue->ExecuteCommandLists(1, lists);

	//m_fenceValuesCPU[m_currentFrameIndex]++;
	//hr = m_commandQueue->Signal(m_fencesGPU[m_currentFrameIndex].Get(), m_fenceValuesCPU[m_currentFrameIndex]);
	//PROMPTFAILHR(hr, "Failed to signal fence with error ");

	//for (int i = 0; i < kFrameBufferCount; i++)
	//{
	//	m_commandList->Reset(m_commandAllocators[i].Get(), nullptr);
	//}

	//if (m_commandList)
	//{
	//	m_commandList->ClearState(nullptr);
	//}
	// The command list should no longer retain recorded state.

	// Delete application-owned objects while the m_device still exists.
	/*for (ModelInstance* object : m_objects)
	{
		delete object;
	}
	m_objects.clear();

	for (Model* model : m_models)
	{
		delete model;
	}
	m_models.clear();*/

	// Release swap-chain buffers before destroying the swap chain.
	//for (int i = 0; i < kFrameBufferCount; ++i)
	//{
	//	SAFE_RELEASE(m_renderTargets[i].Get());
	//}

	BOOL isFullscreen = FALSE;

	if (m_swapChain)
	{
		hr = m_swapChain->GetFullscreenState(&isFullscreen, nullptr);

		if (SUCCEEDED(hr) && isFullscreen)
		{
			m_swapChain->SetFullscreenState(FALSE, nullptr);
		}
	}


	/*SAFE_RELEASE(m_swapChain);

	for (int i = 0; i < kFrameBufferCount; ++i)
	{
		SAFE_RELEASE(m_commandAllocators[i]);
	}

	SAFE_RELEASE(m_pipelineStateObject);
	SAFE_RELEASE(m_rootSignature);
	SAFE_RELEASE(m_rtvDescriptorHeap);

	for (int i = 0; i < kFrameBufferCount; ++i)
	{
		SAFE_RELEASE(m_fencesGPU[i]);
	}

	SAFE_RELEASE(m_commandQueue);
	SAFE_RELEASE(m_device);*/

	return true;
}
