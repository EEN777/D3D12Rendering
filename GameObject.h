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

class GameObject
{
	Microsoft::WRL::ComPtr<ID3D12Resource> _vertexBuffer;
	D3D12_VERTEX_BUFFER_VIEW _vertexBufferView;

	Microsoft::WRL::ComPtr<ID3D12Resource> _indexBuffer;
	D3D12_INDEX_BUFFER_VIEW _indexBufferView;

	const std::wstring _modelFile;
	const std::wstring _textureFile;
	Library::ContentManager* _contentManager;
	UINT _indexCount;
	UINT _vertexCount;

	DirectX::XMMATRIX _modelMatrix;
	DirectX::XMMATRIX _mvpMatrix;

	std::shared_ptr<FirstPersonCamera> _camera;

	ComPtr<ID3D12Resource> _textureResource;
	ComPtr<ID3D12Resource> _textureUploadResource;
	ComPtr<ID3D12DescriptorHeap> _texDescriptorHeap;

	void UpdateBufferResource(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList4> commandList, ID3D12Resource** destinationResource, ID3D12Resource** intermediateResource, std::size_t numElements, std::size_t elementSize, const void* bufferData, D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE);

public:
	GameObject(std::wstring modelFile, std::wstring textureFile, Library::ContentManager& contentManager);
	~GameObject() = default;

	void Initialize(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList4> commandList, std::shared_ptr<FirstPersonCamera> camera);
	void DrawObject(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList4> commandList);
	void UpdateObject();
	Microsoft::WRL::ComPtr<ID3D12Resource>& VertexBuffer();
	Microsoft::WRL::ComPtr<ID3D12Resource>& IndexBuffer();
	D3D12_VERTEX_BUFFER_VIEW VertexBufferView();
	D3D12_INDEX_BUFFER_VIEW IndexBufferView();
	UINT IndexCount();
	UINT VertCount();
	ComPtr<ID3D12DescriptorHeap> TextureDecsriptorHeap();
	ComPtr<ID3D12Resource>& TextureResource();
};