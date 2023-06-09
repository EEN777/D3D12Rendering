#pragma once

#include "Camera.h"

class PerspectiveCamera : public Camera
{
public:
	PerspectiveCamera(float fieldOfView = DefaultFieldOfView, float aspectRatio = DefaultAspectRatio, float nearPlaneDistance = DefaultNearPlaneDistance, float farPlaneDistance = DefaultFarPlaneDistance);
	virtual ~PerspectiveCamera() = default;

	float AspectRatio() const;
	void SetAspectRatio(float aspectRatio);

	float FieldOfView() const;
	void SetFieldOfView(float fieldOfView);

	virtual void UpdateProjectionMatrix() override;

	inline static const float DefaultFieldOfView{ DirectX::XM_PIDIV4 };
	inline static const float DefaultAspectRatio{ 4.0f / 3.0f };

protected:
	float _fieldOfView;
	float _aspectRatio;
};