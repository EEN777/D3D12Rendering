#pragma once
#include <d3d12.h>
#include <wrl.h>
#include <cstdint>
#include <queue>

class CommandQueue
{
	struct CommandAllocatorEntry
	{
		uint64_t _fenceValue;
		Microsoft::WRL::ComPtr<ID3D12CommandAllocator> _commandAllocator;
	};

	using CommandAllocatorQueue = std::queue<CommandAllocatorEntry>;
	using CommandListQueue = std::queue<Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2>>;

	D3D12_COMMAND_LIST_TYPE _commandListType;
	Microsoft::WRL::ComPtr<ID3D12Device2> _d3d12Device;
	Microsoft::WRL::ComPtr<ID3D12CommandQueue> _d3d12CommandQueue;
	Microsoft::WRL::ComPtr<ID3D12Fence> _d3d12Fence;
	HANDLE _fenceEvent;
	uint64_t _fenceValue;

	CommandAllocatorQueue _commandAllocatorQueue;
	CommandListQueue _commandListQueue;

protected:

	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> CreateCommandAllocator();
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> CreateCommandList(Microsoft::WRL::ComPtr<ID3D12CommandAllocator> allocator);

public:
	CommandQueue(Microsoft::WRL::ComPtr<ID3D12Device2> device, D3D12_COMMAND_LIST_TYPE type);
	virtual ~CommandQueue() = default;

	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> GetCommandList();

	uint64_t ExecuteCommandList(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> commandList);

	uint64_t Signal();
	bool IsFenceComplete(uint64_t fenceValue);
	void WaitForFenceValue(uint64_t fenceValue);
	void Flush();

	Microsoft::WRL::ComPtr<ID3D12CommandQueue> GetD3D12CommandQueue() const;
};