#include "Transform.h"

#pragma once
class Camera
{
public:
	Camera(UINT screenWidth, UINT screenHeight);
	void SetTransform(Transform transform);

	const DirectX::XMMATRIX& GetProjectionMatrix() { return m_projectionMatrix; }
	const Transform& GetTransform() { return m_transform; }

private:
	Transform m_transform{};
	DirectX::XMMATRIX m_projectionMatrix;
};

