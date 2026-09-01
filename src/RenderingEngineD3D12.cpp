#include "Macros.h"
#include "RenderingEngineD3D12.h"

void PixBeginEventCustom(UINT32 pixColor, const char* name)
{
#if PIX_ENABLED 1
	PixBeginEvent(pixColor, name);
#endif
}

void PixEndEventCustom()
{
#if PIX_ENABLED 1
	PixEndEvent();
#endif
}

bool RenderingEngineD3D12::Init(Scene& scene)
{
	HRESULT hr;
	CHECK_FAIL(CreateFactory(), "Failed to Create Factory");
	CHECK_FAIL(CreateDevice(), "Failed to CreateDevice"); // needs factory
	CHECK_FAIL(CreateCommandAllocators(), "Failed to CreateCommandAllocators"); // needs m_device
	CHECK_FAIL(CreateCommandQueue(), "Failed to CreateCommandQueue"); // needs m_device
	CHECK_FAIL(CreateSwapChain(), "Failed to CreateSwapChain"); // needs factory and m_device and command queue
	CHECK_FAIL(CreateRTVAndDescriptorHeap(), "Failed to CreateRTVAndDescriptorHeap"); // needs m_device and swapchain
	CHECK_FAIL(CreateDepthStencilBuffer(), "Failed to CreateDepthStencilBuffer"); // needs m_device
	CHECK_FAIL(CreateCommandList(), "Failed to CreateCommandList"); // needs m_device and command allocator
	CHECK_FAIL(CreateFences(), "Failed to CreateFences"); // needs m_device

	//CHECK_FAIL(CompileShaders());
	CreateViewport();

	m_commandList->Reset(m_commandAllocators[m_currentFrameIndex].Get(), nullptr);

	if (m_benchmarker)
	{
		D3D12_QUERY_HEAP_DESC timestampQueryHeapDesc{};
		timestampQueryHeapDesc.Type = D3D12_QUERY_HEAP_TYPE_TIMESTAMP;
		timestampQueryHeapDesc.Count = 2;
		hr = m_device->CreateQueryHeap(&timestampQueryHeapDesc, IID_PPV_ARGS(m_benchmarker->TimestampQueryHeap.GetAddressOf()));
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

	// Uploading all resources for base models. These are shared by each model of a given model
	Camera& camera = scene.GetCamera();
	CHECK_FAIL(CreateBufferResource(camera.VPMatrixResource, ALIGN_256(MATRIX4X4_NUMELEMENTS * sizeof(float))), "Failed to create VP Matrix Resource");
	CHECK_FAIL(UploadBuffer(camera.VPMatrixResource.Get(), reinterpret_cast<byte*> (camera.GetVPMatrixBuffer().data()), MATRIX4X4_NUMELEMENTS * sizeof(float)), "Failed to uplpoad camera VP buffer")

		std::vector<Model>& models = scene.GetModels();
	for (Model& model : models)
	{
		const ModelBinData* modelBinData = model.GetBinData();
		CHECK_FAIL(CreateBufferResource(model.ModelBinResource, modelBinData->binDataSize), "Failed to CreateBufferResource for ModelBin, Model: " + model.Name);
		CHECK_FAIL(UploadBuffer(model.ModelBinResource.Get(), modelBinData->binData, modelBinData->binDataSize), "Failed to UploadBuffer for ModelBin, Model: " + model.Name);

		// Create the resource to store root node transform data for all instances contiguously (used by shader later when formulating the wvp)
		// Uploading only in Init() since instances are not moving
		CHECK_FAIL(CreateBufferResource(model.WorldRootTransformBuffersAllInstancesResource, ALIGN_256(model.WorldRootTransformBuffersAllInstances.size() * MATRIX4X4_NUMELEMENTS * sizeof(float))), "Failed to CreateBufferResource for WorldRootTransformBuffersAllInstancesResource, Model: " + model.Name);
		UploadBuffer(model.WorldRootTransformBuffersAllInstancesResource.Get(),
			reinterpret_cast<byte*>(model.WorldRootTransformBuffersAllInstances.data()),
			ALIGN_256(model.WorldRootTransformBuffersAllInstances.size() * MATRIX4X4_NUMELEMENTS * sizeof(float))
		);

		std::vector<Mesh>& modelMeshes = model.GetMeshes();
		std::vector<Node>& modelNodes = model.GetNodesModelSpace();
		for (UINT meshIndex = 0; meshIndex < modelMeshes.size(); meshIndex++)
		{
			Mesh& mesh = modelMeshes[meshIndex];
			Node& meshNode = modelNodes[mesh.NodeIndex];

			std::cout << "Setting up GPU resources for model " << model.Name << " mesh: " << mesh.Name << "\n";

			// These need the model binary's address so that the buffer view's location can be assigned
			// that's why we set them here instead of in the Model constructor
			model.SetMeshVertexBufferViews(meshIndex);
			model.SetMeshIndexBufferView(meshIndex);

			for (int i = 0; i < mesh.Primitives.size(); i++)
			{
				std::cout << "Mesh Index: " << i << "\n";
				MeshPrimitive& meshPrimitive = mesh.Primitives[i];
				std::vector<Texture>& meshPrimitiveTextures = meshPrimitive.Textures;
				// This is redundant if there are multiple mesh primitives per mesh since they would all share a common node
				CHECK_FAIL(CreateBufferResource(meshPrimitive.MeshPrimitiveModelSpaceTransformBufferResource, ALIGN_256(MATRIX4X4_NUMELEMENTS * sizeof(float))), "Failed to Create buffer for MeshPrimitiveModelSpaceTransformBufferResource, Model: " + model.Name + ", MeshIndex: " + std::to_string(i));
				CHECK_FAIL(UploadBuffer(
					meshPrimitive.MeshPrimitiveModelSpaceTransformBufferResource.Get(),
					reinterpret_cast<byte*>(meshNode.NodeTransform.GetTransformMatrixArray().data()),
					ALIGN_256(MATRIX4X4_NUMELEMENTS * sizeof(float))
				), "Failed to Upload buffer for MeshPrimitiveModelSpaceTransformBufferResource, Model: " + model.Name + ", MeshIndex: " + std::to_string(i)); // Currently parts of a model are not moving w.r.t each other, so we assume these are constant by uploading them once in Init() 

				// Upload the color factors data
				CHECK_FAIL(CreateBufferResource(meshPrimitive.ColorFactorsResource, ALIGN_256(sizeof(ColorFactors))), "Failed to create color factors resource, Model: " + model.Name + ", MeshIndex: " + std::to_string(i));
				CHECK_FAIL(UploadBuffer(
					meshPrimitive.ColorFactorsResource.Get(),
					reinterpret_cast<byte*>(&meshPrimitive.ColorFactorsData),
					ALIGN_256(sizeof(ColorFactors))), "");

				// Create Descriptor Heap -----------------------------------------------------------------
				D3D12_DESCRIPTOR_HEAP_DESC primitiveShaderVisibleDescriptor_HeapDesc{};
				primitiveShaderVisibleDescriptor_HeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
				primitiveShaderVisibleDescriptor_HeapDesc.NodeMask = 0;
				primitiveShaderVisibleDescriptor_HeapDesc.NumDescriptors = meshPrimitive.Textures.size() + 4; // Num of textures used by this mesh + 4 (MeshPrimitiveModelSpaceTransformBufferResource, WorldRootTransformBuffersAllInstancesResource, VPMatrixTransformBuffer, ColorFactorsData 
				primitiveShaderVisibleDescriptor_HeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
				m_device->CreateDescriptorHeap(
					&primitiveShaderVisibleDescriptor_HeapDesc,
					IID_PPV_ARGS(meshPrimitive.PrimitiveShaderVisibleDescriptorHeap.GetAddressOf())
				);

				// Create and Add VP Matrix Descriptor (CBV) to each DescriptorHeap ----------------------------
				D3D12_CPU_DESCRIPTOR_HANDLE descriptorHeapCPUHandle{ meshPrimitive.PrimitiveShaderVisibleDescriptorHeap.Get()->GetCPUDescriptorHandleForHeapStart() };
				D3D12_CONSTANT_BUFFER_VIEW_DESC vpMatrixConstantBufferViewDesc{};
				vpMatrixConstantBufferViewDesc.BufferLocation = scene.GetCamera().VPMatrixResource->GetGPUVirtualAddress();
				vpMatrixConstantBufferViewDesc.SizeInBytes = ALIGN_256(MATRIX4X4_NUMELEMENTS * sizeof(float));
				m_device->CreateConstantBufferView(&vpMatrixConstantBufferViewDesc, descriptorHeapCPUHandle);

				// Set CBV for ModelMatrix ------------------------------------------------------------------------------------------
				descriptorHeapCPUHandle.ptr += DescriptorHandleIncrementSizeCBVSRVUAV;
				D3D12_CONSTANT_BUFFER_VIEW_DESC modelMatrix_CBVDesc{};
				modelMatrix_CBVDesc.BufferLocation = meshPrimitive.MeshPrimitiveModelSpaceTransformBufferResource.Get()->GetGPUVirtualAddress();
				modelMatrix_CBVDesc.SizeInBytes = ALIGN_256(MATRIX4X4_NUMELEMENTS * sizeof(float));
				m_device->CreateConstantBufferView(&modelMatrix_CBVDesc, descriptorHeapCPUHandle);

				// Set CBV for ColorFactorsData --------------------------------------------------------------------------------------------------
				descriptorHeapCPUHandle.ptr += DescriptorHandleIncrementSizeCBVSRVUAV;
				D3D12_CONSTANT_BUFFER_VIEW_DESC colorFactorsCBVDesc{};
				colorFactorsCBVDesc.BufferLocation = meshPrimitive.ColorFactorsResource.Get()->GetGPUVirtualAddress();
				colorFactorsCBVDesc.SizeInBytes = ALIGN_256(sizeof(ColorFactors));
				m_device->CreateConstantBufferView(&colorFactorsCBVDesc, descriptorHeapCPUHandle);

				// SRV for WorldRootTransformBuffersAllInstances ---------------------------------------------------------------------------------------
				descriptorHeapCPUHandle.ptr += DescriptorHandleIncrementSizeCBVSRVUAV;
				D3D12_BUFFER_SRV worldRootTransformBuffersAllInstances_BufferSRV{};
				worldRootTransformBuffersAllInstances_BufferSRV.FirstElement = 0;
				worldRootTransformBuffersAllInstances_BufferSRV.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
				worldRootTransformBuffersAllInstances_BufferSRV.NumElements = model.WorldRootTransformBuffersAllInstances.size();
				worldRootTransformBuffersAllInstances_BufferSRV.StructureByteStride = MATRIX4X4_NUMELEMENTS * sizeof(float);

				D3D12_SHADER_RESOURCE_VIEW_DESC worldRootTransformBuffersAllInstances_SRVDesc{};
				worldRootTransformBuffersAllInstances_SRVDesc.Buffer = worldRootTransformBuffersAllInstances_BufferSRV;
				worldRootTransformBuffersAllInstances_SRVDesc.Format = DXGI_FORMAT_UNKNOWN;
				worldRootTransformBuffersAllInstances_SRVDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
				worldRootTransformBuffersAllInstances_SRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

				m_device->CreateShaderResourceView(
					model.WorldRootTransformBuffersAllInstancesResource.Get(),
					&worldRootTransformBuffersAllInstances_SRVDesc,
					descriptorHeapCPUHandle
				);

				// Create, Upload, and SRVs for textures ------------------------------------------------------------------------------------------------------------------------
				std::vector<Texture>& textures = meshPrimitive.Textures;
				for (Texture& texture : textures)
				{
					CHECK_FAIL(CreateTextureResource(texture), "Failed to CreateTextureResource, Model: " + model.Name + ", texture: " + std::to_string(texture.Type));
					CHECK_FAIL(UploadTexture(texture), "Failed to UploadTexture, Model: " + model.Name + ", texture: " + std::to_string(texture.Type));

					descriptorHeapCPUHandle.ptr += DescriptorHandleIncrementSizeCBVSRVUAV;

					D3D12_TEX2D_SRV tex_Tex2DSRV{};
					tex_Tex2DSRV.MipLevels = 1;

					D3D12_SHADER_RESOURCE_VIEW_DESC tex_SRVDesc{};
					tex_SRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
					tex_SRVDesc.Texture2D = tex_Tex2DSRV;
					tex_SRVDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
					tex_SRVDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;

					m_device->CreateShaderResourceView(texture.GPUResource.Get(), &tex_SRVDesc, descriptorHeapCPUHandle);
				}

				CHECK_FAIL(model.CreateRootSignature(meshPrimitive, m_device), "Failed to CreateRootSignature for Mesh Primitive, Model: " + model.Name + ", MeshIndex: " + std::to_string(i));
				CHECK_FAIL(model.CreatePipelineStateObject(meshPrimitive, meshPrimitive.RootSignature, m_device), "Failed to CreatePipelineStateObject for  Mesh Primitive, Model: " + model.Name + ", MeshIndex: " + std::to_string(i));
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

// Assumes no new models added, no new cameras added/changed. Only changes are in the number of instances of existing models
bool RenderingEngineD3D12::ResetSceneForInstances(Scene& scene)
{
	std::vector<Model>& models = scene.GetModels();
	for (Model& model : models)
	{
		// Reset the resource to be used again with new numbers of elements
		model.WorldRootTransformBuffersAllInstancesResource.Reset();
		CHECK_FAIL(CreateBufferResource(model.WorldRootTransformBuffersAllInstancesResource, ALIGN_256(model.WorldRootTransformBuffersAllInstances.size() * MATRIX4X4_NUMELEMENTS * sizeof(float))), "Failed to CreateBufferResource for WorldRootTransformBuffersAllInstancesResource, Model: " + model.Name);
		UploadBuffer(model.WorldRootTransformBuffersAllInstancesResource.Get(),
			reinterpret_cast<byte*>(model.WorldRootTransformBuffersAllInstances.data()),
			ALIGN_256(model.WorldRootTransformBuffersAllInstances.size() * MATRIX4X4_NUMELEMENTS * sizeof(float))
		);

		std::vector<Mesh>& modelMeshes = model.GetMeshes();
		std::vector<Node>& modelNodes = model.GetNodesModelSpace();
		for (UINT meshIndex = 0; meshIndex < modelMeshes.size(); meshIndex++)
		{
			Mesh& mesh = modelMeshes[meshIndex];
			Node& meshNode = modelNodes[mesh.NodeIndex];

			//std::cout << "Setting up new instances for " << model.Name << " mesh: " << mesh.Name << "\n";

			for (int i = 0; i < mesh.Primitives.size(); i++)
			{
				MeshPrimitive& meshPrimitive = mesh.Primitives[i];

				// We can re-use the existing descriptor heap since the number of descriptors in the heap isn't changing

				// Create and Add VP Matrix Descriptor (CBV) to each DescriptorHeap ----------------------------
				D3D12_CPU_DESCRIPTOR_HANDLE descriptorHeapCPUHandle{ meshPrimitive.PrimitiveShaderVisibleDescriptorHeap.Get()->GetCPUDescriptorHandleForHeapStart() };

				// Camera, ModelMatrix CBVs and Texture SRVs do not change since they are tied to the model, only the instance descriptor heap changes

				// SRV for WorldRootTransformBuffersAllInstances ---------------------------------------------------------------------------------------
				descriptorHeapCPUHandle.ptr += DescriptorHandleIncrementSizeCBVSRVUAV * 3;
				D3D12_BUFFER_SRV worldRootTransformBuffersAllInstances_BufferSRV{};
				worldRootTransformBuffersAllInstances_BufferSRV.FirstElement = 0;
				worldRootTransformBuffersAllInstances_BufferSRV.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
				worldRootTransformBuffersAllInstances_BufferSRV.NumElements = model.WorldRootTransformBuffersAllInstances.size();
				worldRootTransformBuffersAllInstances_BufferSRV.StructureByteStride = MATRIX4X4_NUMELEMENTS * sizeof(float);

				D3D12_SHADER_RESOURCE_VIEW_DESC worldRootTransformBuffersAllInstances_SRVDesc{};
				worldRootTransformBuffersAllInstances_SRVDesc.Buffer = worldRootTransformBuffersAllInstances_BufferSRV;
				worldRootTransformBuffersAllInstances_SRVDesc.Format = DXGI_FORMAT_UNKNOWN;
				worldRootTransformBuffersAllInstances_SRVDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
				worldRootTransformBuffersAllInstances_SRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

				m_device->CreateShaderResourceView(
					model.WorldRootTransformBuffersAllInstancesResource.Get(),
					&worldRootTransformBuffersAllInstances_SRVDesc,
					descriptorHeapCPUHandle
				);

				// Should not need to change root signature since number of descriptor tables/root params and layout is same
				// CHECK_FAIL(model.CreateRootSignature(meshPrimitive, m_device), "Failed to CreateRootSignature for Mesh Primitive, Model: " + model.Name + ", MeshIndex: " + std::to_string(i));
				// CHECK_FAIL(model.CreatePipelineStateObject(meshPrimitive, meshPrimitive.RootSignature, m_device), "Failed to CreatePipelineStateObject for  Mesh Primitive, Model: " + model.Name + ", MeshIndex: " + std::to_string(i));
			}
		}
	}
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
		DescriptorHandleIncrementSizeCBVSRVUAV = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
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

// Assumes buffer is n*256-byte aligned and the resource property is UPLOAD/GPU_UPLOAD
bool RenderingEngineD3D12::UploadBuffer(ID3D12Resource* bufferResource, byte* bufferData, size_t bufferSizeBytes)
{
	void* mapped = nullptr;
	D3D12_RANGE readRange = { 0, 0 }; // CPU will not read
	HRESULT hr = bufferResource->Map(0, &readRange, &mapped);
	PROMPTFAILHR(hr, "Failed to map buffer resource");

	memcpy(mapped, bufferData, bufferSizeBytes);

	D3D12_RANGE writtenRange = { 0, bufferSizeBytes };

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
	PixBeginEventCustom(PIX_COLOR(255, 0, 0), "WaitForPreviousFrame");
	WaitForPreviousFrame();
	PixEndEventCustom();

	HRESULT hr = m_commandAllocators[m_currentFrameIndex]->Reset();

	PROMPTFAILHR(hr, "Failed to reset command allocator");

	// reset, now ready for recording
	hr = m_commandList->Reset(m_commandAllocators[m_currentFrameIndex].Get(), nullptr);
	PROMPTFAILHR(hr, "Failed to reset command LIST");

	if (m_benchmarker)
	{
		m_commandList->EndQuery(m_benchmarker->TimestampQueryHeap.Get(), D3D12_QUERY_TYPE_TIMESTAMP, 0);
	}

	PixBeginEventCustom(PIX_COLOR(0, 255, 0), "Rasterizer and Depth Setup");
	// change from present state to render target state for recording
	D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
		m_renderTargets[m_currentFrameIndex].Get(),
		D3D12_RESOURCE_STATE_PRESENT,
		D3D12_RESOURCE_STATE_RENDER_TARGET
	);
	m_commandList->ResourceBarrier(1, &barrier);


	// need to get the descriptor handle so we can set it as the render target in output merger stage of pipeline
	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvDescriptorHandle{ m_rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), m_currentFrameIndex, m_rtvDescriptorSize };

	CD3DX12_CPU_DESCRIPTOR_HANDLE dsDescriptorHandle{ m_depthStencilDescriptorHeap->GetCPUDescriptorHandleForHeapStart() };

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
	PixEndEventCustom();

	//std::vector<ID3D12DescriptorHeap*>* textureSRVDescriptorHeaps = new std::vector<ID3D12DescriptorHeap*>();
	std::vector<Model>& models = scene.GetModels();

	for (Model& model : models)
	{
		std::vector<Mesh>& modelMeshes = model.GetMeshes();
		std::vector<Node>& modelNodes = model.GetNodesModelSpace();
		for (Mesh& mesh : modelMeshes)
		{
			Node& nodeWithMesh = modelNodes[mesh.NodeIndex];
			//std::cout << mesh.Name << "Mesh Primitives Size: " << mesh.Primitives.size() << "\n";
			for (int i = 0; i < mesh.Primitives.size(); i++)
			{
				// Graphics update
				PixBeginEventCustom(PIX_COLOR(0, 255, 255), "PSO Setup");
				MeshPrimitive& meshPrimitive = mesh.Primitives[i];
				m_commandList->SetGraphicsRootSignature(meshPrimitive.RootSignature.Get());
				m_commandList->SetPipelineState(meshPrimitive.PipelineStateObject.Get());

				int slot = 0;
				for (const D3D12_VERTEX_BUFFER_VIEW& vertexBufferView : meshPrimitive.VertexBufferViews)
				{
					//std::cout << "Slot: " << slot << " is at " << vertexBufferView.BufferLocation <<"\n";
					m_commandList->IASetVertexBuffers(slot, 1, &vertexBufferView);
					slot++;
				}
				m_commandList->IASetIndexBuffer(&meshPrimitive.IndexBufferView);

				m_commandList->SetDescriptorHeaps(1, meshPrimitive.PrimitiveShaderVisibleDescriptorHeap.GetAddressOf());

				D3D12_GPU_DESCRIPTOR_HANDLE descriptorHandle = meshPrimitive.PrimitiveShaderVisibleDescriptorHeap->GetGPUDescriptorHandleForHeapStart();
				m_commandList->SetGraphicsRootDescriptorTable(0, descriptorHandle);

				descriptorHandle.ptr += DescriptorHandleIncrementSizeCBVSRVUAV * 3; // since there are 3 CBVs
				m_commandList->SetGraphicsRootDescriptorTable(1, descriptorHandle);

				if (meshPrimitive.Textures.size() > 0)
				{
					descriptorHandle.ptr += DescriptorHandleIncrementSizeCBVSRVUAV;
					m_commandList->SetGraphicsRootDescriptorTable(2, descriptorHandle);
					m_commandList->SetGraphicsRoot32BitConstants(3, 3, &scene.LightDirection, 0);
				}
				else
				{
					m_commandList->SetGraphicsRoot32BitConstants(2, 3, &scene.LightDirection, 0);
				}



				PixEndEventCustom();

				PixBeginEventCustom(PIX_COLOR(0, 0, 255), "DrawIndexedInstance");
				m_commandList->DrawIndexedInstanced(
					meshPrimitive.NumIndices,
					model.WorldRootTransformBuffersAllInstances.size(),
					0, 0, 0);
				PixEndEventCustom();

				if (m_benchmarker)
				{
					m_benchmarker->SSData.NumDrawCalls[m_benchmarker->CurrentMeasurementFrameCount]++;
				}
			}
		}
	}

	PixBeginEventCustom(PIX_COLOR(255, 0, 255), "Transition Resource barrier to Present");
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
		m_benchmarker->SSData.GpuFrameTimes[m_benchmarker->CurrentMeasurementFrameCount] = gpuTimeFrame;

	}
	hr = m_commandList->Close();
	PixEndEventCustom();

	return true;
}

bool RenderingEngineD3D12::UploadSceneData(Scene& scene)
{
	// The only thing moving is the camera currently, so we only need to update the vp matrix
	Camera& camera = scene.GetCamera();
	CHECK_FAIL(UploadBuffer(camera.VPMatrixResource.Get(), reinterpret_cast<byte*>(camera.GetVPMatrixBuffer().data()), MATRIX4X4_NUMELEMENTS * sizeof(float)), "Failed to UploadBuffer for scene's VP Matrix");

	/*std::vector<Model>& models = scene.GetModels();
	for (Model& model : models)
	{
		std::vector<Node>& nodes = model.GetNodesModelSpace();
		std::vector<Mesh>& meshes = model.GetMeshes();

		for (Mesh& mesh : meshes)
		{
			Node& nodeWithMesh = nodes[mesh.NodeIndex];
			if (nodeWithMesh.MeshIndex == -1) { continue; }

			std::vector<MeshPrimitive>& meshPrimitives = mesh.Primitives;
			for (MeshPrimitive& meshPrimitive : meshPrimitives)
			{
				if (meshPrimitive.MeshPrimitiveModelSpaceTransformBufferResource.Get() == NULL)
				{


				}
			}
		}
	}*/

	return true;
}

bool RenderingEngineD3D12::Render(Scene& scene)
{
	if (scene.state == Scene::READY) // this is to get the updated scene and render the new scene accordingly
	{
		ResetSceneForInstances(scene);
		scene.state = Scene::RUNNING;
	}

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

	CHECK_FAIL(UploadSceneData(scene), "Failed to upload Scene data");

	HRESULT hr = S_OK;
	if (UpdatePipeline(scene) == false)
	{
		PROMPTFAILHR(hr, "Failed to record!\n");
	}

	ID3D12CommandList* commandLists[]{ m_commandList.Get() };

	m_commandQueue->ExecuteCommandLists(1, commandLists);

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

	PROMPTFAILHR(hr, "Failed to create committed resource for texture " + std::to_string(texture.Type) + " default heap");

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

	//std::cout << "Buffer resource size: " << bufferSizeBytes << "\n";

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

		PROMPTFAILHR(hr, "Failed to write texture " + std::to_string(texture.Type) + " to subresource, ");
	}
	else
	{
		PROMPTFAILHR(0, "Upload heap to default heap approach not set up for uploading textures");
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

	// Delete application-owned models while the m_device still exists.
	/*for (ModelInstance* model : m_objects)
	{
		delete model;
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

	SAFE_RELEASE(PipelineStateObject);
	SAFE_RELEASE(RootSignature);
	SAFE_RELEASE(m_rtvDescriptorHeap);

	for (int i = 0; i < kFrameBufferCount; ++i)
	{
		SAFE_RELEASE(m_fencesGPU[i]);
	}

	SAFE_RELEASE(m_commandQueue);
	SAFE_RELEASE(m_device);*/

	return true;
}
