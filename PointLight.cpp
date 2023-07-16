#include "pch.h"
#include "PointLight.h"
#include "VectorHelper.h"


using namespace DirectX;

void PointLight::UpdatePosition(const DirectX::XMFLOAT3& movementAmount, const DirectX::XMFLOAT2& rotationAmount, UpdateEventArgs& args)
{
    float elapsedTime = args.ElapsedTime;

    XMVECTOR position = XMLoadFloat3(&_position);
    XMVECTOR movement = XMLoadFloat3(&movementAmount) * _movementRate * elapsedTime;

    XMVECTOR right = XMLoadFloat3(&_right);
    XMVECTOR up = XMLoadFloat3(&_up);

    XMVECTOR strafe = right * XMVectorGetX(movement);
    position += strafe;

    XMVECTOR forward = XMLoadFloat3(&_direction) * XMVectorGetY(movement);
    position += forward;

    XMVECTOR lift = up * XMVectorGetZ(movement);
    position += lift;

    XMStoreFloat3(&_position, position);
}

void PointLight::SetPosition(float x, float y, float z)
{
	DirectX::XMVECTOR position = XMVectorSet(x, y, z, 1.0f);
	SetPosition(position);
}

void PointLight::SetPosition(DirectX::FXMVECTOR position)
{
	XMStoreFloat3(&_position, position);
}

void PointLight::SetPosition(const DirectX::XMFLOAT3& position)
{
	_position = position;
}

void PointLight::Update([[maybe_unused]] UpdateEventArgs& args)
{
}

void PointLight::CheckForInput(std::vector<KeyCode::Key>& keys, std::pair<int, int> mouse, UpdateEventArgs& args)
{
    XMFLOAT3 movementAmount = Library::Vector3Helper::Zero;
    XMFLOAT2 rotationAmount = Library::Vector2Helper::Zero;
    bool positionChanged{ false };


    for (auto& key : keys)
    {
        switch (key)
        {
        case KeyCode::NumPad8:
            movementAmount.y = -1.0f;
            positionChanged = true;
            break;
        case KeyCode::NumPad4:
            movementAmount.x = 1.0f;
            positionChanged = true;
            break;
        case KeyCode::NumPad5:
            movementAmount.y = 1.0f;
            positionChanged = true;
            break;
        case KeyCode::NumPad6:
            movementAmount.x = -1.0f;
            positionChanged = true;
            break;
        case KeyCode::NumPad9:
            movementAmount.z = 1.0f;
            positionChanged = true;
            break;
        case KeyCode::NumPad7:
            movementAmount.z = -1.0f;
            positionChanged = true;
            break;
        default:
            break;
        }
    }

    if (positionChanged)
    {
        UpdatePosition(movementAmount, rotationAmount, args);
    }
}