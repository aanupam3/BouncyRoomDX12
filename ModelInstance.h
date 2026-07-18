#pragma once
#include "Model.h"
#include "Utils.h"
#include <d3d12.h>
#include <vector>

struct NodeWorld
{
	Transform NodeTransform{};
	std::vector<int> ChildrenNodeIndexes{};
	int ParentNodeIndex;
	int MeshIndex = -1;

	std::vector<ID3D12DescriptorHeap*> PrimitiveShaderVisibleDescriptorHeaps{};

	std::vector<float> WVPMatrixVector;
	ID3D12Resource* WVPMatrixGPUResource{};
};

class ModelInstance
{
private:
	std::vector<NodeWorld*> m_nodesWorldSpace{};
	Transform m_worldTransform;

public:
	const Model* BaseModel;

	ModelInstance(const Model* model, const Transform& worldTransform);
	~ModelInstance();

	void UpdateTransform(const Transform& newTransform);

	const std::vector<NodeWorld*>& GetNodes() const { return m_nodesWorldSpace; }
	const Transform& GetTransform() const { return m_worldTransform; }
};