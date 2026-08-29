#include "Transform.h"

#pragma once
class Camera
{
public:

	Camera(UINT screenWidth, UINT screenHeight);
	void SetTransform(Transform transform);
	void SetVPMatrix();

	ComPtr<ID3D12Resource> VPMatrixResource{};

	std::array<float, MATRIX4X4_NUMELEMENTS>& GetVPMatrixBuffer() { return m_vpMatrixBuffer; }
	const DirectX::XMMATRIX& GetProjectionMatrix() { return m_projectionMatrix; }
	const Transform& GetTransform() { return m_transform; }

private:
	Transform m_transform{};
	DirectX::XMMATRIX m_projectionMatrix;
	DirectX::XMMATRIX m_vpMatrix{};
	std::array<float, MATRIX4X4_NUMELEMENTS> m_vpMatrixBuffer{};
};

