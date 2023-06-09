#include "pch.h"
#include "FirstPersonCamera.h"
#include "VectorHelper.h"

using namespace DirectX;
using namespace Library;

FirstPersonCamera::FirstPersonCamera(float fieldOfView, float aspectRatio, float nearPlaneDistance, float farPlaneDistance) :
    PerspectiveCamera(fieldOfView, aspectRatio, nearPlaneDistance, farPlaneDistance)
{
}

const std::vector<std::function<void()>>& FirstPersonCamera::PositionUpdatedCallbacks() const
{
    return _positionUpdatedCallbacks;
}

void FirstPersonCamera::AddPositionUpdatedCallback(std::function<void()> callback)
{
    _positionUpdatedCallbacks.push_back(callback);
}

void FirstPersonCamera::SetPosition(float x, float y, float z)
{
    Camera::SetPosition(x, y, z);
    InvokePositionUpdatedCallbacks();
}

void FirstPersonCamera::SetPosition(DirectX::FXMVECTOR position)
{
    Camera::SetPosition(position);
    InvokePositionUpdatedCallbacks();
}

void FirstPersonCamera::SetPosition(const DirectX::XMFLOAT3& position)
{
    Camera::SetPosition(position);
    InvokePositionUpdatedCallbacks();
}

float& FirstPersonCamera::MouseSensitivity()
{
    return _mouseSensitivity;
}

float& FirstPersonCamera::RotationRate()
{
    return _rotationRate;
}

float& FirstPersonCamera::MovementRate()
{
    return _movementRate;
}

void FirstPersonCamera::UpdateViewMatrix()
{
    Camera::UpdateViewMatrix();
}

void FirstPersonCamera::Initialize()
{
    Camera::Initialize();
}

void FirstPersonCamera::Update(UpdateEventArgs& args)
{
    Camera::Update(args);
}

void FirstPersonCamera::CheckForInput(KeyCode::Key key, UpdateEventArgs& args)
{
    XMFLOAT2 movementAmount = Vector2Helper::Zero;
    XMFLOAT2 rotationAmount = Vector2Helper::Zero;
    bool positionChanged{ false };

    switch (key)
    {
    case KeyCode::W:
        movementAmount.y = -1.0f;
        positionChanged = true;
        break;
    case KeyCode::A:
        movementAmount.x = 1.0f;
        positionChanged = true;
        break;
    case KeyCode::S:
        movementAmount.y = 1.0f;
        positionChanged = true;
        break;
    case KeyCode::D:
        movementAmount.x = -1.0f;
        positionChanged = true;
        break;
    default:
        break;
    }

    if (positionChanged)
    {
        UpdatePosition(movementAmount, rotationAmount, args);
    }
}

void FirstPersonCamera::UpdatePosition(const DirectX::XMFLOAT2& movementAmount, const DirectX::XMFLOAT2& rotationAmount, UpdateEventArgs& args)
{
    float elapsedTime = args.ElapsedTime;
    XMVECTOR rotationVector = XMLoadFloat2(&rotationAmount) * _rotationRate * elapsedTime;
    XMVECTOR right = XMLoadFloat3(&_right);

    XMMATRIX pitchMatrix = XMMatrixRotationAxis(right, XMVectorGetY(rotationVector));
    XMMATRIX yawMatrix = XMMatrixRotationY(XMVectorGetX(rotationVector));

    ApplyRotation(XMMatrixMultiply(pitchMatrix, yawMatrix));

    XMVECTOR position = XMLoadFloat3(&_position);
    XMVECTOR movement = XMLoadFloat2(&movementAmount) * _movementRate * elapsedTime;

    XMVECTOR strafe = right * XMVectorGetX(movement);
    position += strafe;

    XMVECTOR forward = XMLoadFloat3(&_direction) * XMVectorGetY(movement);
    position += forward;

    XMStoreFloat3(&_position, position);

    InvokePositionUpdatedCallbacks();
    _viewMatrixDataDirty = true;

}

inline void FirstPersonCamera::InvokePositionUpdatedCallbacks()
{
    for (auto& callback : _positionUpdatedCallbacks)
    {
        callback();
    }
}
