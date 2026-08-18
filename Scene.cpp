#include "Scene.h"


Scene::Scene(const RenderWindow& renderWindow) : m_renderWindow(renderWindow), m_camera(renderWindow.Width, renderWindow.Height) {}

Scene::~Scene()
{
	ClearScene();
}

//void Scene::AddModel(Model model)
//{
//	m_models.emplace_back(model);
//}

void Scene::AddObject(ModelInstance object)
{
	m_objects.emplace_back(object);
}

// Change to unordered_map or unordered_set eventually
void Scene::RemoveObject(ModelInstance& object)
{
	int objectIndex = -1;
	for (int i = 0; i < m_objects.size(); i++)
	{
		if (m_objects[i].Id == object.Id)
		{
			objectIndex = i;
			break;
		}
	}
	if (objectIndex == -1)
	{
		std::cerr << "Object with Id" << object.Id << " not found and so cannot be removed!\n";
		return;
	};

	m_objects.erase(m_objects.begin() + objectIndex);
}


void Scene::ClearScene()
{
	m_objects.clear();
	m_models.clear();
}
