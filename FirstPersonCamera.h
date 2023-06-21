#pragma once

#include "PerspectiveCamera.h"
#include <functional>
#include <vector>

class FirstPersonCamera : public PerspectiveCamera
{
	friend class CubeDemo;
public:
	FirstPersonCamera(float fieldOfView = DefaultFieldOfView, float aspectRatio = DefaultAspectRatio, float nearPlaneDistance = DefaultNearPlaneDistance, float farPlaneDistance = DefaultFarPlaneDistance);
	virtual ~FirstPersonCamera() = default;

	const std::vector<std::function<void()>>& PositionUpdatedCallbacks() const;
	void AddPositionUpdatedCallback(std::function<void()> callback);

	virtual void SetPosition(float x, float y, float z) override;
	virtual void SetPosition(DirectX::FXMVECTOR position) override;
	virtual void SetPosition(const DirectX::XMFLOAT3& position) override;

	float& MouseSensitivity();
	float& RotationRate();
	float& MovementRate();

	virtual void UpdateViewMatrix() override;

	void ResetOrientation();

	virtual void Initialize() override;
	virtual void Update(UpdateEventArgs& args) override;
	void CheckForInput(std::vector<KeyCode::Key>& keys, std::pair<int, int> mouse, UpdateEventArgs& args);

	inline static const float DefaultMouseSensitivity{ 0.8f };
	inline static const float DefaultRotationRate{ DirectX::XMConvertToRadians(100.0f) };
	inline static const float DefaultMovementRate{ 100.0f };
	void UpdatePosition(const DirectX::XMFLOAT3& movementAmount, const DirectX::XMFLOAT2& rotationAmount, UpdateEventArgs& args);
	inline bool IsAcceptingMouseMovementInputs() { return _acceptingMouseMovementInputs; };
	inline void SetIsAcceptingMouseMovementInputs(bool state) { _acceptingMouseMovementInputs = state; }

private:
	inline void InvokePositionUpdatedCallbacks();
	std::pair<int, int> prevPair;
	bool _entered{ false };
protected:
	bool _acceptingMouseMovementInputs{ false };
	float _mouseSensitivity{ DefaultMouseSensitivity };
	float _rotationRate{ DefaultRotationRate };
	float _movementRate{ DefaultMovementRate };
	std::vector<std::function<void()>> _positionUpdatedCallbacks;

};
