#pragma once
#include "ModelGLTF.h"
#include "Transform.h"
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

	D3D12_INDEX_BUFFER_VIEW IndexBufferView{};
	UINT NumIndices;

	std::vector<Texture> Textures;
	std::vector<Shader> Shaders;

	// Note that WVP resources are located in model instances as they will be different for each instance

	D3D12_INPUT_LAYOUT_DESC InputLayout{};
	std::vector<D3D12_INPUT_ELEMENT_DESC> InputLayoutList{};
};

struct LocalNode
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

class Model
{
private:
	std::string m_modelBasePath{};
	std::string m_glTFPath{};
	std::string m_binPath{};

	ModelGLTF::ModelJson* m_modelJson{};
	ModelBinData* m_binData;

	ComPtr<ID3D12Resource> m_modelBinaryDefaultHeap{};

	std::vector<Mesh> m_meshes{};
	std::vector<LocalNode> m_nodesLocalSpace{};

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

	// getters
	const ModelGLTF::ModelJson* GetModelJson() const { return m_modelJson; }
	const ModelBinData* GetBinData() const { return m_binData; }
	std::vector<Mesh>& GetMeshes() { return m_meshes; }
	std::vector<LocalNode>& GetNodes() { return m_nodesLocalSpace; }

	ComPtr<ID3D12Resource> ModelBinResource;
};