#pragma once

#include "Defines.h"

#include <wrl.h>
#include <d3d12.h>

#include <memory>
#include <deque>

class UploadBuffer
{
public:


	struct Allocation
	{
		void* CPU;
		D3D12_GPU_VIRTUAL_ADDRESS GPU;
	};

	explicit UploadBuffer(std::size_t pageSize = _2MB);
	inline std::size_t GetPageSize() const { return m_PageSize; };
	Allocation Allocate(std::size_t sizeInBytes, std::size_t alignment);
	void Reset();

private:
	struct Page
	{
		Page(std::size_t sizeInBytes);
		~Page();

		bool HasSpace(std::size_t sizeInBytes, std::size_t alignment) const;
		Allocation Allocate(std::size_t sizeInBytes, std::size_t alignment);
		void Reset();

	private:
		Microsoft::WRL::ComPtr<ID3D12Resource> m_d3d12Resource;

		void* m_CPUPtr;
		D3D12_GPU_VIRTUAL_ADDRESS m_GPUPtr;

		std::size_t m_PageSize;
		std::size_t m_Offset;
	};

	using PagePool = std::deque<std::shared_ptr<Page>>;

	std::shared_ptr<Page> RequestPage();

	PagePool m_PagePool;
	PagePool m_AvailablePages;

	std::shared_ptr<Page> m_CurrentPage;

	std::size_t m_PageSize;

};
