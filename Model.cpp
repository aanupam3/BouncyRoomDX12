#include "Macros.h"
#include "Model.h"
#include <fstream>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

Model::Model(std::string modelBasePath, std::string name) : m_modelBasePath(modelBasePath), Name(name)
{
	m_glTFPath = modelBasePath + "/scene.gltf";
	//m_texturesFolderPath = modelBasePath + "/textures/";

	ExtractDataFromGLTF();
}

void Model::SetData()
{
	SetMeshes();
	SetNodes();
}

Model::~Model()
{
	SAFE_RELEASE(m_modelBinaryDefaultHeap);

	delete m_binData;
	m_binData = nullptr;

	delete m_modelJson;
	m_modelJson = nullptr;

	for (int meshIndex = 0; meshIndex < m_meshes.size(); meshIndex++)
	{
		Mesh* mesh = m_meshes[meshIndex];
		for (int i = 0; i < mesh->Primitives.size(); i++)
		{
			MeshPrimitive* meshPrimitive = mesh->Primitives[i];
			for (int textureIndex = 0; textureIndex < meshPrimitive->Textures.size(); textureIndex++)
			{
				Texture* texture = meshPrimitive->Textures.at(textureIndex);
				SAFE_RELEASE(texture->GPUResource);

				stbi_image_free(texture->PixelData);
				texture->PixelData = nullptr;
				delete(texture);
			}

			meshPrimitive->VertexBufferViews.clear();
			//delete(mesh->VertexBufferViews);

			SAFE_RELEASE(meshPrimitive->MainShaderVisibleDescriptorHeap);


			meshPrimitive->Textures.clear();
		}

		SAFE_RELEASE(mesh->MeshNode->WVPMatrixGPUResource);
		delete(mesh);
	}
	m_meshes.clear();
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
	if (binFile.is_open()) { std::cout << "Found binary!\n"; }
	else
	{
		std::cout << "Did not find binary!\n";
		return;
	}

	const UINT binFileSize = m_modelJson->buffers[0].byteLength;
	m_binData = new ModelBinData(new byte[binFileSize], binFileSize);

	binFile.seekg(0, std::ios::beg);
	binFile.read((char*)m_binData->binData, binFileSize);
}

void Model::SetMeshes()
{
	NumMeshes = static_cast<UINT>(m_modelJson->meshes.size());
	for (UINT meshIndex = 0; meshIndex < NumMeshes; meshIndex++)
	{
		Mesh* mesh = new Mesh();
		mesh->Name = m_modelJson->meshes[meshIndex].name;
		m_meshes.push_back(mesh);

		UINT numMeshPrimitives = m_modelJson->meshes[meshIndex].primitives.size();
		for (UINT primitiveIndex = 0; primitiveIndex < numMeshPrimitives; primitiveIndex++)
		{
			MeshPrimitive* meshPrimitive = new MeshPrimitive();
			mesh->Primitives.push_back(meshPrimitive);
		}

		SetMeshVertexBufferViews(meshIndex);
		SetMeshIndexBufferView(meshIndex);
		SetMeshTextures(meshIndex);
		SetMeshShaders(mesh);
	}
}

void Model::SetMeshVertexBufferViews(int meshIndex)
{
	Mesh* mesh = m_meshes[meshIndex];

	for (UINT primitiveIndex = 0; primitiveIndex < mesh->Primitives.size(); primitiveIndex++)
	{
		MeshPrimitive* meshPrimitive = mesh->Primitives[primitiveIndex];
		std::vector<D3D12_VERTEX_BUFFER_VIEW>& meshPrimitiveVBVs = meshPrimitive->VertexBufferViews;

		nlohmann::json attributes = m_modelJson->meshes[meshIndex].primitives[primitiveIndex].attributes;
		UINT numAttributes = static_cast<UINT>(attributes.size());

		// attributeName is the key, accessorIndex is the value
		int attributeIndex = 0;
		meshPrimitive->AttributeNames.resize(numAttributes);

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

			meshPrimitive->AttributeNames[attributeIndex] = attributeName;
			UINT semanticIndex = 0;

			// Dont want the semantic name to be TEXCOORD_1 like in the glTF file, so we re-assign it explicitly here
			if (attributeName.find("TEXCOORD") != std::string::npos)
			{
				meshPrimitive->AttributeNames[attributeIndex] = "TEXCOORD";
				semanticIndex = std::stoi(attributeName.substr(9));
			}

			D3D12_INPUT_ELEMENT_DESC inputLayoutElement{};
			inputLayoutElement.AlignedByteOffset = 0;
			inputLayoutElement.InputSlot = attributeIndex;
			inputLayoutElement.InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
			inputLayoutElement.SemanticName = meshPrimitive->AttributeNames[attributeIndex].c_str();
			inputLayoutElement.SemanticIndex = semanticIndex;
			std::cout << "Attribute semantic name: " << meshPrimitive->AttributeNames[attributeIndex]
				<< ", semantic index: " << semanticIndex << "\n";

			std::string& attributeType = attributeAccessor.type;
			if (attributeType == "VEC4") { inputLayoutElement.Format = DXGI_FORMAT_R32G32B32A32_FLOAT; }
			else if (attributeType == "VEC3") { inputLayoutElement.Format = DXGI_FORMAT_R32G32B32_FLOAT; }
			else if (attributeType == "VEC2") { inputLayoutElement.Format = DXGI_FORMAT_R32G32_FLOAT; }
			else if (attributeType == "SCALAR") { inputLayoutElement.Format = DXGI_FORMAT_R32_FLOAT; }

			meshPrimitive->inputLayoutList.push_back(inputLayoutElement);

			attributeIndex++;
		}
		meshPrimitive->inputLayout.NumElements = numAttributes;
		meshPrimitive->inputLayout.pInputElementDescs = meshPrimitive->inputLayoutList.data();
	}
}

void Model::SetMeshIndexBufferView(int meshIndex)
{
	Mesh* mesh = m_meshes[meshIndex];

	for (int i = 0; i < mesh->Primitives.size(); i++)
	{
		MeshPrimitive* meshPrimitive = mesh->Primitives[i];
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

		D3D12_INDEX_BUFFER_VIEW& meshPrimitiveIndexBufferView = meshPrimitive->IndexBufferView;
		meshPrimitiveIndexBufferView.BufferLocation = ModelBinResource->GetGPUVirtualAddress() + accessor.byteOffset + bufferView.byteOffset;
		meshPrimitiveIndexBufferView.SizeInBytes = accessor.count * byteStride;
		meshPrimitiveIndexBufferView.Format = DXGI_FORMAT_R32_UINT;

		meshPrimitive->NumIndices = accessor.count;
	}
}

void Model::SetMeshTextures(int meshIndex)
{
	Mesh* mesh = m_meshes[meshIndex];

	for (int i = 0; i < mesh->Primitives.size(); i++)
	{
		MeshPrimitive* meshPrimitive = mesh->Primitives[i];
		int materialIndex = m_modelJson->meshes[meshIndex].primitives[i].material;
		ModelGLTF::Material texMaterial = m_modelJson->materials[materialIndex];

		std::vector<int> texIndices{};

		int normalTexIndex = texMaterial.normalTexture.index;
		int occlusionTexIndex = texMaterial.occlusionTexture.index;
		int emissiveTexIndex = texMaterial.emissiveTexture.index;
		int metallicRoughnessTexIndex = texMaterial.pbrMetallicRoughness.metallicRoughnessTexture.index;
		int pbrBaseColorTexIndex = texMaterial.pbrMetallicRoughness.baseColorTexture.index;

		if (normalTexIndex >= 0) { texIndices.push_back(normalTexIndex); }
		if (occlusionTexIndex >= 0) { texIndices.push_back(occlusionTexIndex); }
		if (emissiveTexIndex >= 0) { texIndices.push_back(emissiveTexIndex); }
		if (metallicRoughnessTexIndex >= 0) { texIndices.push_back(metallicRoughnessTexIndex); }
		if (pbrBaseColorTexIndex >= 0) { texIndices.push_back(pbrBaseColorTexIndex); }

		for (int& texIndex : texIndices)
		{
			ModelGLTF::Texture texMetaData = m_modelJson->textures[texIndex];

			int imgIndex = texMetaData.source;
			ModelGLTF::Image texImage = m_modelJson->images[imgIndex];

			std::string texPath = m_modelBasePath + texImage.uri;
			std::cout << "\nTexture image path: " << texPath;

			Texture* texture = new Texture();
			int tempWidth{};
			int tempHeight{};
			int tempNumChannels{};

			texture->PixelData = stbi_load(texPath.c_str(), &tempWidth, &tempHeight, &tempNumChannels, 4);
			texture->Width = static_cast<UINT>(tempWidth);
			texture->Height = static_cast<UINT>(tempHeight);
			texture->NumChannels = 4;
			texture->TexSizeBytes = texture->Width * texture->Height * texture->NumChannels;
			texture->TexBox = { 0, 0, 0, texture->Width, texture->Height, 1 };
			texture->Path = texPath;

			std::cout << "\nTexture dimensions: (" << texture->Width << "," << texture->Height << "), numChannels:" << texture->NumChannels << "\n";
			meshPrimitive->Textures.push_back(texture);
		}
	}
	/*ModelGLTF::Texture normalTexMetaData = m_modelJson->textures[normalTexIndex];
	ModelGLTF::Texture occlusionTexMetaData = m_modelJson->textures[occlusionTexIndex];
	ModelGLTF::Texture emissiveTexMetaData = m_modelJson->textures[emissiveTexIndex];
	ModelGLTF::Texture metallicRoughnessTexMetaData = m_modelJson->textures[metallicRoughnessTexIndex];
	ModelGLTF::Texture pbrBaseColorTexMetaData = m_modelJson->textures[pbrBaseColorTexIndex];*/
}

void Model::SetMeshShaders(Mesh* mesh)
{
	for (int i = 0; i < mesh->Primitives.size(); i++)
	{
		MeshPrimitive* meshPrimitive = mesh->Primitives[i];
		meshPrimitive->Shaders.clear();

		// TODO: Make generic for any type of shader
		Shader vertexShader{};
		vertexShader.BinPath = mesh->Name + "_VertexShader.cso";
		std::cout << "\nVertex Shader path: " << vertexShader.BinPath << "\n";
		vertexShader.Type = ShaderType::VERTEX;
		Utils::LoadBinaryData(vertexShader.BinPath, vertexShader.BinData, vertexShader.BinSize);
		meshPrimitive->Shaders.push_back(vertexShader);

		Shader pixelShader{};
		pixelShader.BinPath = mesh->Name + "_PixelShader.cso";
		std::cout << "\Pixel Shader path: " << pixelShader.BinPath << "\n";
		pixelShader.Type = ShaderType::PIXEL;
		Utils::LoadBinaryData(pixelShader.BinPath, pixelShader.BinData, pixelShader.BinSize);
		meshPrimitive->Shaders.push_back(pixelShader);
	}
}

void Model::SetWVPMatrixForMesh(Mesh* mesh, DirectX::XMMATRIX& newWVPMatrix)
{
	Node& meshNode = *(mesh->MeshNode);
	meshNode.WVPMatrix = newWVPMatrix;
	meshNode.WVPMatrixVector = Utils::xmMatrixToVector(newWVPMatrix);
}

void Model::SetNodes()
{
	UINT numNodes = static_cast<UINT>(m_modelJson->nodes.size());
	for (ModelGLTF::Node& nodeJson : m_modelJson->nodes)
	{
		Node* node = new Node();

		node->LocalSpaceTransformMatrix = DirectX::XMMATRIX(nodeJson.matrix.data());

		if (nodeJson.mesh != -1) { m_meshes[nodeJson.mesh]->MeshNode = node; }

		m_nodes.push_back(node);
	}

	// Set relationships to other nodes once all nodes have been added
	for (UINT nodeIndex = 0; nodeIndex < numNodes; nodeIndex++)
	{
		Node* node = m_nodes[nodeIndex];
		ModelGLTF::Node& nodeJson = m_modelJson->nodes[nodeIndex];
		for (int childNodeIndex : nodeJson.children)
		{
			Node* childNode = m_nodes[childNodeIndex];
			childNode->ParentNode = node;
			childNode->ModelSpaceTransformMatrix
				= DirectX::XMMatrixMultiply(childNode->LocalSpaceTransformMatrix, node->ModelSpaceTransformMatrix);

			std::cout << "\Model Space Transform matrix for child node at index :" << childNodeIndex;
			Utils::printMatrix(childNode->ModelSpaceTransformMatrix);

			childNode->WorldSpaceTransformMatrix
				= DirectX::XMMatrixMultiply(childNode->ModelSpaceTransformMatrix, m_nodes[0]->WorldSpaceTransformMatrix);

			std::cout << "\World Space Transform matrix for child node at index :" << childNodeIndex;
			Utils::printMatrix(childNode->WorldSpaceTransformMatrix);

			// likely not needed since we don't need non-meshed nodes beyond the above matrix calculation
			node->ChildrenNodes.push_back(childNode);
		}
	}
}

void Model::UpdateNodeWorldSpace(Node* node)
{
	for (Node* childNode : node->ChildrenNodes)
	{
		/*childNode->ModelSpaceTransformMatrix
			= DirectX::XMMatrixMultiply(childNode->LocalSpaceTransformMatrix, childNode->ParentNode->ModelSpaceTransformMatrix);*/

		childNode->WorldSpaceTransformMatrix
			= DirectX::XMMatrixMultiply(childNode->ModelSpaceTransformMatrix, m_nodes[0]->WorldSpaceTransformMatrix);

		UpdateNodeWorldSpace(childNode);
	}
}

void Model::UpdateAllNodesWorldSpace()
{
	// Assumes root node is the first element and that there is only 1 root node
	UpdateNodeWorldSpace(m_nodes[0]);
}

void Model::TranslateBy(float x, float y, float z)
{
	m_nodes[0]->ModelSpaceTransformMatrix *= DirectX::XMMatrixTranslation(x, y, z);
	m_nodes[0]->WorldSpaceTransformMatrix *= DirectX::XMMatrixTranslation(x, y, z);
	UpdateAllNodesWorldSpace();
}

void Model::RotateByDegrees(float x, float y, float z)
{
	float pitch = DirectX::XMConvertToRadians(x);
	float yaw = DirectX::XMConvertToRadians(y);
	float roll = DirectX::XMConvertToRadians(z);
	m_nodes[0]->ModelSpaceTransformMatrix *= DirectX::XMMatrixRotationRollPitchYaw(pitch, yaw, roll);
	m_nodes[0]->WorldSpaceTransformMatrix *= DirectX::XMMatrixRotationRollPitchYaw(pitch, yaw, roll);
	UpdateAllNodesWorldSpace();
}

//void Model::UpdateTransforms(std::vector<Node>& nodesJson, int nodeIndex = 0)
//{
//	m_renderableNodeWorldMatrices.clear(); // reset list of world matrices for renderable nodes
//
//	std::cout << "Updated transforms\n";
//	for (int i = 0; i < nodesJson.size(); i++)
//	{
//		Node& currentNode = nodesJson[i];
//		//std::cout << "\nNode: " << i << ", " << currentNode.name;
//
//		if (i != 0) //root node has no parent
//		{
//			Node& parent = *currentNode.ancestor;
//			currentNode.localTransformMatrix = DirectX::XMMatrixMultiply(currentNode.localTransformMatrix, parent.localTransformMatrix);
//			//std::cout << "\nParent: " << parent.name;
//		}
//
//		//Utils::printMatrix(currentNode.xmMatrix);
//
//		if (currentNode.mesh >= 0)
//		{
//			m_renderableNodeWorldMatrices.push_back(&currentNode.localTransformMatrix);
//		}
//	}
//}
//
//void Model::BuildNodeTree(std::vector<Node>& nodesJson)
//{
//	for(Node& currentNode : nodesJson)
//	{
//		currentNode.localTransformMatrix = DirectX::XMMATRIX(currentNode.matrix.data());
//
//		for (int j = 0; j < currentNode.children.size(); j++)
//		{
//			// Assign the actual node pointers to the children nodes using the children indexes
//			int childIndex = currentNode.children[j];
//			Node& childNode = nodesJson[childIndex];
//
//			childNode.ancestor = &currentNode; // assign the current node as an ancestor for the line
//		}
//	}
//}

// const int Model::GetNumberOfIndicesInMesh(int meshIndex = 0) const
//{
//	nlohmann::json indicesAccessorNumber = m_modelJson->meshes[meshIndex].primitives[0].indices;
//	Accessor accessor = m_modelJson->accessors[indicesAccessorNumber];
//
//	return accessor.count;
//}
//bool Model::UploadModelBinary(ID3D12Device* device, ID3D12GraphicsCommandList* commandList)
//{
//	const size_t modelBinSize = m_binData->binDataSize;
//	const size_t numMeshesInModel = 1;// modelJson->meshes.size();
//
//	D3D12_HEAP_PROPERTIES modelBinaryDefaultHeapProperties{ D3D12_HEAP_TYPE_DEFAULT };
//	D3D12_RESOURCE_DESC modelBinaryResourceDesc{};
//	modelBinaryResourceDesc.Alignment = 0;
//	modelBinaryResourceDesc.DepthOrArraySize = 1;
//	modelBinaryResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
//	modelBinaryResourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
//	modelBinaryResourceDesc.Format = DXGI_FORMAT_UNKNOWN;
//	modelBinaryResourceDesc.Height = 1;
//	modelBinaryResourceDesc.Width = modelBinSize;
//	modelBinaryResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
//	modelBinaryResourceDesc.MipLevels = 1;
//	modelBinaryResourceDesc.SampleDesc = { 1, 0 };
//
//	HRESULT hr = device->CreateCommittedResource(
//		&modelBinaryDefaultHeapProperties,
//		D3D12_HEAP_FLAG_NONE,
//		&modelBinaryResourceDesc,
//		D3D12_RESOURCE_STATE_COMMON,
//		nullptr,
//		IID_PPV_ARGS(&m_modelBinaryDefaultHeap)
//	);
//	PROMPTFAIL(hr, "Failed to create default heap for binary file");
//
//	ID3D12Resource* modelBinaryUploadHeap{};
//
//	D3D12_HEAP_PROPERTIES modelBinaryUploadHeapProperties{ D3D12_HEAP_TYPE_UPLOAD };
//	hr = device->CreateCommittedResource(
//		&modelBinaryUploadHeapProperties,
//		D3D12_HEAP_FLAG_NONE,
//		&modelBinaryResourceDesc,
//		D3D12_RESOURCE_STATE_COMMON,
//		nullptr,
//		IID_PPV_ARGS(&modelBinaryUploadHeap)
//	);
//	PROMPTFAIL(hr, "Failed to create upload heap for binary file");
//
//	byte* pModelBinaryUploadHeap;
//	modelBinaryUploadHeap->Map(0, nullptr, reinterpret_cast<void**>(&pModelBinaryUploadHeap));
//	memcpy(pModelBinaryUploadHeap, m_binData, modelBinSize);
//	modelBinaryUploadHeap->Unmap(0, nullptr);
//
//	commandList->CopyBufferRegion(m_modelBinaryDefaultHeap, 0, modelBinaryUploadHeap, 0, modelBinSize);
//}
