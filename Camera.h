#pragma once

#include <DirectXMath.h>
#include <vector>
#include <functional>
#include "Events.h"

class Camera
{
public:
	Camera(float nearPlaneDistance = DefaultNearPlaneDistance, float farPlaneDistance = DefaultFarPlaneDistance);
	virtual ~Camera() = default;

	const DirectX::XMFLOAT3& Position() const;
	const DirectX::XMFLOAT3& Direction() const;
	const DirectX::XMFLOAT3& Up() const;
	const DirectX::XMFLOAT3& Right() const;

	DirectX::XMVECTOR PositionVector() const;
	DirectX::XMVECTOR DirectionVector() const;
	DirectX::XMVECTOR UpVector() const;
	DirectX::XMVECTOR RightVector() const;

	float NearPlaneDistance() const;
	void SetNearPlaneDistance(float distance);

	float FarPlaneDistance() const;
	void SetFarPlaneDistance(float distance);

	DirectX::XMMATRIX ViewMatrix() const;
	DirectX::XMMATRIX ProjectionMatrix() const;
	DirectX::XMMATRIX ViewProjectionMatrix() const;

	const std::vector<std::function<void()>>& ViewMatrixUpdatedCallbacks() const;
	void AddViewMatrixUpdatedCallback(std::function<void()> callback);

	const std::vector<std::function<void()>>& ProjectionMatrixUpdatedCallbacks() const;
	void AddProjectionMatrixUpdatedCallback(std::function<void()> callback);

	virtual void SetPosition(float x, float y, float z);
	virtual void SetPosition(DirectX::FXMVECTOR position);
	virtual void SetPosition(const DirectX::XMFLOAT3& position);

	virtual void Reset();
	virtual void Initialize();
	virtual void Update(UpdateEventArgs& args);
	virtual void UpdateViewMatrix();
	virtual void UpdateProjectionMatrix() = 0;
	virtual void ApplyRotation(DirectX::CXMMATRIX transform);
	virtual void ApplyRotation(const DirectX::XMFLOAT4X4& transform);

	inline static const float DefaultNearPlaneDistance{ 0.01f };
	inline static const float DefaultFarPlaneDistance{ 10000.0f };

protected:
	float _nearPlaneDistance;
	float _farPlaneDistance;

	DirectX::XMFLOAT3 _position;
	DirectX::XMFLOAT3 _direction;
	DirectX::XMFLOAT3 _up;
	DirectX::XMFLOAT3 _right;

	DirectX::XMFLOAT4X4 _viewMatrix;
	DirectX::XMFLOAT4X4 _projectionMatrix;

	bool _viewMatrixDataDirty{ true };
	bool _projectionMatrixDataDirty{ true };

	std::vector<std::function<void()>> _viewMatrixUpdatedCallbacks;
	std::vector<std::function<void()>> _projectionMatrixUpdatedCallbacks;
};