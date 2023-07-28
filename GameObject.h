#pragma once
#include "pch.h"
#include "FirstPersonCamera.h"
#include "Model.h"
#include "Mesh.h"
#include "ModelMaterial.h"
#include <DirectXTex.h>
#include "TextureLoader.h"
#include "MatrixHelper.h"
#include "VectorHelper.h"
#include "ContentTypeReaderManager.h"
#include "Utility.h"
#include <d3dx12.h>
#include <d3dcompiler.h>
#include "ContentManager.h"
#include <algorithm>
#include "CommonStructures.h"

class GameObject
{
	Microsoft::WRL::ComPtr<ID3D12Resource> _vertexBuffer;
	D3D12_VERTEX_BUFFER_VIEW _vertexBufferView;

	Microsoft::WRL::ComPtr<ID3D12Resource> _indexBuffer;
	D3D12_INDEX_BUFFER_VIEW _indexBufferView;

	Microsoft::WRL::ComPtr<ID3D12Resource> _materialPropertyBuffer;
	D3D12_BUFFER_SRV _materialPropertyBufferView;

	const std::wstring _modelFile;
	const std::wstring _textureFile;
	const std::wstring _normalFile;
	Library::ContentManager* _contentManager;
	UINT _indexCount;
	UINT _vertexCount;

	float scale{ 1.0f };

	std::size_t _modelIndex;

	DirectX::XMMATRIX _modelMatrix;
	DirectX::XMMATRIX _mvpMatrix;
	DirectX::XMMATRIX _worldMatrix{ XMMatrixIdentity() };

	std::wstring _meshName{ L"UNNAMED" };

	std::shared_ptr<FirstPersonCamera> _camera;

	ComPtr<ID3D12Resource> _textureResource;
	ComPtr<ID3D12Resource> _textureUploadResource;
	ComPtr<ID3D12DescriptorHeap> _texDescriptorHeap;

	ComPtr<ID3D12Resource> _normalResource;
	ComPtr<ID3D12Resource> _normalUploadResource;
	ComPtr<ID3D12DescriptorHeap> _normDescriptorHeap;

	MaterialProperties materialProperties;

	void UpdateBufferResource(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList4> commandList, ID3D12Resource** destinationResource, ID3D12Resource** intermediateResource, std::size_t numElements, std::size_t elementSize, const void* bufferData, D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE);

	inline static std::unordered_map<std::wstring, ID3D12Resource*> resourceMap;
public:
	GameObject(const std::wstring& modelFile, const std::wstring& textureFile, const std::wstring& normalFile, Library::ContentManager& contentManager, std::size_t modelIndex = 0, const std::wstring& meshName = L"UNNAMED");
	~GameObject() = default;

	void Initialize(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList4> commandList, std::shared_ptr<FirstPersonCamera> camera);
	void DrawObject(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList4> commandList);
	void UpdateObject();
	Microsoft::WRL::ComPtr<ID3D12Resource>& VertexBuffer();
	Microsoft::WRL::ComPtr<ID3D12Resource>& IndexBuffer();
	Microsoft::WRL::ComPtr<ID3D12Resource>& MaterialPropertiesBuffer();
	D3D12_VERTEX_BUFFER_VIEW VertexBufferView();
	D3D12_INDEX_BUFFER_VIEW IndexBufferView();
	D3D12_BUFFER_SRV MaterialPropertiesView();
	UINT IndexCount();
	UINT VertCount();
	ComPtr<ID3D12DescriptorHeap> TextureDecsriptorHeap();
	ComPtr<ID3D12Resource>& TextureResource();
	ComPtr<ID3D12DescriptorHeap> NormalDescriptorHeap();
	ComPtr<ID3D12Resource>& NormalResource();
	const DirectX::XMMATRIX& WorldMatrix();
	void UpdatePosition(float x, float y, float z);
	float* Scale();
	MaterialProperties& GetMaterialProperties();
};