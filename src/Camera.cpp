#include "Camera.h"

Camera::Camera(UINT screenWidth, UINT screenHeight)
{
	const float n = 0.01f; // near clipping plane
	const float f = 1000.0f; // far clipping plane

	// assume fov = 90, so zoom = 1 along width
	m_projectionMatrix =
	{
		1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 1.0f * screenWidth / screenHeight, 0.0f, 0.0f,
		0.0f, 0.0f, f / (f - n), 1.0f,
		0.0f, 0.0f, -n * f / (f - n), 0.0f
	};
}

void Camera::SetTransform(Transform transform)
{
	m_transform = transform;
}