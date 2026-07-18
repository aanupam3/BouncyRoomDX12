#include "Macros.h"
#include "ModelInstance.h"

ModelInstance::ModelInstance(const Model* model, const Transform& worldTransform) : BaseModel(model), m_worldTransform(worldTransform)
{
	// Create the same node tree but with independent transforms
	const std::vector<NodeLocal*>& baseModelNodes = model->GetNodes(); // base model nodes are in local space, and will now be updated in the instance to world space

	int numNodes = baseModelNodes.size();
	m_nodesWorldSpace.resize(numNodes);

	for (int nodeIndex = 0; nodeIndex < numNodes; nodeIndex++)
	{
		NodeLocal* baseNode = baseModelNodes[nodeIndex];
		m_nodesWorldSpace[nodeIndex] = new NodeWorld();

		NodeWorld* instanceNode = m_nodesWorldSpace[nodeIndex];
		instanceNode->ChildrenNodeIndexes = baseNode->ChildrenNodeIndexes;
		instanceNode->ParentNodeIndex = baseNode->ParentNodeIndex;
		instanceNode->MeshIndex = baseNode->MeshIndex;

		DirectX::XMMATRIX instanceNodeLocalTransformationMatrix = baseNode->NodeTransform.GetTransformationMatrix();
		instanceNode->NodeTransform.SetAndExtractFromTransformationMatrix(instanceNodeLocalTransformationMatrix);
	}

	// Upgrade nodes to model space
	for (UINT nodeIndex = 0; nodeIndex < numNodes; nodeIndex++)
	{
		NodeWorld* node = m_nodesWorldSpace[nodeIndex];
		for (int childNodeIndex : node->ChildrenNodeIndexes)
		{
			NodeWorld* childNode = m_nodesWorldSpace[childNodeIndex];
			childNode->ParentNodeIndex = nodeIndex;

			DirectX::XMMATRIX modelSpaceMatrix =
				childNode->NodeTransform.GetTransformationMatrix() * node->NodeTransform.GetTransformationMatrix();

			childNode->NodeTransform.SetAndExtractFromTransformationMatrix(modelSpaceMatrix);
		}
	}

	// Finally Update to world level
	UpdateTransform(m_worldTransform);
}

ModelInstance::~ModelInstance()
{
	for (int i = 0; i < m_nodesWorldSpace.size(); i++)
	{
		NodeWorld* node = m_nodesWorldSpace[i];
		SAFE_RELEASE(node->WVPMatrixGPUResource);
		for (int primitiveIndex = 0; primitiveIndex < node->PrimitiveShaderVisibleDescriptorHeaps.size(); primitiveIndex++)
		{
			SAFE_RELEASE(node->PrimitiveShaderVisibleDescriptorHeaps[primitiveIndex]);
		}
		delete node;
	}
	m_nodesWorldSpace.clear();
}

void ModelInstance::UpdateTransform(const Transform& newTransform)
{
	const DirectX::XMMATRIX& worldTransformationMatrix = newTransform.GetTransformationMatrix();
	for (int i = 0; i < m_nodesWorldSpace.size(); i++)
	{
		NodeWorld* node = m_nodesWorldSpace[i];
		node->NodeTransform.SetAndExtractFromTransformationMatrix(
			DirectX::XMMatrixMultiply(node->NodeTransform.GetTransformationMatrix(), worldTransformationMatrix)
		);

		//Utils::printMatrix(node->NodeTransform.GetTransformationMatrix(), "Updated world space transform for node " + std::to_string(i));
	}
}