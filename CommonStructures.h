#pragma once
#include <DirectXMath.h>

struct VertexPosColor
{
	DirectX::XMFLOAT3 Position;
	DirectX::XMFLOAT3 Color;
};

struct MaterialProperties
{
	bool isLight{ false };
};