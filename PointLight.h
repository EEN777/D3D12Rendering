#pragma once
#include <DirectXMath.h>
#include "Events.h"
#include "VectorHelper.h"
#include <vector>


class PointLight final
{
	friend class CubeDemo;

	DirectX::XMFLOAT3 _position{Library::Vector3Helper::Zero};
	DirectX::XMFLOAT3 _direction{Library::Vector3Helper::Forward};
	DirectX::XMFLOAT3 _up{Library::Vector3Helper::Up};
	DirectX::XMFLOAT3 _right{Library::Vector3Helper::Right};

	void UpdatePosition(const DirectX::XMFLOAT3& movementAmount, const DirectX::XMFLOAT2& rotationAmount, UpdateEventArgs& args);

	inline static const float DefaultMovementRate{ 100.0f };
	float _movementRate{DefaultMovementRate};

public:
	virtual void SetPosition(float x, float y, float z);
	virtual void SetPosition(DirectX::FXMVECTOR position);
	virtual void SetPosition(const DirectX::XMFLOAT3& position);

	virtual void Update(UpdateEventArgs& args);
	void CheckForInput(std::vector<KeyCode::Key>& keys, std::pair<int, int> mouse, UpdateEventArgs& args);
};