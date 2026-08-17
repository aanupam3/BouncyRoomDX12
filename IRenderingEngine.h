#include "Scene.h"
#include <winnt.h>

enum GraphicsAPI
{
	Direct3D12,
	Vulkan
};

#pragma once
class IRenderingEngine
{
public:
	virtual bool Init(Scene&) = 0;
	virtual bool Render(Scene&) = 0;
	virtual bool Shutdown() = 0;

	virtual GraphicsAPI GetRenderingEngineType() = 0;
};