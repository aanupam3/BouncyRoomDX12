#include "Macros.h"
#include "Model.h"
#include <fstream>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "d3dx12.h"

Model::Model(std::string modelBasePath, std::string name) : m_modelBasePath(modelBasePath), Name(name)
{
	m_glTFPath = modelBasePath + "/scene.gltf";
	//m_texturesFolderPath = modelBasePath + "/textures/";

	std::cout << "Creating model: " << Name << "\n";

	ExtractDataFromGLTF();
	SetMeshes();
	SetNodes();

	std::cout << m_meshes.size();
}


Model::~Model()
{
	for (int meshIndex = 0; meshIndex < m_meshes.size(); meshIndex++)
	{
		Mesh& mesh = m_meshes[meshIndex];
		for (int i = 0; i < mesh.Primitives.size(); i++)
		{
			MeshPrimitive& meshPrimitive = mesh.Primitives[i];
			for (int textureIndex = 0; textureIndex < meshPrimitive.Textures.size(); textureIndex++)
			{
				Texture& texture = meshPrimitive.Textures.at(textureIndex);

				stbi_image_free(texture.PixelData);
				texture.PixelData = nullptr;
			}

			meshPrimitive.VertexBufferViews.clear();
			meshPrimitive.Textures.clear();
		}
	}
	m_meshes.clear();
	m_nodesModelSpace.clear();

}

void Model::ExtractDataFromGLTF()
{
	std::ifstream glTFFile{ m_glTFPath };

	if (!glTFFile.is_open())
	{
		std::cout << "Could not find .gltf at path: " + m_glTFPath << "\n";
	}

	nlohmann::json parsedglTF{ nlohmann::json::parse(glTFFile) };
	m_modelJson = new ModelGLTF::ModelJson(parsedglTF.get<ModelGLTF::ModelJson>());

	m_binPath = m_modelBasePath + std::string(parsedglTF["buffers"][0]["uri"]);
	/*UINT binFileSize = m_modelJson->buffers[0].byteLength;
	m_binData = new ModelBinData(new byte[binFileSize], binFileSize);*/
	//Utils::LoadBinaryData(m_binPath, m_binData->binData, binFileSize);

	std::ifstream binFile{ m_binPath, std::ios::binary };
	if (!binFile.is_open())
	{
		std::cout << "Did not find binary at " << m_binPath << "!\n";
		return;
	}

	const UINT binFileSize = m_modelJson->buffers[0].byteLength;
	m_binData = new ModelBinData(new byte[binFileSize], binFileSize);

	binFile.seekg(0, std::ios::beg);
	binFile.read((char*)m_binData->binData, binFileSize);
}

void Model::SetMeshes()
{
	int NumMeshes = static_cast<UINT>(m_modelJson->meshes.size());
	m_meshes.resize(NumMeshes);
	for (UINT meshIndex = 0; meshIndex < NumMeshes; meshIndex++)
	{
		Mesh& mesh = m_meshes[meshIndex];
		mesh.Name = m_modelJson->meshes[meshIndex].name;

		UINT numMeshPrimitives = m_modelJson->meshes[meshIndex].primitives.size();
		mesh.Primitives.resize(numMeshPrimitives);

		SetMeshTexturesAndColors(meshIndex, mesh);
		SetMeshShaders(mesh);
	}
}

void Model::SetMeshVertexBufferViews(int meshIndex)
{
	Mesh& mesh = m_meshes[meshIndex];

	for (UINT primitiveIndex = 0; primitiveIndex < mesh.Primitives.size(); primitiveIndex++)
	{
		MeshPrimitive& meshPrimitive = mesh.Primitives[primitiveIndex];
		std::vector<D3D12_VERTEX_BUFFER_VIEW>& meshPrimitiveVBVs = meshPrimitive.VertexBufferViews;

		nlohmann::json attributes = m_modelJson->meshes[meshIndex].primitives[primitiveIndex].attributes;
		UINT numAttributes = static_cast<UINT>(attributes.size());

		// attributeName is the key, accessorIndex is the value
		int attributeIndex = 0;
		meshPrimitive.AttributeNames.resize(numAttributes);

		for (auto& [attributeName, accessorIndex] : attributes.items())
		{
			ModelGLTF::Accessor attributeAccessor = m_modelJson->accessors[accessorIndex.get<int>()];
			ModelGLTF::BufferView attributeBufferViewInfo = m_modelJson->bufferViews[attributeAccessor.bufferView];

			D3D12_VERTEX_BUFFER_VIEW vertexBufferView{};
			vertexBufferView.BufferLocation = ModelBinResource->GetGPUVirtualAddress() + attributeAccessor.byteOffset + attributeBufferViewInfo.byteOffset;
			vertexBufferView.SizeInBytes = attributeAccessor.count * attributeBufferViewInfo.byteStride;
			vertexBufferView.StrideInBytes = attributeBufferViewInfo.byteStride; // size of each vertex buffer position attribute
			meshPrimitiveVBVs.push_back(vertexBufferView);

			/*std::cout << "MeshIndex " << meshIndex << ", BufferView " << attributeName << ":\n"
			<< "Offset: " << vertexBufferView.BufferLocation << "\n"
			<< "Size: " << vertexBufferView.SizeInBytes << "\n"
			<< "Stride: " << vertexBufferView.StrideInBytes << "\n";*/

			meshPrimitive.AttributeNames[attributeIndex] = attributeName;
			UINT semanticIndex = 0;

			// Dont want the semantic name to be TEXCOORD_1 like in the glTF file, so we re-assign it explicitly here
			if (attributeName.find("TEXCOORD") != std::string::npos)
			{
				meshPrimitive.AttributeNames[attributeIndex] = "TEXCOORD";
				semanticIndex = std::stoi(attributeName.substr(9));
			}

			D3D12_INPUT_ELEMENT_DESC inputLayoutElement{};
			inputLayoutElement.AlignedByteOffset = 0;
			inputLayoutElement.InputSlot = attributeIndex;
			inputLayoutElement.InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
			inputLayoutElement.SemanticName = meshPrimitive.AttributeNames[attributeIndex].c_str();
			inputLayoutElement.SemanticIndex = semanticIndex;
			/*std::cout << "Attribute semantic name: " << meshPrimitive->AttributeNames[attributeIndex]
				<< ", semantic index: " << semanticIndex << "\n";*/

			std::string& attributeType = attributeAccessor.type;
			if (attributeType == "VEC4") { inputLayoutElement.Format = DXGI_FORMAT_R32G32B32A32_FLOAT; }
			else if (attributeType == "VEC3") { inputLayoutElement.Format = DXGI_FORMAT_R32G32B32_FLOAT; }
			else if (attributeType == "VEC2") { inputLayoutElement.Format = DXGI_FORMAT_R32G32_FLOAT; }
			else if (attributeType == "SCALAR") { inputLayoutElement.Format = DXGI_FORMAT_R32_FLOAT; }

			meshPrimitive.InputLayoutList.push_back(inputLayoutElement);

			attributeIndex++;
		}
		meshPrimitive.InputLayout.NumElements = numAttributes;
		meshPrimitive.InputLayout.pInputElementDescs = meshPrimitive.InputLayoutList.data();
	}
}

void Model::SetMeshIndexBufferView(int meshIndex)
{
	Mesh& mesh = m_meshes[meshIndex];

	for (int i = 0; i < mesh.Primitives.size(); i++)
	{
		MeshPrimitive& meshPrimitive = mesh.Primitives[i];
		ModelGLTF::Accessor accessor = m_modelJson->accessors[m_modelJson->meshes[meshIndex].primitives[i].indices];
		ModelGLTF::BufferView bufferView = m_modelJson->bufferViews[accessor.bufferView];

		int byteStride{};
		switch (accessor.componentType)
		{
		case ModelGLTF::ComponentType::UInt:
		case ModelGLTF::ComponentType::Float:
			byteStride = 4;
			break;

		case ModelGLTF::ComponentType::Short:
		case ModelGLTF::ComponentType::UShort:
			byteStride = 2;
			break;

		case ModelGLTF::ComponentType::Byte:
		case ModelGLTF::ComponentType::UByte:
			byteStride = 1;
			break;
		}

		D3D12_INDEX_BUFFER_VIEW& meshPrimitiveIndexBufferView = meshPrimitive.IndexBufferView;
		meshPrimitiveIndexBufferView.BufferLocation = ModelBinResource->GetGPUVirtualAddress() + accessor.byteOffset + bufferView.byteOffset;
		meshPrimitiveIndexBufferView.SizeInBytes = accessor.count * byteStride;
		meshPrimitiveIndexBufferView.Format = DXGI_FORMAT_R32_UINT;

		meshPrimitive.NumIndices = accessor.count;
	}
}


void Model::SetMeshTexturesAndColors(int meshIndex, Mesh& mesh)
{
	for (int i = 0; i < mesh.Primitives.size(); i++)
	{
		MeshPrimitive& meshPrimitive = mesh.Primitives[i];
		int materialIndex = m_modelJson->meshes[meshIndex].primitives[i].material;
		ModelGLTF::Material texMaterial = m_modelJson->materials[materialIndex];

		// Colors
		std::vector<float>& baseColorFactor = texMaterial.pbrMetallicRoughness.baseColorFactor;
		meshPrimitive.ColorFactorsData.baseColorFactor = { baseColorFactor[0], baseColorFactor[1], baseColorFactor[2], baseColorFactor[3] };
		meshPrimitive.ColorFactorsData.metallicFactor = texMaterial.pbrMetallicRoughness.metallicFactor;
		meshPrimitive.ColorFactorsData.roughnessFactor = texMaterial.pbrMetallicRoughness.roughnessFactor;

		// Textures
		std::vector<TexIndexAndType> texIndicesAndNames{};

		int normalTexIndex = texMaterial.normalTexture.index;
		int occlusionTexIndex = texMaterial.occlusionTexture.index;
		int emissiveTexIndex = texMaterial.emissiveTexture.index;
		int metallicRoughnessTexIndex = texMaterial.pbrMetallicRoughness.metallicRoughnessTexture.index;
		int pbrBaseColorTexIndex = texMaterial.pbrMetallicRoughness.baseColorTexture.index;

		if (normalTexIndex >= 0) { texIndicesAndNames.push_back({ normalTexIndex, NORMAL }); }
		if (occlusionTexIndex >= 0) { texIndicesAndNames.push_back({ occlusionTexIndex, OCCLUSION }); }
		if (emissiveTexIndex >= 0) { texIndicesAndNames.push_back({ emissiveTexIndex, EMISSIVE }); }
		if (metallicRoughnessTexIndex >= 0) { texIndicesAndNames.push_back({ metallicRoughnessTexIndex, METALLIC_ROUGHNESS }); }
		if (pbrBaseColorTexIndex >= 0) { texIndicesAndNames.push_back({ pbrBaseColorTexIndex, PBR_BASE }); }

		for (TexIndexAndType& texIndexAndName : texIndicesAndNames)
		{
			ModelGLTF::Texture texMetaData = m_modelJson->textures[texIndexAndName.Index];

			int imgIndex = texMetaData.source;
			ModelGLTF::Image texImage = m_modelJson->images[imgIndex];

			std::string texPath = m_modelBasePath + texImage.uri;
			//std::cout << "\nTexture image path: " << texPath;

			Texture texture{};
			int tempWidth{};
			int tempHeight{};
			int tempNumChannels{};

			texture.PixelData = stbi_load(texPath.c_str(), &tempWidth, &tempHeight, &tempNumChannels, 4);
			texture.Width = static_cast<UINT>(tempWidth);
			texture.Height = static_cast<UINT>(tempHeight);
			texture.NumChannels = 4;
			texture.TexSizeBytes = texture.Width * texture.Height * texture.NumChannels;
			texture.TexBox = { 0, 0, 0, texture.Width, texture.Height, 1 };
			texture.Path = texPath;
			texture.Type = texIndexAndName.TextureType;

			//std::cout << "\nTexture dimensions: (" << texture->Width << "," << texture->Height << "), numChannels:" << texture->NumChannels << "\n";
			meshPrimitive.Textures.push_back(texture);
		}
	}
}

void Model::SetMeshShaders(Mesh& mesh)
{
	std::string shaderBinPath = "./shaderBin/";
	for (int i = 0; i < mesh.Primitives.size(); i++)
	{
		MeshPrimitive& meshPrimitive = mesh.Primitives[i];
		meshPrimitive.Shaders.clear();

		// TODO: Make generic for any type of shader
		Shader vertexShader{};
		vertexShader.BinPath = shaderBinPath + mesh.Name + "_VertexShader.cso";
		//std::cout << "\nVertex Shader path: " << vertexShader.BinPath << "\n";
		vertexShader.Type = ShaderType::VERTEX;
		Utils::LoadBinaryData(vertexShader.BinPath, vertexShader.BinData, vertexShader.BinSize);
		meshPrimitive.Shaders.push_back(vertexShader);

		Shader pixelShader{};
		pixelShader.BinPath = shaderBinPath + mesh.Name + "_PixelShader.cso";
		//std::cout << "\Pixel Shader path: " << pixelShader.BinPath << "\n";
		pixelShader.Type = ShaderType::PIXEL;
		Utils::LoadBinaryData(pixelShader.BinPath, pixelShader.BinData, pixelShader.BinSize);
		meshPrimitive.Shaders.push_back(pixelShader);
	}
}

bool Model::CreateRootSignature(MeshPrimitive& meshPrimitive, ComPtr<ID3D12Device>& device)
{
	std::vector<D3D12_ROOT_PARAMETER> rootParams{};

	// Layout ------------------------
	// Descriptor Table 1:
	//	Descriptor at b0: VPMatrix (CBV)
	//	Descriptor at b1: MeshPrimitiveModelSpaceTransformBuffer(CBV)
	//  Descriptor at b2: MeshPrimitiveColorFactors(CBV)
	// Descriptor Table 2:
	//	t0: WorldRootTransformBufferAllInstances(SRV)
	// Descriptor Table 3: 
	//	t1: Texture 1 (SRV)
	//	t2: Texture 2 (SRV)
	//  ...
	// Descriptor Table 4:
	//	b3: Light Direction (Constants)
	rootParams.reserve(4); // 3 tables + 1 lightDirection constant

	// ------------- Table 1: CBVs -----------------------------------------------
	// First table contains both CBVs
	D3D12_ROOT_DESCRIPTOR_TABLE rootCBVsDescTable{};

	D3D12_DESCRIPTOR_RANGE rootVPMatrixBufferDescRange{};
	rootVPMatrixBufferDescRange.BaseShaderRegister = 0; //b0
	rootVPMatrixBufferDescRange.NumDescriptors = 1;
	rootVPMatrixBufferDescRange.OffsetInDescriptorsFromTableStart = 0;
	rootVPMatrixBufferDescRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;

	D3D12_DESCRIPTOR_RANGE rootMeshPrimitiveModelSpaceTransformDescRange{};
	rootMeshPrimitiveModelSpaceTransformDescRange.BaseShaderRegister = 1; //b1
	rootMeshPrimitiveModelSpaceTransformDescRange.NumDescriptors = 1;
	rootMeshPrimitiveModelSpaceTransformDescRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
	rootMeshPrimitiveModelSpaceTransformDescRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;

	D3D12_DESCRIPTOR_RANGE rootMeshPrimitiveColorFactorsDescRange{};
	rootMeshPrimitiveColorFactorsDescRange.BaseShaderRegister = 2; //b2
	rootMeshPrimitiveColorFactorsDescRange.NumDescriptors = 1;
	rootMeshPrimitiveColorFactorsDescRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
	rootMeshPrimitiveColorFactorsDescRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;

	constexpr UINT numCBVDescriptorRanges = 3;
	rootCBVsDescTable.NumDescriptorRanges = numCBVDescriptorRanges;
	std::array<D3D12_DESCRIPTOR_RANGE, numCBVDescriptorRanges> cbvDescriptorRanges{ rootVPMatrixBufferDescRange, rootMeshPrimitiveModelSpaceTransformDescRange, rootMeshPrimitiveColorFactorsDescRange };
	rootCBVsDescTable.pDescriptorRanges = cbvDescriptorRanges.data();

	D3D12_ROOT_PARAMETER rootParamCBVs{};
	rootParamCBVs.DescriptorTable = rootCBVsDescTable;
	rootParamCBVs.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParamCBVs.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	rootParams.emplace_back(rootParamCBVs);

	// ------------- Table 2: WorldRootTransformBufferAllInstances SRV -----------------------------------
	D3D12_DESCRIPTOR_RANGE allInstancesSRVDescRange{};
	allInstancesSRVDescRange.BaseShaderRegister = 0; //t0
	allInstancesSRVDescRange.NumDescriptors = 1;
	allInstancesSRVDescRange.OffsetInDescriptorsFromTableStart = 0;
	allInstancesSRVDescRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;

	D3D12_ROOT_DESCRIPTOR_TABLE rootAllInstancesSRVTable{};
	rootAllInstancesSRVTable.NumDescriptorRanges = 1;
	rootAllInstancesSRVTable.pDescriptorRanges = &allInstancesSRVDescRange;

	D3D12_ROOT_PARAMETER rootParamAllInstancesSRV{};
	rootParamAllInstancesSRV.DescriptorTable = rootAllInstancesSRVTable;
	rootParamAllInstancesSRV.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParamAllInstancesSRV.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	rootParams.emplace_back(rootParamAllInstancesSRV);

	// ------------- Table 3: Textures SRV -----------------------------------------------------------

	std::vector<Texture>& meshTextures = meshPrimitive.Textures;
	UINT numTextures = static_cast<UINT>(meshTextures.size());
	std::vector<D3D12_DESCRIPTOR_RANGE> rootTextSrvDescriptorRanges{ numTextures };
	if (numTextures > 0)
	{
		for (int i = 0; i < numTextures; i++)
		{
			Texture& meshTexture = meshTextures[i];

			switch (meshTexture.Type)
			{
			case NORMAL:
				rootTextSrvDescriptorRanges[i].BaseShaderRegister = 1; //t1
				break;
			case OCCLUSION:
				rootTextSrvDescriptorRanges[i].BaseShaderRegister = 2;
				break;
			case EMISSIVE:
				rootTextSrvDescriptorRanges[i].BaseShaderRegister = 3;
				break;
			case METALLIC_ROUGHNESS:
				rootTextSrvDescriptorRanges[i].BaseShaderRegister = 4;
				break;
			case PBR_BASE:
				rootTextSrvDescriptorRanges[i].BaseShaderRegister = 5;
				break;
			default:
				rootTextSrvDescriptorRanges[i].BaseShaderRegister = 1;
			}

			rootTextSrvDescriptorRanges[i].NumDescriptors = 1;
			rootTextSrvDescriptorRanges[i].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
			rootTextSrvDescriptorRanges[i].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		}

		D3D12_ROOT_DESCRIPTOR_TABLE rootTexSrvDescriptorTable{};
		rootTexSrvDescriptorTable.NumDescriptorRanges = numTextures;
		rootTexSrvDescriptorTable.pDescriptorRanges = rootTextSrvDescriptorRanges.data();

		D3D12_ROOT_PARAMETER rootParamTextures{};
		rootParamTextures.DescriptorTable = rootTexSrvDescriptorTable;
		rootParamTextures.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		rootParamTextures.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

		rootParams.emplace_back(rootParamTextures);
	}


	// ---------------- Constants -----------------------------

	D3D12_ROOT_CONSTANTS rootConstantsLightDirection{};
	rootConstantsLightDirection.Num32BitValues = 3;
	rootConstantsLightDirection.ShaderRegister = 3; //b3
	rootConstantsLightDirection.RegisterSpace = 0;

	D3D12_ROOT_PARAMETER rootParamLightDirection{};
	rootParamLightDirection.ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
	rootParamLightDirection.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	rootParamLightDirection.Constants = rootConstantsLightDirection;

	rootParams.emplace_back(rootParamLightDirection);


	// ------------------ Static sampler --------------------------
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
	if (FAILED(hr) && errorBlob)
	{
		std::cout << static_cast<const char*>(errorBlob->GetBufferPointer()) << "\n";
	}
	PROMPTFAILHR(hr, "Failed to assign root signature blob! ");
	hr = device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(meshPrimitive.RootSignature.GetAddressOf()));
	PROMPTFAILHR(hr, "Failed to create root signature! ");

	return true;
}

bool Model::CreatePipelineStateObject(MeshPrimitive& meshPrimitive, ComPtr<ID3D12RootSignature> rootSignature, ComPtr<ID3D12Device>& device)
{
	HRESULT hr;
	D3D12_GRAPHICS_PIPELINE_STATE_DESC graphicsPipelineStateDesc{};
	graphicsPipelineStateDesc.InputLayout = meshPrimitive.InputLayout;
	graphicsPipelineStateDesc.pRootSignature = rootSignature.Get();
	graphicsPipelineStateDesc.VS = { meshPrimitive.Shaders[0].BinData.data(), meshPrimitive.Shaders[0].BinSize };
	graphicsPipelineStateDesc.PS = { meshPrimitive.Shaders[1].BinData.data(), meshPrimitive.Shaders[1].BinSize };
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

	graphicsPipelineStateDesc.pRootSignature = meshPrimitive.RootSignature.Get();
	hr = device->CreateGraphicsPipelineState(&graphicsPipelineStateDesc, IID_PPV_ARGS(meshPrimitive.PipelineStateObject.GetAddressOf()));
	PROMPTFAILHR(device->GetDeviceRemovedReason(), "Failed to create graphics pipeline state object. ");

	return true;
}


void Model::SetNodes()
{
	UINT numNodes = static_cast<UINT>(m_modelJson->nodes.size());
	m_nodesModelSpace.clear();
	m_nodesModelSpace.resize(numNodes);

	for (int nodeIndex = 0; nodeIndex < numNodes; nodeIndex++)
	{
		ModelGLTF::Node& nodeJson = m_modelJson->nodes[nodeIndex];
		Node& node = m_nodesModelSpace[nodeIndex];

		// These are in local node space
		node.NodeTransform.SetAndExtractFromTransformationMatrix(DirectX::XMMATRIX(nodeJson.matrix.data()));

		if (nodeJson.mesh != -1)
		{
			m_meshes[nodeJson.mesh].NodeIndex = nodeIndex;
			node.MeshIndex = nodeJson.mesh;
		}

		node.ChildrenNodeIndexes = nodeJson.children;
	}

	// Assign parent nodes to their children & upgrade nodes to model space from their local node space
	for (UINT nodeIndex = 0; nodeIndex < numNodes; nodeIndex++)
	{
		Node& node = m_nodesModelSpace[nodeIndex];
		for (int childNodeIndex : node.ChildrenNodeIndexes)
		{
			Node& childNode = m_nodesModelSpace[childNodeIndex];
			childNode.ParentNodeIndex = nodeIndex;

			DirectX::XMMATRIX modelSpaceMatrix =
				childNode.NodeTransform.GetTransformationMatrix() * node.NodeTransform.GetTransformationMatrix();

			childNode.NodeTransform.SetAndExtractFromTransformationMatrix(modelSpaceMatrix);
		}
	}
}