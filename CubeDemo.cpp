#include "pch.h"
#include "CubeDemo.h"

#include "Application.h"
#include "CommandQueue.h"
#include "Helpers.h"
#include "Window.h"

#include "Model.h"
#include "Mesh.h"
#include "ModelMaterial.h"
#include "DXRHelper.h"
#include "nv_helpers_dx12/BottomLevelASGenerator.h"
#include "nv_helpers_dx12/RaytracingPipelineGenerator.h"
#include "nv_helpers_dx12/RootSignatureGenerator.h"

#include <DirectXTex.h>
#include "TextureLoader.h"

#include "MatrixHelper.h"
#include "VectorHelper.h"

#include "ContentTypeReaderManager.h"

#include "Utility.h"

#include "imgui.h"
#include "imgui_impl_dx12.h"
#include "imgui_impl_win32.h"

#include <d3dx12.h>
#include <d3dcompiler.h>

#include <algorithm>

#include "CommonStructures.h"

#pragma region Imgui Preamble
struct FrameContext
{
    ID3D12CommandAllocator* CommandAllocator;
    UINT64                  FenceValue;
};

// Data
static int const                    NUM_FRAMES_IN_FLIGHT = 3;
static FrameContext                 g_frameContext[NUM_FRAMES_IN_FLIGHT] = {};
static UINT                         g_frameIndex = 0;

static int const                    NUM_BACK_BUFFERS = 3;
static ID3D12Device* g_pd3dDevice = nullptr;
static ID3D12DescriptorHeap* g_pd3dRtvDescHeap = nullptr;
static ID3D12DescriptorHeap* g_pd3dSrvDescHeap = nullptr;
static ID3D12CommandQueue* g_pd3dCommandQueue = nullptr;
static ID3D12GraphicsCommandList* g_pd3dCommandList = nullptr;
static ID3D12Fence* g_fence = nullptr;
static HANDLE                       g_fenceEvent = nullptr;
static UINT64                       g_fenceLastSignaledValue = 0;
static IDXGISwapChain3* g_pSwapChain = nullptr;
static HANDLE                       g_hSwapChainWaitableObject = nullptr;
static ID3D12Resource* g_mainRenderTargetResource[NUM_BACK_BUFFERS] = {};
static D3D12_CPU_DESCRIPTOR_HANDLE  g_mainRenderTargetDescriptor[NUM_BACK_BUFFERS] = {};
#pragma endregion

using namespace DirectX;

ComPtr<ID3D12RootSignature> CubeDemo::CreateRayGenSignature()
{
    nv_helpers_dx12::RootSignatureGenerator rsc;
    rsc.AddHeapRangesParameter({ {0, 1, 0, D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 0}, {0, 1, 0, D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1}, {0, 1, 0, D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 2} });
    rsc.AddHeapRangesParameter({
    {1, 1 , 0, D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 0}  // CBV for Frame Info.
    });
    rsc.AddHeapRangesParameter({
    {1, 1 , 0, D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 0}  // CBV for Frame Info.
        });

    return rsc.Generate(Application::Get().GetDevice().Get(), true);
}

ComPtr<ID3D12RootSignature> CubeDemo::CreateHitSignature()
{
    nv_helpers_dx12::RootSignatureGenerator rsc;
    //rsc.AddRootParameter(D3D12_ROOT_PARAMETER_TYPE_SRV, 0);
    //rsc.AddRootParameter(D3D12_ROOT_PARAMETER_TYPE_SRV, 1);
    
    rsc.AddHeapRangesParameter({
        {0, 1, 3, D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1}
    });

    rsc.AddHeapRangesParameter({
        {2, 2, 0, D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0}  // SRV for the texture
    });

    rsc.AddHeapRangesParameter({
    {0, 1, 1, D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0}  // SRV for Vertices.
    });

    rsc.AddHeapRangesParameter({
    {0, 1 , 2, D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0}  // SRV for Indices.
    });

    rsc.AddHeapRangesParameter({
    {0, 1 , 0, D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 0}  // CBV for Point Light.
    });

    rsc.AddHeapRangesParameter({
    {1, 1 , 0, D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 0}  // CBV for Random Frame.
    });

    rsc.AddHeapRangesParameter({
    {0, _gameObjects.size(), 4, D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0}  // SRV for Material Properties.
    });

    return rsc.Generate(Application::Get().GetDevice().Get(), true);
}

ComPtr<ID3D12RootSignature> CubeDemo::CreateMissSignature()
{
    nv_helpers_dx12::RootSignatureGenerator rsc;

    return rsc.Generate(Application::Get().GetDevice().Get(), true);
}


void CubeDemo::CreateRayTracingPipeline()
{
    auto device = Application::Get().GetDevice().Get();
    nv_helpers_dx12::RayTracingPipelineGenerator pipeline{device};

    m_rayGenLibrary = nv_helpers_dx12::CompileShaderLibrary(L"RayGen.hlsl");
    m_hitLibrary = nv_helpers_dx12::CompileShaderLibrary(L"Hit.hlsl");
    m_missLibrary = nv_helpers_dx12::CompileShaderLibrary(L"Miss.hlsl");
    m_shadowLibrary = nv_helpers_dx12::CompileShaderLibrary(L"ShadowRay.hlsl");
    m_scatterLibrary = nv_helpers_dx12::CompileShaderLibrary(L"ScatterShader.hlsl");

    pipeline.AddLibrary(m_rayGenLibrary.Get(), { L"RayGen" });
    pipeline.AddLibrary(m_hitLibrary.Get(), { L"ClosestHit" });
    pipeline.AddLibrary(m_missLibrary.Get(), { L"Miss" });
    pipeline.AddLibrary(m_shadowLibrary.Get(), { L"ShadowClosestHit" , L"ShadowMiss" });
    pipeline.AddLibrary(m_scatterLibrary.Get(), { L"ScatterClosestHit" , L"ScatterMiss" });

    m_rayGenSignature = CreateRayGenSignature();
    m_missSignature = CreateMissSignature();
    m_hitSignature = CreateHitSignature();
    m_shadowSignature = CreateHitSignature();
    m_scatterSignature = CreateHitSignature();

    pipeline.AddHitGroup(L"HitGroup", L"ClosestHit");
    pipeline.AddHitGroup(L"ShadowHitGroup", L"ShadowClosestHit");
    pipeline.AddHitGroup(L"ScatterHitGroup", L"ScatterClosestHit");

    pipeline.AddRootSignatureAssociation(m_rayGenSignature.Get(), { L"RayGen" });
    pipeline.AddRootSignatureAssociation(m_missSignature.Get(), { L"Miss" });
    pipeline.AddRootSignatureAssociation(m_hitSignature.Get(), { L"HitGroup"});
    pipeline.AddRootSignatureAssociation(m_shadowSignature.Get(), { L"ShadowHitGroup" });
    pipeline.AddRootSignatureAssociation(m_scatterSignature.Get(), { L"ScatterHitGroup" });
    pipeline.AddRootSignatureAssociation(m_missSignature.Get(), { L"Miss", L"ShadowMiss"});

    pipeline.SetMaxPayloadSize(8 * sizeof(float));
    pipeline.SetMaxAttributeSize(2 * sizeof(float));
    pipeline.SetMaxRecursionDepth(7);

    m_rtStateObject = pipeline.Generate();
    ThrowIfFailed(m_rtStateObject->QueryInterface(IID_PPV_ARGS(&m_rtStateObjectProps)));
}

void CubeDemo::CreateRaytracingOutputBuffer()
{
    auto device = Application::Get().GetDevice().Get();

    D3D12_RESOURCE_DESC resDesc{};
    resDesc.DepthOrArraySize = 1;
    resDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    resDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    resDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
    resDesc.Width = 1920;
    resDesc.Height = 1080;
    resDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    resDesc.MipLevels = 1;
    resDesc.SampleDesc.Count = 1;

    ThrowIfFailed(device->CreateCommittedResource(&nv_helpers_dx12::kDefaultHeapProps, D3D12_HEAP_FLAG_NONE, &resDesc, D3D12_RESOURCE_STATE_COPY_SOURCE, nullptr, IID_PPV_ARGS(&m_outputResource)));

}

void CubeDemo::CreateAccumulatedOutputBuffer()
{
    auto device = Application::Get().GetDevice().Get();

    D3D12_RESOURCE_DESC resDesc{};
    resDesc.DepthOrArraySize = 1;
    resDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    resDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
    resDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
    resDesc.Width = 1920;
    resDesc.Height = 1080;
    resDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    resDesc.MipLevels = 1;
    resDesc.SampleDesc.Count = 1;

    ThrowIfFailed(device->CreateCommittedResource(&nv_helpers_dx12::kDefaultHeapProps, D3D12_HEAP_FLAG_NONE, &resDesc, D3D12_RESOURCE_STATE_COPY_SOURCE, nullptr, IID_PPV_ARGS(&m_accumulatedResource)));
}

void CubeDemo::CreateShaderResourceHeap()
{
    auto device = Application::Get().GetDevice().Get();

    uint32_t heapCount = 6 + (_gameObjects.size() * (5)); // _gamObjects.size() multiplied by 5 for Texture, Normal Map VertexBuffer, IndexBuffer, and Material Properties.

    m_srvUavHeap = nv_helpers_dx12::CreateDescriptorHeap(device, heapCount, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, true);

    D3D12_CPU_DESCRIPTOR_HANDLE srvHandle = m_srvUavHeap->GetCPUDescriptorHandleForHeapStart();

    D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
    uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
    device->CreateUnorderedAccessView(m_outputResource.Get(), nullptr, &uavDesc, srvHandle);

    srvHandle.ptr += device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);


    //Acceleration Stucture.
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc;
    srvDesc.Format = DXGI_FORMAT_UNKNOWN;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.RaytracingAccelerationStructure.Location = m_topLevelASBuffers.result->GetGPUVirtualAddress();
    device->CreateShaderResourceView(nullptr, &srvDesc, srvHandle);
    srvHandle.ptr += device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc{};
    cbvDesc.BufferLocation = m_cameraBuffer->GetGPUVirtualAddress();
    cbvDesc.SizeInBytes = m_cameraBufferSize;
    device->CreateConstantBufferView(&cbvDesc, srvHandle);
    srvHandle.ptr += device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    cbvDesc.BufferLocation = m_pointLightBuffer->GetGPUVirtualAddress();
    cbvDesc.SizeInBytes = m_pointLightBufferSize;
    device->CreateConstantBufferView(&cbvDesc, srvHandle);
    srvHandle.ptr += device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    cbvDesc.BufferLocation = m_randomFrameNumberBuffer->GetGPUVirtualAddress();
    cbvDesc.SizeInBytes = 256;
    device->CreateConstantBufferView(&cbvDesc, srvHandle);
    srvHandle.ptr += device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    device->CreateUnorderedAccessView(m_accumulatedResource.Get(), nullptr, &uavDesc, srvHandle);
    srvHandle.ptr += device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDescTex = {};
    srvDescTex.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDescTex.Texture2D.MipLevels = 1; // Only one level of detail
    srvDescTex.Texture2D.MostDetailedMip = 0;
    srvDescTex.Texture2D.ResourceMinLODClamp = 0.0f;
    srvDescTex.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

    for (auto& object : _gameObjects)
    {
        srvDescTex.Format = object->TextureResource()->GetDesc().Format;
        device->CreateShaderResourceView(object->TextureResource().Get(), &srvDescTex, srvHandle);
        srvHandle.ptr += device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        srvDescTex.Format = object->NormalResource()->GetDesc().Format;
        device->CreateShaderResourceView(object->NormalResource().Get(), &srvDescTex, srvHandle);
        srvHandle.ptr += device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    }


    D3D12_SHADER_RESOURCE_VIEW_DESC vertDesc = {};
    vertDesc.Format = DXGI_FORMAT_UNKNOWN;
    vertDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
    vertDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    vertDesc.Buffer.FirstElement = 0;
    vertDesc.Buffer.StructureByteStride = sizeof(float);
    vertDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;

    for (auto& object : _gameObjects)
    {
        vertDesc.Buffer.NumElements = object->VertexBufferView().SizeInBytes / sizeof(float);
        device->CreateShaderResourceView(object->VertexBuffer().Get(), &vertDesc, srvHandle);
        srvHandle.ptr += device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    }

    D3D12_SHADER_RESOURCE_VIEW_DESC indexDesc = {};
    indexDesc.Format = DXGI_FORMAT_UNKNOWN;
    indexDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
    indexDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    indexDesc.Buffer.FirstElement = 0;
    indexDesc.Buffer.StructureByteStride = sizeof(UINT);
    indexDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;    

    for (auto& object : _gameObjects)
    {
        indexDesc.Buffer.NumElements = object->IndexBufferView().SizeInBytes / sizeof(UINT);
        device->CreateShaderResourceView(object->IndexBuffer().Get(), &indexDesc, srvHandle);
        srvHandle.ptr += device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    }

    D3D12_SHADER_RESOURCE_VIEW_DESC materialProperties = {};
    materialProperties.Format = DXGI_FORMAT_UNKNOWN;
    materialProperties.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
    materialProperties.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    materialProperties.Buffer.FirstElement = 0;
    materialProperties.Buffer.StructureByteStride = sizeof(MaterialProperties);
    materialProperties.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;

    for (auto& object : _gameObjects)
    {
        materialProperties.Buffer.NumElements = 1;
        device->CreateShaderResourceView(object->MaterialPropertiesBuffer().Get(), &materialProperties, srvHandle);
        srvHandle.ptr += device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    }
}

void CubeDemo::CreateShaderBindingTable()
{
    auto device = Application::Get().GetDevice().Get();

    m_sbtHelper.Reset();
    D3D12_GPU_DESCRIPTOR_HANDLE srvUavHeapHandle = m_srvUavHeap->GetGPUDescriptorHandleForHeapStart();

    D3D12_GPU_DESCRIPTOR_HANDLE pointLightSrvHandle = srvUavHeapHandle;
    pointLightSrvHandle.ptr = srvUavHeapHandle.ptr + (UINT{ 3 } *device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV));

    D3D12_GPU_DESCRIPTOR_HANDLE randomFrameSrvHandle = srvUavHeapHandle;
    randomFrameSrvHandle.ptr = pointLightSrvHandle.ptr + (device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV));

    D3D12_GPU_DESCRIPTOR_HANDLE accumulatedSrvHandle = randomFrameSrvHandle;
    accumulatedSrvHandle.ptr = randomFrameSrvHandle.ptr + (device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV));

    D3D12_GPU_DESCRIPTOR_HANDLE textureSrvHandle = srvUavHeapHandle;
    textureSrvHandle.ptr = accumulatedSrvHandle.ptr + (device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV));

    D3D12_GPU_DESCRIPTOR_HANDLE vertexSrvHandle = textureSrvHandle;
    vertexSrvHandle.ptr = textureSrvHandle.ptr + ((_gameObjects.size() * 2) * device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV));

    D3D12_GPU_DESCRIPTOR_HANDLE indexSrvHandle = vertexSrvHandle;
    indexSrvHandle.ptr = vertexSrvHandle.ptr + (_gameObjects.size() * device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV));

    D3D12_GPU_DESCRIPTOR_HANDLE materialPropertiesSrvHandle = indexSrvHandle;
    materialPropertiesSrvHandle.ptr = indexSrvHandle.ptr + (_gameObjects.size() * device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV));


    auto heapPointer = reinterpret_cast<uint64_t*>(srvUavHeapHandle.ptr);
    auto pointLightPointer = reinterpret_cast<uint64_t*>(pointLightSrvHandle.ptr);
    auto randomFramePointer = reinterpret_cast<uint64_t*>(randomFrameSrvHandle.ptr);
    auto accumulatedPointer = reinterpret_cast<uint64_t*>(accumulatedSrvHandle.ptr);
    auto texPointer = reinterpret_cast<uint64_t*>(textureSrvHandle.ptr);
    auto vertexPointer = reinterpret_cast<uint64_t*>(vertexSrvHandle.ptr);
    auto indexPointer = reinterpret_cast<uint64_t*>(indexSrvHandle.ptr);
    auto materialProperties = reinterpret_cast<uint64_t*>(materialPropertiesSrvHandle.ptr);
    m_sbtHelper.AddRayGenerationProgram(L"RayGen", { heapPointer, randomFramePointer, accumulatedPointer });
    m_sbtHelper.AddMissProgram(L"Miss", {});
    m_sbtHelper.AddHitGroup(L"HitGroup", { {heapPointer, texPointer, vertexPointer, indexPointer, pointLightPointer, randomFramePointer, materialProperties, nullptr, nullptr, nullptr}}); // Padding with nullptr as needed.
    m_sbtHelper.AddHitGroup(L"ShadowHitGroup", { {heapPointer, texPointer, vertexPointer, indexPointer, pointLightPointer, randomFramePointer, materialProperties, nullptr, nullptr, nullptr}});
    m_sbtHelper.AddHitGroup(L"ScatterHitGroup", { {heapPointer, texPointer, vertexPointer, indexPointer, pointLightPointer, randomFramePointer, materialProperties, nullptr, nullptr, nullptr} }); // Padding with nullptr as needed.
    m_sbtHelper.AddMissProgram(L"ShadowMiss", { {}});
    m_sbtHelper.AddMissProgram(L"ScatterMiss", { {}});

    const uint32_t sbtSize = m_sbtHelper.ComputeSBTSize();
    m_sbtStorage = nv_helpers_dx12::CreateBuffer(device, sbtSize, D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATE_GENERIC_READ, nv_helpers_dx12::kUploadHeapProps);
    if (!m_sbtStorage)
    {
        throw std::runtime_error("Allocation failed!");
    }

    m_sbtHelper.Generate(m_sbtStorage.Get(), m_rtStateObjectProps.Get());
}

void CubeDemo::CreateCameraBuffer()
{
    auto device = Application::Get().GetDevice().Get();

    uint32_t nbMatrix = 4;

    m_cameraBufferSize = nbMatrix * sizeof(XMMATRIX);

    m_cameraBuffer = nv_helpers_dx12::CreateBuffer(device, m_cameraBufferSize, D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATE_GENERIC_READ, nv_helpers_dx12::kUploadHeapProps);
    m_constHeap = nv_helpers_dx12::CreateDescriptorHeap(device, 1, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, true);

    D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc{};

    cbvDesc.BufferLocation = m_cameraBuffer->GetGPUVirtualAddress();
    cbvDesc.SizeInBytes = m_cameraBufferSize;

    D3D12_CPU_DESCRIPTOR_HANDLE srvHandle = m_constHeap->GetCPUDescriptorHandleForHeapStart();

    device->CreateConstantBufferView(&cbvDesc, srvHandle);
}

void CubeDemo::UpdateCameraBuffer()
{
    std::vector<XMMATRIX> matrices{ 4 };
    XMVECTOR Eye = XMVectorSet(1.5f, 1.5f, 1.5f, 0.0f);
    XMVECTOR At = XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f);
    XMVECTOR Up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

    matrices[0] = _camera->ViewMatrix();

    matrices[1] = _camera->ProjectionMatrix();

    DirectX::XMVECTOR det;
    matrices[2] = DirectX::XMMatrixInverse(&det, matrices[0]);
    matrices[3] = DirectX::XMMatrixInverse(&det, matrices[1]);

    uint8_t* pData;
    ThrowIfFailed(m_cameraBuffer->Map(0, nullptr, reinterpret_cast<void**>(&pData)));
    memcpy(pData, matrices.data(), m_cameraBufferSize);
    m_cameraBuffer->Unmap(0, nullptr);

}

void CubeDemo::CreatePointLightBuffer()
{
    auto device = Application::Get().GetDevice().Get();

    uint32_t pointLightCount = 1;

    m_pointLightBufferSize = pointLightCount * sizeof(XMFLOAT3);

    if (m_pointLightBufferSize < 256)
    {
        m_pointLightBufferSize = 256;
    }

    m_pointLightBuffer = nv_helpers_dx12::CreateBuffer(device, m_pointLightBufferSize, D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATE_GENERIC_READ, nv_helpers_dx12::kUploadHeapProps);
    m_pointLightHeap = nv_helpers_dx12::CreateDescriptorHeap(device, 1, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, true);

    m_pointLightBuffer->SetName(L"Point Light Buffer");

    D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc{};

    cbvDesc.BufferLocation = m_pointLightBuffer->GetGPUVirtualAddress();
    cbvDesc.SizeInBytes = m_pointLightBufferSize;

    D3D12_CPU_DESCRIPTOR_HANDLE srvHandle = m_pointLightHeap->GetCPUDescriptorHandleForHeapStart();

    device->CreateConstantBufferView(&cbvDesc, srvHandle);
}

void CubeDemo::UpdatePointLightBuffer()
{
    XMFLOAT3& pointLightPosition = _pointLight->_position;
    XMFLOAT3& pointLightColor = _pointLight->_color;
    float& pointLightIntensity = _pointLight->_intensity;

    std::vector<float> proxyFloatVector{ pointLightPosition.x, pointLightPosition.y, pointLightPosition.z, 0.0f, pointLightColor.x, pointLightColor.y, pointLightColor.z, pointLightIntensity, pointLightIntensity};

    for (std::size_t index{ 0 }; index < 55; ++index)
    {
        proxyFloatVector.push_back(0.0f);
    }

    uint8_t* pData;
    ThrowIfFailed(m_pointLightBuffer->Map(0, nullptr, reinterpret_cast<void**>(&pData)));
    memcpy(pData, proxyFloatVector.data(), m_pointLightBufferSize);
    m_pointLightBuffer->Unmap(0, nullptr);
}

void CubeDemo::CreateRandomFrameNumberBuffer()
{
    auto device = Application::Get().GetDevice().Get();

    m_randomFrameNumberBuffer = nv_helpers_dx12::CreateBuffer(device, 256, D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATE_GENERIC_READ, nv_helpers_dx12::kUploadHeapProps);
    m_randomFrameNumberBufferHeap = nv_helpers_dx12::CreateDescriptorHeap(device, 1, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, true);

    D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc{};

    cbvDesc.BufferLocation = m_randomFrameNumberBuffer->GetGPUVirtualAddress();
    cbvDesc.SizeInBytes = 256;

    D3D12_CPU_DESCRIPTOR_HANDLE srvHandle = m_randomFrameNumberBufferHeap->GetCPUDescriptorHandleForHeapStart();

    device->CreateConstantBufferView(&cbvDesc, srvHandle);
}

void CubeDemo::UpdateRandomFrameNumberBuffer()
{
    UINT* framePtr = &FrameNumber;
    std::byte* bytePtr = reinterpret_cast<std::byte*>(framePtr);
    std::byte* boolBytePtr = reinterpret_cast<std::byte*>(&_isAccumulatingFrames);
    std::byte* accBytesPtr = reinterpret_cast<std::byte*>(&_frameAccumulationStarted);
    std::byte* aaBoolBytesPtr = reinterpret_cast<std::byte*>(&_isSuperSampling);
    std::vector<std::byte> proxyIntVector{ bytePtr[0], bytePtr[1], bytePtr[2], bytePtr[3], accBytesPtr[0], accBytesPtr[1], accBytesPtr[2], accBytesPtr[3], boolBytePtr[0], std::byte{}, std::byte{}, std::byte{}, aaBoolBytesPtr[0] };
    sizeof(bool);
    for (std::size_t index{ 0 }; index < 235; ++index)
    {
        proxyIntVector.push_back(std::byte{});
    }

    uint8_t* pData;
    ThrowIfFailed(m_randomFrameNumberBuffer->Map(0, nullptr, reinterpret_cast<void**>(&pData)));
    memcpy(pData, proxyIntVector.data(), 256);
    m_randomFrameNumberBuffer->Unmap(0, nullptr);
}


CubeDemo::CubeDemo(const std::wstring& name, int width, int height, bool vSync) :
    super{ name, width, height, vSync },
    _scissorRect{ CD3DX12_RECT{0, 0, LONG_MAX, LONG_MAX } },
    _viewport{ CD3DX12_VIEWPORT(0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height)) },
    _fieldOfView{ 45.0f },
    _contentLoaded{ false }
{

}

bool CubeDemo::LoadContent()
{    

    g_pd3dDevice = Application::Get().GetDevice().Get();

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;

    D3D12_DESCRIPTOR_HEAP_DESC desc = {};
    desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    desc.NumDescriptors = 1;
    desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    if (g_pd3dDevice->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&g_pd3dSrvDescHeap)) != S_OK)
        return false;

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();

    // Setup Platform/Renderer backends
    ImGui_ImplWin32_Init(_window->GetWindowHandle());
    ImGui_ImplDX12_Init(g_pd3dDevice, NUM_FRAMES_IN_FLIGHT,
        DXGI_FORMAT_R8G8B8A8_UNORM, g_pd3dSrvDescHeap,
        g_pd3dSrvDescHeap->GetCPUDescriptorHandleForHeapStart(),
        g_pd3dSrvDescHeap->GetGPUDescriptorHandleForHeapStart());

    // Loading model here
    Library::ContentTypeReaderManager::Initialize();

    //Check Raytracing feature support.
    D3D12_FEATURE_DATA_D3D12_OPTIONS5 options5{};
    ThrowIfFailedI(
        Application::Get().GetDevice()->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5, &options5, sizeof(options5)));
    if (options5.RaytracingTier < D3D12_RAYTRACING_TIER_1_0)
    {
        throw std::runtime_error("Raytracing not supported on device");
    }

    _camera = std::make_shared<FirstPersonCamera>();
    _camera->Initialize();
    _camera->SetPosition(-125, 100, -500);

    _pointLight = std::make_shared<PointLight>();
    _pointLight->SetPosition(88.4f, 111.4f, -556.5f);

    auto device = Application::Get().GetDevice();
    auto commandQueue = Application::Get().GetCommandQueue(D3D12_COMMAND_LIST_TYPE_DIRECT);
    auto commandList = commandQueue->GetCommandList();

    _gameObjects = {
        { std::make_shared<GameObject>(L"GBFinalPlaced.model", L"Content/GB_4K.DDS", L"Content/GiantBaby_Normal.dds", _contentManager, 0, L"GB Body")},
        { std::make_shared<GameObject>(L"GBFinalPlaced.model", L"Content/GB_4K.DDS", L"Content/GiantBaby_Normal.dds", _contentManager, 1, L"GB Body")},
        { std::make_shared<GameObject>(L"GBFinalPlaced.model", L"Content/GB_4K.DDS", L"Content/GiantBaby_Normal.dds", _contentManager, 2, L"GB Body")},
        { std::make_shared<GameObject>(L"GBFinalPlaced.model", L"Content/NeptuneColorMap.dds", L"Content/GiantBaby_Normal.dds", _contentManager, 3, L"GB Eyes")},
        { std::make_shared<GameObject>(L"BusterSwordFinalPlaced.model", L"Content/BusterSword_ColorMap.dds", L"Content/BusterSword_Normal.dds", _contentManager, 0, L"Buster Sword")},
        { std::make_shared<GameObject>(L"BasaltColumnsFinalPlaced.model", L"Content/BasaltColumnColorMap.DDS", L"Content/Basalt_Normal.dds", _contentManager, 0, L"Basalt Columns")},
        { std::make_shared<GameObject>(L"SandFinalPlaced.model", L"Content/SandColorMap.DDS", L"Content/Sand_Normal.DDS", _contentManager, 0, L"Sand")},
    };

    //for (auto& object : _gameObjects)
    //{
    //    object->GetMaterialProperties().isLight = false;
    //}

    _lightProxy = new GameObject{L"Sphere.obj.bin",L"Content/NeptuneColorMap.dds", L"Content/GiantBaby_Normal.dds", _contentManager, 0, L"Proxy"};
    _lightProxy->GetMaterialProperties().isLight = true;

    _gameObjects.emplace_back(_lightProxy);

    _lightProxy->UpdatePosition(0.0f, 0.0f, -1500.0f);
    //_gameObjects.reserve(296);

    //for (std::size_t i{ 0 }; i < 296; ++i)
    //{
    //    _gameObjects.emplace_back(std::make_shared<GameObject>(L"RescaledHead.model", L"Content/HeadLambert.dds", L"Content/HeadNormal.dds", _contentManager, i, L"GB Body"));
    //}

    for (auto& object : _gameObjects)
    {
        auto initCommandList = commandQueue->GetCommandList();
        object->Initialize(commandList, _camera);
        auto initFenceValue = commandQueue->ExecuteCommandList(initCommandList);
        commandQueue->WaitForFenceValue(initFenceValue);
    }


    D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc{};
    dsvHeapDesc.NumDescriptors = 1;
    dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
    dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    ThrowIfFailedI(device->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&_DSVHeap)));

    ComPtr<ID3DBlob> vertexShaderBlob;
    ThrowIfFailedI(D3DReadFileToBlob(L"VertexShader.cso", &vertexShaderBlob));

    ComPtr<ID3DBlob> pixelShaderBlob;
    ThrowIfFailedI(D3DReadFileToBlob(L"PixelShader.cso", &pixelShaderBlob));

    D3D12_INPUT_ELEMENT_DESC inputLayout[]
    {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        {"COLOR", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
    };

    D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData{};
    featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;
    if (FAILED(device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &featureData, sizeof(featureData))))
    {
        featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
    }

    D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags
    {
        D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS
    };

    D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
    srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    srvHeapDesc.NumDescriptors = 1; // Number of textures to bind
    srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    ThrowIfFailedI(device->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(_texDescriptorHeap.GetAddressOf())));

    CD3DX12_CPU_DESCRIPTOR_HANDLE hDescriptor(_texDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

    D3D12_DESCRIPTOR_HEAP_DESC samplerHeapDesc = {};
    samplerHeapDesc.NumDescriptors = 1;
    samplerHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
    samplerHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    ID3D12DescriptorHeap* samplerDescriptorHeap;
    device->CreateDescriptorHeap(&samplerHeapDesc, IID_PPV_ARGS(&samplerDescriptorHeap));

    D3D12_SAMPLER_DESC samplerDesc = {};
    samplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
    samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    samplerDesc.MipLODBias = 0;
    samplerDesc.MaxAnisotropy = 1;
    samplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
    samplerDesc.BorderColor[0] = 1.0f;
    samplerDesc.BorderColor[1] = 1.0f;
    samplerDesc.BorderColor[2] = 1.0f;
    samplerDesc.BorderColor[3] = 1.0f;
    samplerDesc.MinLOD = 0;
    samplerDesc.MaxLOD = D3D12_FLOAT32_MAX;

    device->CreateSampler(&samplerDesc, samplerDescriptorHeap->GetCPUDescriptorHandleForHeapStart());


    CD3DX12_DESCRIPTOR_RANGE1 descriptorRangeSampler;
    descriptorRangeSampler.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, 1, 0);

    CD3DX12_ROOT_DESCRIPTOR_TABLE1 descriptorTableSampler;
    descriptorTableSampler.Init(1, &descriptorRangeSampler);

    CD3DX12_DESCRIPTOR_RANGE1 ranges[1];
    ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC);

    CD3DX12_ROOT_PARAMETER1 rootParameters[3];
    rootParameters[0].InitAsConstants(sizeof(XMMATRIX) / 4, 0, 0, D3D12_SHADER_VISIBILITY_VERTEX);
    rootParameters[1].InitAsDescriptorTable(1, &ranges[0], D3D12_SHADER_VISIBILITY_PIXEL);//InitAsShaderResourceView(0, 0, D3D12_ROOT_DESCRIPTOR_FLAG_NONE, D3D12_SHADER_VISIBILITY_PIXEL);
    rootParameters[2].InitAsDescriptorTable(1, &descriptorRangeSampler, D3D12_SHADER_VISIBILITY_PIXEL);

    CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDescription;
    rootSignatureDescription.Init_1_1(_countof(rootParameters), rootParameters, 0, nullptr, rootSignatureFlags);

    ComPtr<ID3DBlob> rootSignatureBlob;
    ComPtr<ID3DBlob> errorBlob;
    ThrowIfFailedI(D3DX12SerializeVersionedRootSignature(&rootSignatureDescription, featureData.HighestVersion, &rootSignatureBlob, &errorBlob));
    ThrowIfFailedI(device->CreateRootSignature(0, rootSignatureBlob->GetBufferPointer(), rootSignatureBlob->GetBufferSize(), IID_PPV_ARGS(&_rootSignature)));

    CD3DX12_DESCRIPTOR_RANGE range;
    range.Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0);

    CD3DX12_DESCRIPTOR_RANGE rtRange;
    rtRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 3);

    CD3DX12_ROOT_PARAMETER rtParameters[2];
    rtParameters[0].InitAsDescriptorTable(1, &range, D3D12_SHADER_VISIBILITY_ALL);
    rtParameters[1].InitAsDescriptorTable(1, &rtRange, D3D12_SHADER_VISIBILITY_ALL);


    CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
    rootSignatureDesc.Init(_countof(rtParameters), rtParameters, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

    ComPtr<ID3DBlob> cameraRootSignatureBlob;
    ComPtr<ID3DBlob> cameraRootSignatureErrorBlob;

    ThrowIfFailed(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &cameraRootSignatureBlob, &cameraRootSignatureErrorBlob));
    ThrowIfFailed(device->CreateRootSignature(0, cameraRootSignatureBlob->GetBufferPointer(), cameraRootSignatureBlob->GetBufferSize(), IID_PPV_ARGS(&_rtRootSignature)));

    D3D12_RT_FORMAT_ARRAY rtvFormats{};
    rtvFormats.NumRenderTargets = 1;
    rtvFormats.RTFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;

    pipelineStateStream.rootSignature = _rootSignature.Get();
    pipelineStateStream.inputLayout = { inputLayout, _countof(inputLayout) };
    pipelineStateStream.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    pipelineStateStream.vs = CD3DX12_SHADER_BYTECODE(vertexShaderBlob.Get());
    pipelineStateStream.ps = CD3DX12_SHADER_BYTECODE(pixelShaderBlob.Get());
    pipelineStateStream.DSVFormat = DXGI_FORMAT_D32_FLOAT;
    pipelineStateStream.RTVFormats = rtvFormats;

    D3D12_PIPELINE_STATE_STREAM_DESC pipelineStateStreamDesc{ sizeof(PipelineStateStream), &pipelineStateStream };

    ThrowIfFailedI(device->CreatePipelineState(&pipelineStateStreamDesc, IID_PPV_ARGS(&_pipelineState)));

    auto fenceValue = commandQueue->ExecuteCommandList(commandList);
    commandQueue->WaitForFenceValue(fenceValue);

    //DXR Init Acceleration Structure Creation

    auto commandQueue2 = Application::Get().GetCommandQueue(D3D12_COMMAND_LIST_TYPE_DIRECT);

    commandList = commandQueue2->GetCommandList();

    auto fwd = Library::Vector3Helper::Forward;
    auto up = Library::Vector3Helper::Up;
    auto right = Library::Vector3Helper::Right;
    auto pos = XMFLOAT3{ -15.0f, -15.0f, -15.0f };

    XMMATRIX worldMatrix = XMMatrixIdentity();
    Library::MatrixHelper::SetForward(worldMatrix, fwd);
    Library::MatrixHelper::SetUp(worldMatrix, up);
    Library::MatrixHelper::SetRight(worldMatrix, right);
    Library::MatrixHelper::SetTranslation(worldMatrix, pos);

    std::vector<AccelerationStructureBuffers> bottomLevelBuffers;

    for (auto& object : _gameObjects)
    {
        bottomLevelBuffers.emplace_back(CreateBottomLevelAS({ { object->VertexBuffer().Get(), object->VertCount() } }, { {object->IndexBuffer().Get(), object->IndexCount()} }, commandList.Get()));
        m_bottomLevelASInstances.push_back({ bottomLevelBuffers[bottomLevelBuffers.size() - 1].result, object->WorldMatrix()});
    }

    CreateTopLevelAS(m_bottomLevelASInstances, commandList.Get());

    fenceValue = commandQueue2->ExecuteCommandList(commandList);
    commandQueue2->WaitForFenceValue(fenceValue);

    CreateRayTracingPipeline();
    CreateRaytracingOutputBuffer();
    CreateAccumulatedOutputBuffer();
    CreateCameraBuffer();
    CreatePointLightBuffer();
    CreateRandomFrameNumberBuffer();
    CreateShaderResourceHeap();
    CreateShaderBindingTable();

    _contentLoaded = true;
    ResizeDepthBuffer(1920, 1080);

    return true;
}

bool CubeDemo::UnloadContent()
{
    ImGui_ImplDX12_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    return false;
}

void CubeDemo::OnUpdate(UpdateEventArgs& args)
{

    static uint64_t frameCount = 0;
    static double totalTime = 0.0;

    ++FrameNumber;

    super::OnUpdate(args);
    _camera->CheckForInput(cameraKeyArgument, cameraMouseArgument, args);
    _camera->Update(args);

    _pointLight->CheckForInput(pointLightArgument, cameraMouseArgument, args);
    _pointLight->Update(args);
    auto& lightPos = _pointLight->_position;
    _lightProxy->UpdatePosition(lightPos.x, lightPos.y, lightPos.z);

    UpdateCameraBuffer();

    if (!_isRaster)
    {
        UpdatePointLightBuffer();
    }

    UpdateRandomFrameNumberBuffer();

    totalTime += args.ElapsedTime;
    frameCount++;

    if (totalTime > 1.0)
    {
        double fps = frameCount / totalTime;

        char buffer[512];
        sprintf_s(buffer, "FPS: %f\n", fps);
        OutputDebugStringA(buffer);

        frameCount = 0;
        totalTime = 0.0;
    }

    std::vector<XMMATRIX> updatedMatrices;

    for (auto& object : _gameObjects)
    {
        object->UpdateObject();
        updatedMatrices.push_back(object->WorldMatrix());
    }

    std::size_t index{ 0 };
    for (auto& object : m_bottomLevelASInstances)
    {
        object.transformation = updatedMatrices[index];
        ++index;
    }

    auto commandQueue = Application::Get().GetCommandQueue(D3D12_COMMAND_LIST_TYPE_DIRECT);
    auto commandList = commandQueue->GetCommandList();

    CreateTopLevelAS(m_bottomLevelASInstances, commandList.Get(), true);

    auto initFenceValue = commandQueue->ExecuteCommandList(commandList);
    commandQueue->WaitForFenceValue(initFenceValue);
}

void CubeDemo::OnRender(RenderEventArgs& args)
{
    super::OnRender(args);

    ImGui_ImplDX12_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    bool show_demo_window = true;
    bool show_another_window = false;
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    XMFLOAT3& pointLightPosition = _pointLight->_position;

    {
        static float f = 0.0f;
        static int counter = 0;
        static bool raytracing = true;
        static bool temporalAccumulation = false;
        static bool antiAliasing = false;

        ImGui::Begin("DXR Scene");

        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
        //ImGui::Checkbox("Raytracing", &raytracing);
        //_isRaster = !raytracing;

        ImGui::Checkbox("Temporal Accumulation", &temporalAccumulation);

        if (temporalAccumulation != _isAccumulatingFrames)
        {
            _frameAccumulationStarted = FrameNumber;
        }
        _isAccumulatingFrames = temporalAccumulation;

        ImGui::Checkbox("4x Super Sampling", &antiAliasing);
        _isSuperSampling = antiAliasing;

        ImGui::Text("Move Camera Forward/Backward : WASD EQ");
        ImGui::Text("Camera Look : Click and Drag");
        ImGui::Text("Camera Reset Orientation : Tab");
        ImGui::Text("Light Position : X: %.1f , Y: %.1f , Z: %.1f", pointLightPosition.x, pointLightPosition.y, pointLightPosition.z);
        ImGui::DragFloat("Light Scale", _lightProxy->Scale(), 0.5f, 1.0f, 70.0f, "%.3f");
        ImGui::DragFloat("Light Intensity", &_pointLight->_intensity, 0.1f, 1.0f, 50.0f, "%.3f");
        ImGui::ColorEdit4("Light Color", (float*)&_pointLight->_color);

        ImGui::End();
    }

    auto commandQueue = Application::Get().GetCommandQueue(D3D12_COMMAND_LIST_TYPE_DIRECT);
    auto commandList = commandQueue->GetCommandList();

    UINT currentBackBufferIndex = _window->GetCurrentBackBufferIndex();
    auto backBuffer = _window->GetCurrentBackBuffer();
    auto rtv = _window->GetCurrentRenderTargetView();
    auto dsv = _DSVHeap->GetCPUDescriptorHandleForHeapStart();

    {
        TransitionResource(commandList, backBuffer,
            D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
    }

    commandList->SetGraphicsRootSignature(_rootSignature.Get());
    commandList->RSSetViewports(1, &_viewport);
    commandList->RSSetScissorRects(1, &_scissorRect);
    commandList->OMSetRenderTargets(1, &rtv, false, &dsv);

    ImGui::Render();

    if (_isRaster)
    {
        FLOAT clearColor[]{ 0.0f, 0.0f, 0.0f, 1.0f };
        ClearRTV(commandList, rtv, clearColor);
        ClearDepth(commandList, dsv);

        commandList->SetPipelineState(_pipelineState.Get());
        for (auto& object : _gameObjects)
        {
            object->DrawObject(commandList);
        }
    }

    else
    {
        commandList->SetGraphicsRootSignature(_rtRootSignature.Get());

        std::vector<ID3D12DescriptorHeap*> heaps{ m_srvUavHeap.Get() };
        commandList->SetDescriptorHeaps(static_cast<UINT>(heaps.size()), heaps.data());
        CD3DX12_RESOURCE_BARRIER transition = CD3DX12_RESOURCE_BARRIER::Transition(m_outputResource.Get(), D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
        commandList->ResourceBarrier(1, &transition);

        D3D12_DISPATCH_RAYS_DESC desc = {};
        uint32_t rayGenerationSectionSizeInBytes = m_sbtHelper.GetRayGenSectionSize();
        desc.RayGenerationShaderRecord.StartAddress = m_sbtStorage->GetGPUVirtualAddress();
        desc.RayGenerationShaderRecord.SizeInBytes = rayGenerationSectionSizeInBytes;

        uint32_t missSectionSizeInBytes = m_sbtHelper.GetMissSectionSize();
        desc.MissShaderTable.StartAddress = m_sbtStorage->GetGPUVirtualAddress() + rayGenerationSectionSizeInBytes;
        desc.MissShaderTable.SizeInBytes = missSectionSizeInBytes;
        desc.MissShaderTable.StrideInBytes = m_sbtHelper.GetMissEntrySize();

        uint32_t hitGroupsSectionSize = m_sbtHelper.GetHitGroupSectionSize();
        desc.HitGroupTable.StartAddress = m_sbtStorage->GetGPUVirtualAddress() + rayGenerationSectionSizeInBytes + missSectionSizeInBytes;
        desc.HitGroupTable.SizeInBytes = hitGroupsSectionSize;
        desc.HitGroupTable.StrideInBytes = m_sbtHelper.GetHitGroupEntrySize();

        desc.Width = _window->GetClientWidth();
        desc.Height = _window->GetClientHeight();
        desc.Depth = 1;

        commandList->SetPipelineState1(m_rtStateObject.Get());
        commandList->DispatchRays(&desc);

        transition = CD3DX12_RESOURCE_BARRIER::Transition(m_outputResource.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_SOURCE);
        commandList->ResourceBarrier(1, &transition);
        transition = CD3DX12_RESOURCE_BARRIER::Transition(_window->GetCurrentBackBuffer().Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_COPY_DEST);
        commandList->ResourceBarrier(1, &transition);
        commandList->CopyResource(_window->GetCurrentBackBuffer().Get(), m_outputResource.Get());
        transition = CD3DX12_RESOURCE_BARRIER::Transition(_window->GetCurrentBackBuffer().Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_RENDER_TARGET);
        commandList->ResourceBarrier(1, &transition);
    }

    commandList->SetDescriptorHeaps(1, &g_pd3dSrvDescHeap);
    ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), commandList.Get());

    {
        TransitionResource(commandList, backBuffer, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);

        _fenceValues[currentBackBufferIndex] = commandQueue->ExecuteCommandList(commandList);

        currentBackBufferIndex = _window->Present();

        commandQueue->WaitForFenceValue(_fenceValues[currentBackBufferIndex]);
    }

}

void CubeDemo::OnKeyPressed(KeyEventArgs& args)
{
    super::OnKeyPressed(args);
    auto it = std::find(cameraKeyArgument.begin(), cameraKeyArgument.end(), args.Key);
    auto it2 = std::find(pointLightArgument.begin(), pointLightArgument.end(), args.Key);
    switch (args.Key)
    {
    case KeyCode::Escape:
        Application::Get().Quit(0);
        break;
    case KeyCode::Enter:
        if (args.Alt)
        {
    case KeyCode::F11:
        _window->ToggleFullscreen();
        break;
        }
        break;
    case KeyCode::V:
        _window->ToggleVSync();
    case KeyCode::W:
    case KeyCode::A:
    case KeyCode::S:
    case KeyCode::D:
    case KeyCode::Q:
    case KeyCode::E:
        if (it == std::end(cameraKeyArgument))
        {
            cameraKeyArgument.emplace_back(args.Key);
        }
        break;
    case KeyCode::NumPad8:
    case KeyCode::NumPad4:
    case KeyCode::NumPad5:
    case KeyCode::NumPad6:
    case KeyCode::NumPad7:
    case KeyCode::NumPad9:
        if (it2 == std::end(pointLightArgument))
        {
            pointLightArgument.emplace_back(args.Key);
        }
        break;
    //case KeyCode::Tab:
    //    _camera->ResetOrientation();
    //    break;
    case KeyCode::T:
        _isAccumulatingFrames = !_isAccumulatingFrames;
        _frameAccumulationStarted = FrameNumber;
        break;
    default:
        break;
    }
}

void CubeDemo::OnKeyReleased(KeyEventArgs& args)
{
    auto it = std::find(cameraKeyArgument.begin(), cameraKeyArgument.end(), args.Key);
    if (it != cameraKeyArgument.end())
    {
        cameraKeyArgument.erase(it);
    }

    auto it2 = std::find(pointLightArgument.begin(), pointLightArgument.end(), args.Key);
    if (it2 != pointLightArgument.end())
    {
        pointLightArgument.erase(it2);
    }
}

void CubeDemo::OnMouseWheel(MouseWheelEventArgs& args)
{


}

void CubeDemo::UpdateBufferResource(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList4> commandList, ID3D12Resource** destinationResource, ID3D12Resource** intermediateResource, std::size_t numElements, std::size_t elementSize, const void* bufferData, D3D12_RESOURCE_FLAGS flags)
{
    auto device = Application::Get().GetDevice();
    std::size_t bufferSize = numElements * elementSize;
    auto heapProperty1 = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
    auto heapProperty2 = CD3DX12_RESOURCE_DESC::Buffer(bufferSize, flags);
    ThrowIfFailedI(device->CreateCommittedResource(
        &heapProperty1,
        D3D12_HEAP_FLAG_NONE,
        &heapProperty2,
        D3D12_RESOURCE_STATE_COPY_DEST,
        nullptr,
        IID_PPV_ARGS(destinationResource)));

    auto heapProperty3 = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
    auto heapProperty4 = CD3DX12_RESOURCE_DESC::Buffer(bufferSize);
    if (bufferData)
    {
        ThrowIfFailedI(device->CreateCommittedResource(
            &heapProperty3,
            D3D12_HEAP_FLAG_NONE,
            &heapProperty4,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(intermediateResource)));
        D3D12_SUBRESOURCE_DATA subresourceData{};
        subresourceData.pData = bufferData;
        subresourceData.RowPitch = bufferSize;
        subresourceData.SlicePitch = subresourceData.RowPitch;

        UpdateSubresources(commandList.Get(),*destinationResource, *intermediateResource, 0, 0, 1, &subresourceData);
    }
}

CubeDemo::AccelerationStructureBuffers CubeDemo::CreateBottomLevelAS(std::vector<std::pair<ComPtr<ID3D12Resource>, uint32_t>> vVertexBuffers, std::vector<std::pair<ComPtr<ID3D12Resource>, uint32_t>> vIndexBuffers, ID3D12GraphicsCommandList4* commandList)
{
    nv_helpers_dx12::BottomLevelASGenerator bottomLevelAS;

    for (std::size_t i{0}; i < vVertexBuffers.size(); ++i)
    {
        if (i < vIndexBuffers.size() && vIndexBuffers[i].second > 0)
        {
            bottomLevelAS.AddVertexBuffer(vVertexBuffers[i].first.Get(), 0, vVertexBuffers[i].second / 2, sizeof(VertexPosColor), vIndexBuffers[i].first.Get(), 0, vIndexBuffers[i].second, nullptr, 0, true);
        }
        else
        {
            bottomLevelAS.AddVertexBuffer(vVertexBuffers[i].first.Get(), 0, vVertexBuffers[i].second, sizeof(VertexPosColor), 0, 0);
        }
    }


    UINT64 scratchSize;
    UINT64 resultSize;

    auto device = Application::Get().GetDevice().Get();

    bottomLevelAS.ComputeASBufferSizes(device, false, &scratchSize, &resultSize);

    AccelerationStructureBuffers buffers;

    buffers.scratch = nv_helpers_dx12::CreateBuffer(device, scratchSize, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, nv_helpers_dx12::kDefaultHeapProps);
    buffers.result = nv_helpers_dx12::CreateBuffer(device, resultSize, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE, nv_helpers_dx12::kDefaultHeapProps);

    bottomLevelAS.Generate(commandList, buffers.scratch.Get(), buffers.result.Get(), false, nullptr);

    return buffers;
}

void CubeDemo::CreateTopLevelAS(const std::vector<BottomLevelASInstance>& instances, ID3D12GraphicsCommandList4* commandList, bool updateOnly)
{
    if (!updateOnly)
    {
        for (std::size_t i{0}; i < instances.size(); i++)
        {
            m_topLevelASGenerator.AddInstance(instances[i].bottomLevelAS.Get(), instances[i].transformation, static_cast<UINT>(i), static_cast<UINT>(0));
        }

        auto device = Application::Get().GetDevice().Get();

        UINT64 scratchSize;
        UINT64 resultSize;
        UINT64 descsSize;

        m_topLevelASGenerator.ComputeASBufferSizes(device, false, &scratchSize, &resultSize, &descsSize);

        m_topLevelASBuffers.scratch = nv_helpers_dx12::CreateBuffer(device, scratchSize, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, nv_helpers_dx12::kDefaultHeapProps);
        m_topLevelASBuffers.result = nv_helpers_dx12::CreateBuffer(device, resultSize, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE, nv_helpers_dx12::kDefaultHeapProps);
        m_topLevelASBuffers.descriptors = nv_helpers_dx12::CreateBuffer(device, descsSize, D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATE_GENERIC_READ, nv_helpers_dx12::kUploadHeapProps);
    }

    m_topLevelASGenerator.Generate(commandList, m_topLevelASBuffers.scratch.Get(), m_topLevelASBuffers.result.Get(), m_topLevelASBuffers.descriptors.Get());
}

void CubeDemo::CreateAccelerationStructures(ID3D12GraphicsCommandList4* commandList)
{
}

void CubeDemo::ResizeDepthBuffer(int width, int height)
{
    if (_contentLoaded)
    {
        Application::Get().Flush();

        width = std::max(1, width);
        height = std::max(1, height);

        auto device = Application::Get().GetDevice();

        D3D12_CLEAR_VALUE optimizedClearValue{};
        optimizedClearValue.Format = DXGI_FORMAT_D32_FLOAT;
        optimizedClearValue.DepthStencil = { 1.0f, 0 };

        auto heapProperties1 = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
        auto resourceDesc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_D32_FLOAT, width, height,
            1, 0, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);

        ThrowIfFailedI(device->CreateCommittedResource(
            &heapProperties1,
            D3D12_HEAP_FLAG_NONE,
            &resourceDesc,
            D3D12_RESOURCE_STATE_DEPTH_WRITE,
            &optimizedClearValue,
            IID_PPV_ARGS(&_depthBuffer)
        ));

        D3D12_DEPTH_STENCIL_VIEW_DESC dsv{};
        dsv.Format = DXGI_FORMAT_D32_FLOAT;
        dsv.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
        dsv.Texture2D.MipSlice = 0;
        dsv.Flags = D3D12_DSV_FLAG_NONE;

        device->CreateDepthStencilView(_depthBuffer.Get(), &dsv, _DSVHeap->GetCPUDescriptorHandleForHeapStart());
    }
}

void CubeDemo::OnResize(ResizeEventArgs& args)
{
    if (args.Width != 1920 || args.Height != 1080)
    {
        super::OnResize(args);

        _viewport = CD3DX12_VIEWPORT(0.0f, 0.0f, static_cast<float>(args.Width), static_cast<float>(args.Width));

        ResizeDepthBuffer(args.Width, args.Height);
    }
}

void CubeDemo::OnMouseMoved(MouseMotionEventArgs& args)
{
    if (_camera->IsAcceptingMouseMovementInputs())
    {
        cameraMouseArgument.first = args.X - startingMousePos.first;
        cameraMouseArgument.second = args.Y - startingMousePos.second;

        char buffer[512];
        sprintf_s(buffer, "XDIFF: %d\n", args.X - startingMousePos.first);
        OutputDebugStringA(buffer);

        char buffer2[512];
        sprintf_s(buffer2, "YDIFF: %d\n", args.Y - startingMousePos.second);
        OutputDebugStringA(buffer2);
        startingMousePos = { args.X, args.Y };
    }
}

void CubeDemo::OnMouseButtonPressed(MouseButtonEventArgs& args)
{
    if (args.RightButton)
    {
        SetCapture(_window->GetWindowHandle());
        _camera->SetIsAcceptingMouseMovementInputs(true);
        startingMousePos.first = args.X;
        startingMousePos.second = args.Y;
    }
}

void CubeDemo::OnMouseButtonReleased(MouseButtonEventArgs& args)
{
    if (!args.RightButton)
    {
        _camera->SetIsAcceptingMouseMovementInputs(false);
        ReleaseCapture();
    }
}

void CubeDemo::TransitionResource(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList4> commandList, Microsoft::WRL::ComPtr<ID3D12Resource> resource, D3D12_RESOURCE_STATES beforeState, D3D12_RESOURCE_STATES afterState)
{
    CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
        resource.Get(),
        beforeState, afterState);

    commandList->ResourceBarrier(1, &barrier);
}

void CubeDemo::ClearRTV(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList4> commandList, D3D12_CPU_DESCRIPTOR_HANDLE rtv, float* clearColor)
{
    commandList->ClearRenderTargetView(rtv, clearColor, 0, nullptr);
}

void CubeDemo::ClearDepth(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList4> commandList, D3D12_CPU_DESCRIPTOR_HANDLE dsv, float depth)
{
    commandList->ClearDepthStencilView(dsv, D3D12_CLEAR_FLAG_DEPTH, depth, 0, 0, nullptr);
}
