#pragma once
#include <DirectXMath.h>

struct VertexPosColor
{
	DirectX::XMFLOAT3 Position;
	DirectX::XMFLOAT3 Color;
};

struct PointLight
{
	DirectX::XMFLOAT3 Position;
	DirectX::XMFLOAT3 Color;
	float Intensity;
	float Radius;
};