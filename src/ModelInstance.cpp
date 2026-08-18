#include "Macros.h"
#include "ModelInstance.h"

ModelInstance::ModelInstance(Model& model, const Transform& worldTransform) : BaseModel(&model), m_worldTransform(worldTransform)
{
	// Create the same node tree but with independent transforms
	std::vector<LocalNode>& baseModelNodes = model.GetNodes(); // base model nodes are in local space, and will now be updated in the instance to world space

	int numNodes = baseModelNodes.size();
	m_nodesWorldSpace.resize(numNodes);

	for (int nodeIndex = 0; nodeIndex < numNodes; nodeIndex++)
	{
		LocalNode& baseNode = baseModelNodes[nodeIndex];
		WorldNode& worldNode = m_nodesWorldSpace[nodeIndex];
		worldNode.ChildrenNodeIndexes = baseNode.ChildrenNodeIndexes;
		worldNode.ParentNodeIndex = baseNode.ParentNodeIndex;
		worldNode.MeshIndex = baseNode.MeshIndex;

		DirectX::XMMATRIX instanceNodeLocalTransformationMatrix = baseNode.NodeTransform.GetTransformationMatrix();
		worldNode.NodeTransform.SetAndExtractFromTransformationMatrix(instanceNodeLocalTransformationMatrix);

		/*if (nodeIndex == 0)
		{
			Utils::printMatrix(baseNode.NodeTransform.GetTransformationMatrix(), "Base Node transformation matrix: " + std::to_string(0));
		}*/
	}

	// Upgrade nodes to model space
	for (UINT nodeIndex = 0; nodeIndex < numNodes; nodeIndex++)
	{
		WorldNode& node = m_nodesWorldSpace[nodeIndex];
		for (int childNodeIndex : node.ChildrenNodeIndexes)
		{
			WorldNode& childNode = m_nodesWorldSpace[childNodeIndex];
			childNode.ParentNodeIndex = nodeIndex;

			DirectX::XMMATRIX modelSpaceMatrix =
				childNode.NodeTransform.GetTransformationMatrix() * node.NodeTransform.GetTransformationMatrix();

			childNode.NodeTransform.SetAndExtractFromTransformationMatrix(modelSpaceMatrix);
		}
	}

	// Finally Update to world level
	UpdateTransform(m_worldTransform);
}

ModelInstance::~ModelInstance()
{
	m_nodesWorldSpace.clear();
}

void ModelInstance::UpdateTransform(const Transform& newTransform)
{
	const DirectX::XMMATRIX& worldTransformationMatrix = newTransform.GetTransformationMatrix();
	for (int i = 0; i < m_nodesWorldSpace.size(); i++)
	{
		WorldNode& node = m_nodesWorldSpace[i];
		node.NodeTransform.SetAndExtractFromTransformationMatrix(
			DirectX::XMMatrixMultiply(node.NodeTransform.GetTransformationMatrix(), worldTransformationMatrix)
		);

		/*if (i == 0)
		{
			Utils::printMatrix(node.NodeTransform.GetTransformationMatrix(), "Updated world space transform for node " + std::to_string(i));
		}*/

	}
}