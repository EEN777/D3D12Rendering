#pragma once
#include "Game.h"
#include "Window.h"
#include "FirstPersonCamera.h"
#include <DirectXMath.h>

class CubeDemo : public Game
{
	uint64_t _fenceValues[Window::BufferCount]{};
	Microsoft::WRL::ComPtr<ID3D12Resource> _vertexBuffer;
	D3D12_VERTEX_BUFFER_VIEW _vertexBufferView;

	Microsoft::WRL::ComPtr<ID3D12Resource> _indexBuffer;
	D3D12_INDEX_BUFFER_VIEW _indexBufferView;

	Microsoft::WRL::ComPtr<ID3D12Resource> _depthBuffer;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> _DSVHeap;

	Microsoft::WRL::ComPtr<ID3D12RootSignature> _rootSignature;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> _pipelineState;

	std::shared_ptr<FirstPersonCamera> _camera;

	D3D12_VIEWPORT _viewport;
	D3D12_RECT _scissorRect;

	float _fieldOfView;

	DirectX::XMMATRIX _modelMatrix;
	DirectX::XMMATRIX _viewMatrix;
	DirectX::XMMATRIX _projectionMatrix;

	bool _contentLoaded;


public:
	using super = Game;


	CubeDemo(const std::wstring& name, int width, int height, bool vSync = false);
	virtual bool LoadContent() override;
	virtual bool UnloadContent() override;

protected: 
	virtual void OnUpdate(UpdateEventArgs& args) override;
	virtual void OnRender(RenderEventArgs& args) override;
	virtual void OnKeyPressed(KeyEventArgs& args) override;
	virtual void OnKeyReleased(KeyEventArgs& args) override;
	virtual void OnMouseWheel(MouseWheelEventArgs& args) override;
	virtual void OnResize(ResizeEventArgs& args) override;
	virtual void OnMouseMoved(MouseMotionEventArgs& args) override;
	virtual void OnMouseButtonPressed(MouseButtonEventArgs& args) override;
	virtual void OnMouseButtonReleased(MouseButtonEventArgs& args) override;
private:
	void TransitionResource(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> commandList,
		Microsoft::WRL::ComPtr<ID3D12Resource> resource,
		D3D12_RESOURCE_STATES beforeState, D3D12_RESOURCE_STATES afterState);

	void ClearRTV(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> commandList,
		D3D12_CPU_DESCRIPTOR_HANDLE rtv, float* clearColor);

	void ClearDepth(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> commandList,
		D3D12_CPU_DESCRIPTOR_HANDLE dsv, float depth = 1.0f);

	void UpdateBufferResource(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> commandList,
		ID3D12Resource** destinationResource, ID3D12Resource** intermediateResource,
		std::size_t numElements, std::size_t elementSize, const void* bufferData,
		D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE);

	void ResizeDepthBuffer(int width, int height);
};