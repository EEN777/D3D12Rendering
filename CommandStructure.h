#pragma once
#include <wrl.h>
#include <dxgi1_6.h>
#include <d3d12.h>
#include <vector>

template <typename T>
using ComPtr = Microsoft::WRL::ComPtr<T>;

class CommandStructure
{
	ComPtr<ID3D12CommandQueue> g_CommandQueue;
	ComPtr<ID3D12GraphicsCommandList> g_CommandList;
	std::vector<ComPtr<ID3D12CommandAllocator>> g_CommandAllocators;
};