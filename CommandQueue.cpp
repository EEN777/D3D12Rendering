#include "CommandQueue.h"
#include "pch.h"


CommandQueue::CommandQueue(ComPtr<ID3D12Device5> device, D3D12_COMMAND_LIST_TYPE type) :
	_fenceValue{ 0 }, _commandListType(type), _d3d12Device{ device }
{
	D3D12_COMMAND_QUEUE_DESC desc{};
	desc.Type = type;
	desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
	desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	desc.NodeMask = 0;

	ThrowIfFailedI(_d3d12Device->CreateCommandQueue(&desc, IID_PPV_ARGS(&_d3d12CommandQueue)));
	ThrowIfFailedI(_d3d12Device->CreateFence(_fenceValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&_d3d12Fence)));

	_fenceEvent = ::CreateEvent(nullptr, false, false, nullptr);
	assert(_fenceEvent && "Failed to create event handle!");
}

ComPtr<ID3D12CommandAllocator> CommandQueue::CreateCommandAllocator()
{
	ComPtr<ID3D12CommandAllocator> commandAllocator;
	ThrowIfFailedI(_d3d12Device->CreateCommandAllocator(_commandListType, IID_PPV_ARGS(&commandAllocator)));
	return commandAllocator;
}

ComPtr<ID3D12GraphicsCommandList4> CommandQueue::CreateCommandList(ComPtr<ID3D12CommandAllocator> allocator)
{
	ComPtr<ID3D12GraphicsCommandList4> commandList;
	ThrowIfFailedI(_d3d12Device->CreateCommandList(0, _commandListType, allocator.Get(), nullptr, IID_PPV_ARGS(&commandList)));
	return commandList;
}

ComPtr<ID3D12GraphicsCommandList4> CommandQueue::GetCommandList()
{
	ComPtr<ID3D12CommandAllocator> commandAllocator;
	ComPtr<ID3D12GraphicsCommandList4> commandList;

	if (!_commandAllocatorQueue.empty() && IsFenceComplete(_commandAllocatorQueue.front()._fenceValue))
	{
		commandAllocator = _commandAllocatorQueue.front()._commandAllocator;
		_commandAllocatorQueue.pop();

		ThrowIfFailedI(commandAllocator->Reset());
	}
	else
	{
		commandAllocator = CreateCommandAllocator();
	}

	if (!_commandListQueue.empty())
	{
		commandList = _commandListQueue.front();
		_commandListQueue.pop();

		ThrowIfFailedI(commandList->Reset(commandAllocator.Get(), nullptr));
	}

	else
	{
		commandList = CreateCommandList(commandAllocator);
	}

	ThrowIfFailedI(commandList->SetPrivateDataInterface(__uuidof(ID3D12CommandAllocator), commandAllocator.Get()));

	return commandList;
}

uint64_t CommandQueue::ExecuteCommandList(ComPtr<ID3D12GraphicsCommandList4> commandList)
{
	commandList->Close();
	ID3D12CommandAllocator* commandAllocator;
	UINT dataSize = sizeof(commandAllocator);
	ThrowIfFailedI(commandList->GetPrivateData(__uuidof(ID3D12CommandAllocator), &dataSize, &commandAllocator));

	ID3D12CommandList* const commandLists[]
	{
		commandList.Get()
	};

	_d3d12CommandQueue->ExecuteCommandLists(1, commandLists);
	uint64_t fenceValue = Signal();
	_commandAllocatorQueue.emplace(CommandAllocatorEntry{ fenceValue, commandAllocator });
	_commandListQueue.push(commandList);

	commandAllocator->Release();

	return fenceValue;
}

uint64_t CommandQueue::Signal()
{
	uint64_t fenceValue = ++_fenceValue;
	_d3d12CommandQueue->Signal(_d3d12Fence.Get(), fenceValue);
	return fenceValue;
}

bool CommandQueue::IsFenceComplete(uint64_t fenceValue)
{
	return _d3d12Fence->GetCompletedValue() >= fenceValue;
}

void CommandQueue::WaitForFenceValue(uint64_t fenceValue)
{
	if (!IsFenceComplete(fenceValue))
	{
		_d3d12Fence->SetEventOnCompletion(fenceValue, _fenceEvent);
		::WaitForSingleObject(_fenceEvent, DWORD_MAX);
	}
}

void CommandQueue::Flush()
{
	WaitForFenceValue(Signal());
}

Microsoft::WRL::ComPtr<ID3D12CommandQueue> CommandQueue::GetD3D12CommandQueue() const
{
	return _d3d12CommandQueue;
}
