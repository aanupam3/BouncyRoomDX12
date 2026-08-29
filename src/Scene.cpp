#include "Scene.h"


Scene::Scene(const RenderWindow& renderWindow) : m_renderWindow(renderWindow), m_camera(renderWindow.Width, renderWindow.Height) {}

Scene::~Scene()
{
	ClearScene();
}

void Scene::AddModelInstance(Model& model, Transform& transform)
{
	model.WorldRootTransformBuffersAllInstances.emplace_back(transform.GetTransformMatrixArray());
}

void Scene::AddModelInstances(Model& model, std::vector<std::array<float, MATRIX4X4_NUMELEMENTS>>& transformBuffers)
{
	model.WorldRootTransformBuffersAllInstances.insert(model.WorldRootTransformBuffersAllInstances.end(), transformBuffers.begin(), transformBuffers.end());
}

void Scene::ClearScene()
{
	for (Model& model : m_models)
	{
		model.WorldRootTransformBuffersAllInstances.clear();
	}
	m_models.clear();
}


// Change to unordered_map or unordered_set eventually
//void Scene::RemoveModelInstance(ModelInstance& object)
//{
//	int objectIndex = -1;
//	for (int i = 0; i < m_objects.size(); i++)
//	{
//		if (m_objects[i].Id == object.Id)
//		{
//			objectIndex = i;
//			break;
//		}
//	}
//	if (objectIndex == -1)
//	{
//		std::cerr << "Object with Id" << object.Id << " not found and so cannot be removed!\n";
//		return;
//	};
//
//	m_objects.erase(m_objects.begin() + objectIndex);
//}

