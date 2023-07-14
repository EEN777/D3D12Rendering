#pragma once
#include "Game.h"
#include "Window.h"
#include "FirstPersonCamera.h"
#include <DirectXMath.h>
#include "TextureLoader.h"
#include "ContentManager.h"
#include "GameObject.h"
#include <dxcapi.h>
#include "nv_helpers_dx12/TopLevelASGenerator.h"
#include "nv_helpers_dx12/ShaderBindingTableGenerator.h"

class CubeDemo : public Game
{
	uint64_t _fenceValues[Window::BufferCount]{};
	//Microsoft::WRL::ComPtr<ID3D12Resource> _vertexBuffer;
	//D3D12_VERTEX_BUFFER_VIEW _vertexBufferView;

	//Microsoft::WRL::ComPtr<ID3D12Resource> _indexBuffer;
	//D3D12_INDEX_BUFFER_VIEW _indexBufferView;

	Microsoft::WRL::ComPtr<ID3D12Resource> _depthBuffer;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> _DSVHeap;

	Microsoft::WRL::ComPtr<ID3D12RootSignature> _rootSignature;
	Microsoft::WRL::ComPtr<ID3D12RootSignature> _rtRootSignature;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> _pipelineState;

	std::shared_ptr<FirstPersonCamera> _camera;

	D3D12_VIEWPORT _viewport;
	D3D12_RECT _scissorRect;

	std::vector<std::shared_ptr<GameObject>> _gameObjects;
	std::shared_ptr<GameObject> gameObject;
	std::shared_ptr<GameObject> gameObject2;

	float _fieldOfView;

	DirectX::XMMATRIX _modelMatrix;
	DirectX::XMMATRIX _viewMatrix;
	DirectX::XMMATRIX _projectionMatrix;

	bool _contentLoaded;
	bool _isRaster;

	struct AccelerationStructureBuffers
	{
		ComPtr<ID3D12Resource> scratch;
		ComPtr<ID3D12Resource> result;
		ComPtr<ID3D12Resource> descriptors;
	};

	struct BottomLevelASInstance
	{
		ComPtr<ID3D12Resource> bottomLevelAS;
		DirectX::XMMATRIX transformation;
	};

	std::vector<BottomLevelASInstance> m_bottomLevelASInstances;
	ComPtr<ID3D12Resource> m_bottomLevelAS;
	nv_helpers_dx12::TopLevelASGenerator m_topLevelASGenerator;
	AccelerationStructureBuffers m_topLevelASBuffers;
	std::vector<std::pair<ComPtr<ID3D12Resource>, DirectX::XMMATRIX>> m_instances;

	ComPtr<ID3D12RootSignature> CreateRayGenSignature();
	ComPtr<ID3D12RootSignature> CreateMissSignature();
	ComPtr<ID3D12RootSignature> CreateHitSignature();
	void CreateRayTracingPipeline();
	ComPtr<IDxcBlob> m_rayGenLibrary;
	ComPtr<IDxcBlob> m_hitLibrary;
	ComPtr<IDxcBlob> m_missLibrary;
	ComPtr<ID3D12RootSignature> m_rayGenSignature;
	ComPtr<ID3D12RootSignature> m_hitSignature;
	ComPtr<ID3D12RootSignature> m_missSignature;
	ComPtr<ID3D12StateObject> m_rtStateObject;
	ComPtr<ID3D12StateObjectProperties> m_rtStateObjectProps;

	void CreateRaytracingOutputBuffer();
	void CreateShaderResourceHeap();
	ComPtr<ID3D12Resource> m_outputResource;
	ComPtr<ID3D12DescriptorHeap> m_srvUavHeap;

	void CreateShaderBindingTable();
	nv_helpers_dx12::ShaderBindingTableGenerator m_sbtHelper;
	ComPtr<ID3D12Resource> m_sbtStorage;

	void CreateCameraBuffer();
	void UpdateCameraBuffer();
	ComPtr<ID3D12Resource> m_cameraBuffer;
	ComPtr<ID3D12DescriptorHeap> m_constHeap;
	uint32_t m_cameraBufferSize = 0;

	Library::ContentManager _contentManager{};

	//temp
	UINT indexCount;
	ComPtr<ID3D12Resource> _textureResource;
	ComPtr<ID3D12Resource> _textureUploadResource;
	ComPtr<ID3D12DescriptorHeap> _texDescriptorHeap;

	TextureLoader::TextureLoader _textureLoader{};

public:

	struct PipelineStateStream
	{
		CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE rootSignature;
		CD3DX12_PIPELINE_STATE_STREAM_INPUT_LAYOUT inputLayout;
		CD3DX12_PIPELINE_STATE_STREAM_PRIMITIVE_TOPOLOGY PrimitiveTopologyType;
		CD3DX12_PIPELINE_STATE_STREAM_VS vs;
		CD3DX12_PIPELINE_STATE_STREAM_PS ps;
		CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL_FORMAT DSVFormat;
		CD3DX12_PIPELINE_STATE_STREAM_RENDER_TARGET_FORMATS RTVFormats;
	} pipelineStateStream;

	using super = Game;

	std::vector<KeyCode::Key> cameraKeyArgument{};
	std::pair<int, int> cameraMouseArgument{INT_MIN, INT_MIN};
	std::pair<int, int> startingMousePos{};


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
	void TransitionResource(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList4> commandList,
		Microsoft::WRL::ComPtr<ID3D12Resource> resource,
		D3D12_RESOURCE_STATES beforeState, D3D12_RESOURCE_STATES afterState);

	void ClearRTV(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList4> commandList,
		D3D12_CPU_DESCRIPTOR_HANDLE rtv, float* clearColor);

	void ClearDepth(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList4> commandList,
		D3D12_CPU_DESCRIPTOR_HANDLE dsv, float depth = 1.0f);

	void UpdateBufferResource(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList4> commandList,
		ID3D12Resource** destinationResource, ID3D12Resource** intermediateResource,
		std::size_t numElements, std::size_t elementSize, const void* bufferData,
		D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE);

	AccelerationStructureBuffers CreateBottomLevelAS(std::vector<std::pair<ComPtr<ID3D12Resource>, uint32_t>> vVertexBuffers, std::vector<std::pair<ComPtr<ID3D12Resource>, uint32_t>> vIndexBuffers, ID3D12GraphicsCommandList4* commandList);

	void CreateTopLevelAS(const std::vector<BottomLevelASInstance>& instances, ID3D12GraphicsCommandList4* commandList);

	void CreateAccelerationStructures(ID3D12GraphicsCommandList4* commandList);

	void ResizeDepthBuffer(int width, int height);
};