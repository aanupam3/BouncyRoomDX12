#pragma once

#include "DirectXMath.h"
#include <d3d12.h>
#include <iostream>
#include <string>
#include <vector>

class Transform
{
private:
	DirectX::XMFLOAT3 m_position{ 0,0,0 };
	DirectX::XMFLOAT3 m_rotationRadians{ 0,0,0 };
	DirectX::XMFLOAT3 m_scale{ 1,1,1 };

	DirectX::XMMATRIX m_translationMatrix{};
	DirectX::XMMATRIX m_rotationMatrix{};
	DirectX::XMMATRIX m_scalingMatrix{};
	DirectX::XMMATRIX m_transformationMatrix{};

public:
	const DirectX::XMFLOAT3& GetPosition() const { return m_position; }
	const DirectX::XMFLOAT3& GetRotationRadians() const { return m_rotationRadians; }
	const DirectX::XMFLOAT3& GetScale() const { return m_scale; }
	const DirectX::XMMATRIX& GetTransformationMatrix() const { return m_transformationMatrix; }

	void SetPosition(float x, float y, float z);
	void SetRotationDegrees(float x, float y, float z);
	void SetRotationRadians(float x, float y, float z);
	void SetScale(float scale);
	void SetScale(float x, float y, float z);
	void SetAndExtractFromTransformationMatrix(DirectX::XMMATRIX transformationMatrix); // glTF formats often directly specify the matrix instead of the individual position, rotation, and scale

	void TranslateBy(float x, float y, float z);
	void RotateByDegrees(float x, float y, float z);
	void ScaleBy(float x, float y, float z);

	Transform();
	Transform(DirectX::XMMATRIX transformationMatrix);
	Transform(DirectX::XMFLOAT3 position, DirectX::XMFLOAT3 rotation, DirectX::XMFLOAT3 scale);
	Transform(DirectX::XMFLOAT3 position, DirectX::XMFLOAT3 rotation);
	Transform(DirectX::XMFLOAT3 position);
	Transform(const Transform&) = default;
};

