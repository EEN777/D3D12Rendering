//#pragma once
//
//#include "DescriptorAllocation.h"
//
//#include <d3d12.h>
//
//#include <wrl.h>
//
//#include <map>
//#include <memory>
//#include <mutex>
//#include <queue>
//
//class DescriptoAllocatorPage : public std::enable_shared_from_this<DescriptoAllocatorPage>
//{
//public: 
//	DescriptorAllocatorPage(D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t numDescriptors);
//
//	D3D12_DESCRIPTOR_HEAP_TYPE GetHeapType() const;
//
//	bool HasSpace(uint32_t numDescriptors) const;
//};
