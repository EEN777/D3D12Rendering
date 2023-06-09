#include "pch.h"
#include "Camera.h"
#include "VectorHelper.h"

using namespace DirectX;
using namespace Library;

Camera::Camera(float nearPlaneDistance, float farPlaneDistance):
    _nearPlaneDistance{ nearPlaneDistance }, _farPlaneDistance{ farPlaneDistance }
{
}

const DirectX::XMFLOAT3& Camera::Position() const
{
    return _position;
}

const DirectX::XMFLOAT3& Camera::Direction() const
{
    return _direction;
}

const DirectX::XMFLOAT3& Camera::Up() const
{
    return _up;
}

const DirectX::XMFLOAT3& Camera::Right() const
{
    return _right;
}

DirectX::XMVECTOR Camera::PositionVector() const
{
    return XMLoadFloat3(&_position);
}

DirectX::XMVECTOR Camera::DirectionVector() const
{
    return XMLoadFloat3(&_direction);
}

DirectX::XMVECTOR Camera::UpVector() const
{
    return XMLoadFloat3(&_up);
}

DirectX::XMVECTOR Camera::RightVector() const
{
    return XMLoadFloat3(&_right);
}

float Camera::NearPlaneDistance() const
{
    return _nearPlaneDistance;
}

void Camera::SetNearPlaneDistance(float distance)
{
    _nearPlaneDistance = distance;
}

float Camera::FarPlaneDistance() const
{
    return _farPlaneDistance;
}

void Camera::SetFarPlaneDistance(float distance)
{
    _farPlaneDistance = distance;
}

DirectX::XMMATRIX Camera::ViewMatrix() const
{
    return XMLoadFloat4x4(&_viewMatrix);
}

DirectX::XMMATRIX Camera::ProjectionMatrix() const
{
    return XMLoadFloat4x4(&_projectionMatrix);
}

DirectX::XMMATRIX Camera::ViewProjectionMatrix() const
{
    XMMATRIX viewMatrix = XMLoadFloat4x4(&_viewMatrix);
    XMMATRIX projectionMatrix = XMLoadFloat4x4(&_projectionMatrix);

    return XMMatrixMultiply(viewMatrix, projectionMatrix);
}

const std::vector<std::function<void()>>& Camera::ViewMatrixUpdatedCallbacks() const
{
    return _viewMatrixUpdatedCallbacks;
}

void Camera::AddViewMatrixUpdatedCallback(std::function<void()> callback)
{
    _viewMatrixUpdatedCallbacks.push_back(callback);
}

const std::vector<std::function<void()>>& Camera::ProjectionMatrixUpdatedCallbacks() const
{
    return _projectionMatrixUpdatedCallbacks;
}

void Camera::AddProjectionMatrixUpdatedCallback(std::function<void()> callback)
{
    _projectionMatrixUpdatedCallbacks.push_back(callback);
}

void Camera::SetPosition(float x, float y, float z)
{
    DirectX::XMVECTOR position = XMVectorSet(x, y, z, 1.0f);
    SetPosition(position);
}

void Camera::SetPosition(DirectX::FXMVECTOR position)
{
    XMStoreFloat3(&_position, position);
    _viewMatrixDataDirty = true;
}

void Camera::SetPosition(const DirectX::XMFLOAT3& position)
{
    _position = position;
    _viewMatrixDataDirty = true;
}

void Camera::Reset()
{
    _position = Vector3Helper::Zero;
    _direction = Vector3Helper::Forward;
    _up = Vector3Helper::Up;
    _right = Vector3Helper::Right;
    _viewMatrixDataDirty;

    UpdateViewMatrix();
}

void Camera::Initialize()
{
    UpdateProjectionMatrix();
    Reset();
}

void Camera::Update(UpdateEventArgs& args)
{
    if (_viewMatrixDataDirty)
    {
        UpdateViewMatrix();
    }
}

void Camera::UpdateViewMatrix()
{
    XMVECTOR eyePosition = XMLoadFloat3(&_position);
    XMVECTOR direction = XMLoadFloat3(&_direction);
    XMVECTOR upDirection = XMLoadFloat3(&_up);

    XMMATRIX viewMatrix = XMMatrixLookToLH(eyePosition, direction, upDirection);
    XMStoreFloat4x4(&_viewMatrix, viewMatrix);

    for (auto& callback : _viewMatrixUpdatedCallbacks)
    {
        callback();
    }

    _viewMatrixDataDirty = false;
}

void Camera::ApplyRotation(DirectX::CXMMATRIX transform)
{
    XMVECTOR direction = XMLoadFloat3(&_direction);
    XMVECTOR up = XMLoadFloat3(&_up);

    direction = XMVector3TransformNormal(direction, transform);
    direction = XMVector3Normalize(direction);

    XMVECTOR right = XMVector3Cross(direction, up);
    up = XMVector3Cross(right, direction);

    XMStoreFloat3(&_direction, direction);
    XMStoreFloat3(&_up, up);
    XMStoreFloat3(&_right, right);

    _viewMatrixDataDirty = true;
}

void Camera::ApplyRotation(const DirectX::XMFLOAT4X4& transform)
{
    XMMATRIX transformMatrix = XMLoadFloat4x4(&transform);
    ApplyRotation(transformMatrix);
}
