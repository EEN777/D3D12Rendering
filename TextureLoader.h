#pragma once
#include "pch.h"
#include <d3d12.h>
#include <DirectXTex.h>
#include "DDSTextureLoader.h"
#include <unordered_map>

namespace TextureLoader
{
	struct Texture
	{
		// Unique material name for lookup.
		std::string Name;

		std::wstring Filename;

		ComPtr<ID3D12Resource> Resource = nullptr;
		ComPtr<ID3D12Resource> UploadHeap = nullptr;
	};

	class TextureLoader
	{

	public:
		inline void LoadTextureFromFile(ID3D12Device9* device, ID3D12Resource** resource, ID3D12GraphicsCommandList4* commandList, const wchar_t* filename)
		{
			auto texture = std::make_unique<Texture>();
			texture->Name = "EarthTexture";
			texture->Filename = filename;
			ThrowIfFailedI(DirectX::CreateDDSTextureFromFile12(device,
				commandList, texture->Filename.c_str(),
				texture->Resource, texture->UploadHeap));

			mTextures.emplace_back(std::move(texture));
		}

		std::vector<std::unique_ptr<Texture>> mTextures;
	};


	inline void LoadTexture(ID3D12Device9* device,ComPtr<ID3D12Resource> resource, ComPtr<ID3D12Resource> uploadResource, ID3D12GraphicsCommandList4* commandList, const wchar_t* filename)
	{
		ThrowIfFailedI(DirectX::CreateDDSTextureFromFile12(device,
			commandList, filename,
			resource, uploadResource));
		//DirectX::TexMetadata metaData;
		//DirectX::ScratchImage image;

		//ThrowIfFailedI(DirectX::LoadFromWICFile(filename, DirectX::WIC_FLAGS_NONE, &metaData, image));

		//const DirectX::Image* img = image.GetImage(0, 0, 0);
		//CD3DX12_HEAP_PROPERTIES heapProperties(D3D12_HEAP_TYPE_DEFAULT);
		//CD3DX12_RESOURCE_DESC resourceDesc = CD3DX12_RESOURCE_DESC::Tex2D(img->format, img->width, img->height, 1, 1);

		//ThrowIfFailedI(device->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &resourceDesc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(resource)));

		//ID3D12Resource* uploadResource = nullptr;
		//ThrowIfFailedI(device->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		//	D3D12_HEAP_FLAG_NONE,
		//	&CD3DX12_RESOURCE_DESC::Buffer(image.GetPixelsSize()),
		//	D3D12_RESOURCE_STATE_GENERIC_READ,
		//	nullptr,
		//	IID_PPV_ARGS(&uploadResource)
		//));

		//D3D12_SUBRESOURCE_DATA subresourceData{};
		//subresourceData.pData = img->pixels;
		//subresourceData.RowPitch = img->rowPitch;
		//subresourceData.SlicePitch = img->slicePitch;

		//UpdateSubresources<1>(commandList, *resource, uploadResource, 0, 0, 1, &subresourceData);

		//CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
		//	*resource,
		//	D3D12_RESOURCE_STATE_COPY_DEST,
		//	D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

		//commandList->ResourceBarrier(1, &barrier);

		//uploadResource->Release();
	}

}
