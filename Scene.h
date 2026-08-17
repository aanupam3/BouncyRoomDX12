#include "Camera.h"
#include "Model.h"
#include "ModelInstance.h"

struct RenderWindow
{
	LPCTSTR WindowName;
	LPCTSTR WindowTitle;
	unsigned int Width = 1920;
	unsigned int Height = 1080;
	bool FullScreen = false;
	HWND WindowHandle = NULL;
};

#pragma once
class Scene
{
public:
	Scene(const RenderWindow& renderWindow);
	~Scene();

	//void AddModel(Model model);
	void AddObject(ModelInstance object);
	void RemoveObject(ModelInstance& object);
	void ClearScene();

	// Getters
	Camera& GetCamera() { return m_camera; }
	ModelInstance& GetObjectFromId(std::string id) {};
	std::vector<ModelInstance>& GetObjects() { return m_objects; }
	std::vector<Model>& GetModels() { return m_models; }

private:
	std::vector<Model> m_models{};
	std::vector<ModelInstance> m_objects{};
	Camera m_camera;
	const RenderWindow& m_renderWindow;
};

