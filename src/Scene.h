#include "Camera.h"
#include "Model.h"

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
	void AddModelInstance(Model& model, Transform& transform);
	void AddModelInstances(Model& model, std::vector<std::array<float, MATRIX4X4_NUMELEMENTS>>& transformBuffers);
	//void RemoveModelInstance(ModelInstance& object);
	void ClearScene();

	enum SceneState
	{
		READY,
		RUNNING,
		RESETTING
	} state{};

	// Getters
	Camera& GetCamera() { return m_camera; }
	std::vector<Model>& GetModels() { return m_models; }


	DirectX::XMFLOAT3 LightDirection{};
	//ComPtr<ID3D12Resource> LightDirectionResource{};

private:
	std::vector<Model> m_models{};
	Camera m_camera;
	const RenderWindow& m_renderWindow;
};

