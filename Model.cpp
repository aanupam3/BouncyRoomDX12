#include "Macros.h"
#include "Model.h"
#include <fstream>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

Model::Model(std::string modelBasePath, std::string name) : m_modelBasePath(modelBasePath), Name(name)
{
	m_glTFPath = modelBasePath + "/scene.gltf";
	//m_texturesFolderPath = modelBasePath + "/textures/";

	std::cout << "Creating model: " << Name << "\n";

	ExtractDataFromGLTF();
	SetMeshes();
	SetNodes();
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
	m_nodesLocalSpace.clear();
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
	for (UINT meshIndex = 0; meshIndex < NumMeshes; meshIndex++)
	{
		Mesh mesh{};
		mesh.Name = m_modelJson->meshes[meshIndex].name;

		UINT numMeshPrimitives = m_modelJson->meshes[meshIndex].primitives.size();
		mesh.Primitives.resize(numMeshPrimitives);

		SetMeshTextures(meshIndex, mesh);
		SetMeshShaders(mesh);

		m_meshes.emplace_back(mesh);
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

void Model::SetMeshTextures(int meshIndex, Mesh& mesh)
{
	for (int i = 0; i < mesh.Primitives.size(); i++)
	{
		MeshPrimitive& meshPrimitive = mesh.Primitives[i];
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

			//std::cout << "\nTexture dimensions: (" << texture->Width << "," << texture->Height << "), numChannels:" << texture->NumChannels << "\n";
			meshPrimitive.Textures.push_back(texture);
		}
	}
}

void Model::SetMeshShaders(Mesh& mesh)
{
	for (int i = 0; i < mesh.Primitives.size(); i++)
	{
		MeshPrimitive& meshPrimitive = mesh.Primitives[i];
		meshPrimitive.Shaders.clear();

		// TODO: Make generic for any type of shader
		Shader vertexShader{};
		vertexShader.BinPath = mesh.Name + "_VertexShader.cso";
		//std::cout << "\nVertex Shader path: " << vertexShader.BinPath << "\n";
		vertexShader.Type = ShaderType::VERTEX;
		Utils::LoadBinaryData(vertexShader.BinPath, vertexShader.BinData, vertexShader.BinSize);
		meshPrimitive.Shaders.push_back(vertexShader);

		Shader pixelShader{};
		pixelShader.BinPath = mesh.Name + "_PixelShader.cso";
		//std::cout << "\Pixel Shader path: " << pixelShader.BinPath << "\n";
		pixelShader.Type = ShaderType::PIXEL;
		Utils::LoadBinaryData(pixelShader.BinPath, pixelShader.BinData, pixelShader.BinSize);
		meshPrimitive.Shaders.push_back(pixelShader);
	}
}

void Model::SetNodes()
{
	UINT numNodes = static_cast<UINT>(m_modelJson->nodes.size());
	m_nodesLocalSpace.reserve(numNodes);

	for (int nodeIndex = 0; nodeIndex < numNodes; nodeIndex++)
	{
		ModelGLTF::Node& nodeJson = m_modelJson->nodes[nodeIndex];
		LocalNode node{};

		node.NodeTransform.SetAndExtractFromTransformationMatrix(DirectX::XMMATRIX(nodeJson.matrix.data()));

		if (nodeJson.mesh != -1)
		{
			m_meshes[nodeJson.mesh].NodeIndex = nodeIndex;
			node.MeshIndex = nodeJson.mesh;
		}

		node.ChildrenNodeIndexes = nodeJson.children;

		m_nodesLocalSpace.push_back(node);
	}

	for (UINT nodeIndex = 0; nodeIndex < numNodes; nodeIndex++)
	{
		LocalNode& node = m_nodesLocalSpace[nodeIndex];
		for (int childNodeIndex : node.ChildrenNodeIndexes)
		{
			LocalNode& childNode = m_nodesLocalSpace[childNodeIndex];
			childNode.ParentNodeIndex = nodeIndex;
		}
	}
}