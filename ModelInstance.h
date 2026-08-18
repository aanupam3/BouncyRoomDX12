#pragma once
#include "Model.h"
#include "Utils.h"
#include <d3d12.h>
#include <vector>
#include <wrl.h>

using Microsoft::WRL::ComPtr;

struct WorldNode
{
	Transform NodeTransform{};

	std::vector<int> ChildrenNodeIndexes{};
	int ParentNodeIndex;
	int MeshIndex = -1;

	std::vector<ComPtr<ID3D12DescriptorHeap>> PrimitiveShaderVisibleDescriptorHeaps{};

	std::vector<float> WVPMatrixVector;
	ComPtr<ID3D12Resource> WVPMatrixGPUResource{};
};

class ModelInstance
{
private:
	std::vector<WorldNode> m_nodesWorldSpace{};
	Transform m_worldTransform;

public:
	Model* BaseModel;
	long Id;

	ModelInstance(Model& model, const Transform& worldTransform);
	~ModelInstance();

	void UpdateTransform(const Transform& newTransform);
	void SetShaderVisibleDescriptors(WorldNode& nodeWithMesh);

	std::vector<WorldNode>& GetNodes() { return m_nodesWorldSpace; }
	const Transform& GetTransform() const { return m_worldTransform; }
};