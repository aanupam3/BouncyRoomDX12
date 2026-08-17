#include "Transform.h"
#include <algorithm>

void Transform::SetAndExtractFromTransformationMatrix(DirectX::XMMATRIX transformationMatrix)
{
	m_transformationMatrix = transformationMatrix;

	DirectX::XMVECTOR outTranslationVector;
	DirectX::XMVECTOR outRotationVectorQuat;
	DirectX::XMVECTOR outScalingVector;

	DirectX::XMMatrixDecompose(&outScalingVector, &outRotationVectorQuat, &outTranslationVector, m_transformationMatrix);

	// Retrieve Position
	m_position = { DirectX::XMVectorGetX(outTranslationVector), DirectX::XMVectorGetY(outTranslationVector), DirectX::XMVectorGetZ(outTranslationVector) };
	m_translationMatrix = DirectX::XMMatrixTranslationFromVector(outTranslationVector);

	// Rotation calculations
	DirectX::XMFLOAT4 rotationQuaternion = { DirectX::XMVectorGetX(outRotationVectorQuat), DirectX::XMVectorGetY(outRotationVectorQuat), DirectX::XMVectorGetZ(outRotationVectorQuat), DirectX::XMVectorGetZ(outRotationVectorQuat) };
	const float x = rotationQuaternion.x;
	const float y = rotationQuaternion.y;
	const float z = rotationQuaternion.z;
	const float w = rotationQuaternion.w;

	// Pitch: rotation around X
	float sinPitch = 2.0f * (w * x - y * z);
	sinPitch = std::clamp(sinPitch, -1.0f, 1.0f);
	m_rotationRadians.x = std::asin(sinPitch);

	// Yaw: rotation around Y
	m_rotationRadians.y = std::atan2(
		2.0f * (x * z + w * y),
		1.0f - 2.0f * (x * x + y * y)
	);

	// Roll: rotation around Z
	m_rotationRadians.z = std::atan2(
		2.0f * (x * y + w * z),
		1.0f - 2.0f * (x * x + z * z)
	);
	DirectX::XMVECTOR RotationVector = DirectX::XMLoadFloat3(&m_rotationRadians);
	m_rotationMatrix = DirectX::XMMatrixRotationRollPitchYawFromVector(RotationVector);

	// Retrieve Scale
	m_scale = { DirectX::XMVectorGetX(outScalingVector), DirectX::XMVectorGetY(outScalingVector), DirectX::XMVectorGetZ(outScalingVector) };
	m_scalingMatrix = DirectX::XMMatrixScalingFromVector(outScalingVector);
}

void Transform::SetPosition(float x, float y, float z)
{
	m_position = { x, y, z };
	DirectX::XMVECTOR TranslationVector = DirectX::XMLoadFloat3(&m_position); //XMVECTOR is faster than FLOAT3 as it uses SIMD and possibly even dedicated hardware registers
	m_translationMatrix = DirectX::XMMatrixTranslationFromVector(TranslationVector);

	m_transformationMatrix = m_scalingMatrix * m_rotationMatrix * m_translationMatrix;
}

void Transform::SetRotationDegrees(float xDegrees, float yDegrees, float zDegrees)
{
	float x = DirectX::XMConvertToRadians(xDegrees);
	float y = DirectX::XMConvertToRadians(yDegrees);
	float z = DirectX::XMConvertToRadians(zDegrees);
	SetRotationRadians(x, y, z);
}

void Transform::SetRotationRadians(float x, float y, float z)
{
	m_rotationRadians = { x, y, z };

	DirectX::XMVECTOR RotationVector = DirectX::XMLoadFloat3(&m_rotationRadians);
	m_rotationMatrix = DirectX::XMMatrixRotationRollPitchYawFromVector(RotationVector);

	m_transformationMatrix = m_scalingMatrix * m_rotationMatrix * m_translationMatrix;
}

void Transform::SetScale(float x, float y, float z)
{
	m_scale = { x, y, z };
	DirectX::XMVECTOR ScalingVector = DirectX::XMLoadFloat3(&m_scale);
	m_scalingMatrix = DirectX::XMMatrixScalingFromVector(ScalingVector);

	m_transformationMatrix = m_scalingMatrix * m_rotationMatrix * m_translationMatrix;
}

void Transform::SetScale(float scale)
{
	SetScale(scale, scale, scale);
}

void Transform::TranslateBy(float xOffset, float yOffset, float zOffset)
{
	//std::cout << "\nTranslating by: " << xOffset << ", " << yOffset << ", " << zOffset << "\n";
	SetPosition(m_position.x + xOffset, m_position.y + yOffset, m_position.z + zOffset);
}

void Transform::RotateByDegrees(float x, float y, float z)
{
	float pitch = DirectX::XMConvertToRadians(x);
	float yaw = DirectX::XMConvertToRadians(y);
	float roll = DirectX::XMConvertToRadians(z);

	//std::cout << "\nRotating by: " << x << ", " << y << ", " << z << "\n";
	SetRotationDegrees(m_rotationRadians.x + pitch,
		m_rotationRadians.y + yaw,
		m_rotationRadians.z + roll);
}

void Transform::ScaleBy(float x, float y, float z)
{
	//std::cout << "\nScaling by: " << x << ", " << y << ", " << z << "\n";
	SetScale(m_scale.x * x, m_scale.y * y, m_scale.z * z);
}

Transform::Transform()
{
	SetPosition(0, 0, 0);
	SetRotationRadians(0, 0, 0);
	SetScale(1, 1, 1);
}

Transform::Transform(DirectX::XMMATRIX transformationMatrix)
{
	SetAndExtractFromTransformationMatrix(transformationMatrix);
}

Transform::Transform(DirectX::XMFLOAT3 position, DirectX::XMFLOAT3 rotation, DirectX::XMFLOAT3 scale)
{
	SetPosition(position.x, position.y, position.z);
	SetRotationRadians(rotation.x, rotation.y, rotation.z);
	SetScale(scale.x, scale.y, scale.z);
}

Transform::Transform(DirectX::XMFLOAT3 position, DirectX::XMFLOAT3 rotation)
{
	SetPosition(position.x, position.y, position.z);
	SetRotationRadians(rotation.x, rotation.y, rotation.z);
	SetScale(1, 1, 1);
}

Transform::Transform(DirectX::XMFLOAT3 position)
{
	SetPosition(position.x, position.y, position.z);
	SetRotationRadians(0, 0, 0);
	SetScale(1, 1, 1);
}

Transform::Transform(const Transform& srcTransform)
{
	DirectX::XMFLOAT3 srcPosition = srcTransform.GetPosition();
	SetPosition(srcPosition.x, srcPosition.y, srcPosition.z);

	DirectX::XMFLOAT3 srcRotation = srcTransform.GetRotationRadians();
	SetRotationRadians(srcRotation.x, srcRotation.y, srcRotation.z);

	DirectX::XMFLOAT3 srcScale = srcTransform.GetScale();
	SetScale(srcScale.x, srcScale.y, srcScale.z);
}
