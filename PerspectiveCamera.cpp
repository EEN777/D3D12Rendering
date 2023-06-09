#include "pch.h"
#include "PerspectiveCamera.h"

using namespace DirectX;

PerspectiveCamera::PerspectiveCamera(float fieldOfView, float aspectRatio, float nearPlaneDistance, float farPlaneDistance) :
	Camera{ nearPlaneDistance, farPlaneDistance }, _fieldOfView{ fieldOfView }, _aspectRatio{ aspectRatio }
{
}

float PerspectiveCamera::AspectRatio() const
{
	return _aspectRatio;
}

void PerspectiveCamera::SetAspectRatio(float aspectRatio)
{
	_aspectRatio = aspectRatio;
	_projectionMatrixDataDirty = true;
}

float PerspectiveCamera::FieldOfView() const
{
	return _fieldOfView;
}

void PerspectiveCamera::SetFieldOfView(float fieldOfView)
{
	_fieldOfView = fieldOfView;
	_projectionMatrixDataDirty = true;
}

void PerspectiveCamera::UpdateProjectionMatrix()
{
	if (_projectionMatrixDataDirty)
	{
		XMMATRIX projectionMatrix = XMMatrixPerspectiveFovRH(_fieldOfView, _aspectRatio, _nearPlaneDistance, _farPlaneDistance);
		XMStoreFloat4x4(&_projectionMatrix, projectionMatrix);

		for (auto& callback : _projectionMatrixUpdatedCallbacks)
		{
			callback();
		}
	}
}
