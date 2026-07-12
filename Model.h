#pragma once
#include "DirectXMath.h"
#include "ModelGLTF.h"
#include <d3d12.h>
#include <iostream>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

typedef unsigned char byte;

struct ModelBinData {
	byte* binData;
	UINT binDataSize;

	ModelBinData(byte* newBinData, int newBinDataSize) :
		binData(newBinData),
		binDataSize(newBinDataSize)
	{
	}
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
	size_t BinSize{ 0 };
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

struct Node
{
	DirectX::XMMATRIX LocalSpaceTransformMatrix{ DirectX::XMMatrixIdentity() };
	DirectX::XMMATRIX ModelSpaceTransformMatrix{ DirectX::XMMatrixIdentity() };
	DirectX::XMMATRIX WorldSpaceTransformMatrix{ DirectX::XMMatrixIdentity() };

	DirectX::XMMATRIX WVPMatrix{ DirectX::XMMatrixIdentity() };
	std::vector<float> WVPMatrixVector;
	ID3D12Resource* WVPMatrixGPUResource;

	std::vector<Node*> ChildrenNodes{};
	Node* ParentNode;
};

struct Mesh
{
	std::string Name;
	std::vector<MeshPrimitive*> Primitives{};
	Node* MeshNode{};
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

	DirectX::XMFLOAT3 m_worldPosition{ 0,0,0 };
	DirectX::XMFLOAT3 m_worldRotationRadians{ 0,0,0 };
	DirectX::XMFLOAT3 m_worldScale{ 1,1,1 };

	DirectX::XMMATRIX worldTranslationMatrix{};
	DirectX::XMMATRIX worldRotationMatrix{};
	DirectX::XMMATRIX worldScalingMatrix{};

	void ExtractDataFromGLTF();
	void SetNodes();
	void UpdateAllNodesWorldSpace();
	void SetMeshes();

	void SetMeshTextures(int meshIndex);
	void SetMeshShaders(Mesh* mesh);

public:
	Model(std::string modelBasePath, std::string name = "");
	void SetMeshVertexBufferViews(int meshIndex);
	void SetMeshIndexBufferView(int meshIndex);
	~Model();

	std::string Name;
	UINT NumMeshes;

	const void SetWorldPosition(float x, float y, float z);
	const void SetWorldRotationDegrees(float x, float y, float z);
	const void SetWorldScale(float scale);
	const void SetWorldScale(float x, float y, float z);

	void TranslateBy(float x, float y, float z);
	void RotateByDegrees(float x, float y, float z);
	void ScaleBy(float x, float y, float z);

	// getters
	const ModelGLTF::ModelJson* GetModelJson() const { return m_modelJson; }
	const ModelBinData* GetBinData() const { return m_binData; }
	const std::vector<Mesh*>& GetMeshes() const { return m_meshes; }
	const std::vector<Node*>& GetNodes() const { return m_nodes; }
	const DirectX::XMFLOAT3& GetWorldPosition() const { return m_worldPosition; }
	const DirectX::XMFLOAT3& GetWorldRotationDegrees() const { return m_worldRotationRadians; }
	const DirectX::XMFLOAT3& GetWorldScale() const { return m_worldScale; }

	void SetWVPMatrixForMesh(Mesh* mesh, DirectX::XMMATRIX& newWVPMatrix);

	ID3D12Resource* ModelBinResource; //nullptr needs to be set
};
