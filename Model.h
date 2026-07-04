#pragma once
#include <string>
#include <vector>
#include <iostream>
#include <d3d12.h>
#include <nlohmann/json.hpp>
#include "DirectXMath.h"
#include "ModelGLTF.h"

typedef unsigned char byte;

struct ModelBinData {
	byte* binData;
	UINT binDataSize;

	ModelBinData(byte* newBinData, int newBinDataSize) : 
		binData(newBinData), 
		binDataSize(newBinDataSize)
	{ }
};

struct Texture
{
	UINT TexDataOffset{};
	UINT TexSizeBytes{};
	std::string Name;
	UINT Width{};
	UINT Height{};
	UINT NumChannels{};
	DXGI_FORMAT Format{ DXGI_FORMAT_R8G8B8A8_UNORM };
	std::string Path{};
	D3D12_BOX TexBox{};

	byte* PixelData{};
	ID3D12Resource* GPUResource{};
};

enum ShaderType
{
	VERTEX = 0,
	PIXEL
};

struct Shader
{
	ShaderType Type{};
	std::string BinPath{};
	byte* BinData{};
	size_t BinSize{0};
};

struct MeshPrimitive
{
	std::vector<D3D12_VERTEX_BUFFER_VIEW> VertexBufferViews{};
	std::vector<std::string> AttributeNames{};

	ID3D12DescriptorHeap* MainShaderVisibleDescriptorHeap;

	D3D12_INDEX_BUFFER_VIEW IndexBufferView{};
	UINT NumIndices;

	std::vector<Texture*> Textures;
	std::vector<Shader> Shaders;


	D3D12_INPUT_LAYOUT_DESC inputLayout{};
	std::vector<D3D12_INPUT_ELEMENT_DESC> inputLayoutList{};
};

struct Mesh
{
	std::string Name;
	std::vector<MeshPrimitive*> Primitives{};

	// Transform related
	DirectX::XMMATRIX LocalSpaceTransformMatrix{ DirectX::XMMatrixIdentity() };
	DirectX::XMMATRIX ModelSpaceTransformMatrix{ DirectX::XMMatrixIdentity() };
	DirectX::XMMATRIX WVPMatrix{ DirectX::XMMatrixIdentity() };
	std::vector<float> WVPMatrixVector;
	ID3D12Resource* WVPMatrixGPUResource;
};

struct Node
{
	DirectX::XMMATRIX LocalSpaceTransformMatrix{ DirectX::XMMatrixIdentity() };
	DirectX::XMMATRIX ModelSpaceTransformMatrix{ DirectX::XMMatrixIdentity() };
	Mesh* mesh{};
	std::vector<Node*> ChildrenNodes{};
	Node* ParentNode;
};


class Model
{
private:
	std::string m_modelBasePath{};
	std::string m_glTFPath{};
	std::string m_binPath{};
	ModelGLTF::ModelJson* m_modelJson{};

	const ModelBinData* m_binData{};
	ID3D12Resource* m_modelBinaryDefaultHeap{};
	
	std::vector<Mesh*> m_meshes{};
	std::vector<Node*> m_nodes{};

	void ExtractDataFromGLTF();
	void SetNodes();
	void SetMeshes();
	void SetMeshVertexBufferViews(int meshIndex);
	void SetMeshIndexBufferView(int meshIndex);
	void SetMeshTextures(int meshIndex);
	void SetMeshShaders(Mesh* mesh);

public:
	Model(std::string modelBasePath, std::string name = "");
	void SetData();
	~Model();

	std::string Name;
	UINT NumMeshes;

	// getters
	const ModelGLTF::ModelJson* GetModelJson() const { return m_modelJson; }
	const ModelBinData* GetBinData() const { return m_binData; }
	const std::vector<Mesh*>& GetMeshes() const { return m_meshes; }
	const std::vector<Node*>& GetNodes() const { return m_nodes; }

	void SetWVPMatrixForMesh(Mesh* mesh, DirectX::XMMATRIX& newWVPMatrix);

	ID3D12Resource* ModelBinResource; //nullptr needs to be set
};
