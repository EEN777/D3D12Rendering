#include "pch.h"
#include "ContentManager.h"
#include "ContentTypeReaderManager.h"
#include "Helpers.h"

using namespace std;

namespace Library
{
	const wstring ContentManager::DefaultRootDirectory{ L"Content\\" };

	ContentManager::ContentManager(const wstring& rootDirectory) :
		mRootDirectory(rootDirectory)
	{
	}

	void ContentManager::AddAsset(const wstring& assetName, const shared_ptr<RTTI>& asset)
	{
		mLoadedAssets[assetName] = asset;
	}

	void ContentManager::RemoveAsset(const wstring& assetName)
	{
		mLoadedAssets.erase(assetName);
	}

	void ContentManager::Clear()
	{
		mLoadedAssets.clear();
	}

	shared_ptr<RTTI> ContentManager::ReadAsset(const int64_t targetTypeId, const wstring& assetName)
	{
		const auto& contentTypeReaders = ContentTypeReaderManager::ContentTypeReaders();
		auto it = contentTypeReaders.find(targetTypeId);
		if (it == contentTypeReaders.end())
		{
			throw std::runtime_error("Content type reader not registered.");
		}

		auto& reader = it->second;
		return reader->Read(assetName);
	}
}