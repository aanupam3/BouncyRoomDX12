#pragma once
#include "Macros.h"
#include "ModelGLTF.h"
#include "Transform.h"
#include <array>
#include <nlohmann/json.hpp>
#include <wrl.h>

typedef unsigned char byte;
using Microsoft::WRL::ComPtr;

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
	ComPtr<ID3D12Resource> GPUResource{};
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
	std::vector<byte> BinData{};
	size_t BinSize{ 0 };
};

struct MeshPrimitive
{
	std::vector<std::string> AttributeNames{};
	std::vector<D3D12_VERTEX_BUFFER_VIEW> VertexBufferViews{};

	D3D12_INDEX_BUFFER_VIEW IndexBufferView{};
	UINT NumIndices{};

	std::vector<Texture> Textures{};
	std::vector<Shader> Shaders{};

	ComPtr<ID3D12PipelineState> PipelineStateObject{};
	ComPtr<ID3D12RootSignature> RootSignature{};

	D3D12_INPUT_LAYOUT_DESC InputLayout{};
	std::vector<D3D12_INPUT_ELEMENT_DESC> InputLayoutList{};

	ComPtr<ID3D12DescriptorHeap> PrimitiveShaderVisibleDescriptorHeap{};
	ComPtr<ID3D12Resource> MeshPrimitiveModelSpaceTransformBufferResource{}; // matrix containing the model space transform of this mesh primitive
};

struct Node
{
	Transform NodeTransform{};
	std::vector<int> ChildrenNodeIndexes{};
	int ParentNodeIndex;
	int MeshIndex = -1;
};

struct Mesh
{
	std::string Name;
	std::vector<MeshPrimitive> Primitives;
	int NodeIndex;
};


struct Instance
{
	Transform WorldTransform;
	bool IsEnabled;
	long Index;
};

class Model
{
private:
	std::string m_modelBasePath{};
	std::string m_glTFPath{};
	std::string m_binPath{};

	ModelGLTF::ModelJson* m_modelJson{};
	ModelBinData* m_binData;

	std::vector<Mesh> m_meshes{};
	std::vector<Node> m_nodesModelSpace{};
	//std::vector<Node> m_nodesWorldSpace{};

	void ExtractDataFromGLTF();

	void SetNodes();
	void SetMeshes();

	void SetMeshTextures(int meshIndex, Mesh& mesh);
	void SetMeshShaders(Mesh& mesh);
public:
	Model(std::string modelBasePath, std::string name = "");
	void SetMeshVertexBufferViews(int meshIndex);
	void SetMeshIndexBufferView(int meshIndex);
	~Model();

	std::string Name;
	std::vector<std::array<float, MATRIX4X4_NUMELEMENTS>> WorldRootTransformBuffersAllInstances{};
	ComPtr<ID3D12Resource> WorldRootTransformBuffersAllInstancesResource{};

	bool CreateRootSignature(MeshPrimitive& meshPrimitive, ComPtr<ID3D12Device>& device);
	bool CreatePipelineStateObject(MeshPrimitive& meshPrimitive, ComPtr<ID3D12RootSignature> rootSignature, ComPtr<ID3D12Device>& device);

	// getters
	const ModelGLTF::ModelJson* GetModelJson() const { return m_modelJson; }
	const ModelBinData* GetBinData() const { return m_binData; }
	std::vector<Mesh>& GetMeshes() { return m_meshes; }
	std::vector<Node>& GetNodesModelSpace() { return m_nodesModelSpace; }

	ComPtr<ID3D12Resource> ModelBinResource;
};