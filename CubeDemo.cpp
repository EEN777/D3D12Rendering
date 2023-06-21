#include "pch.h"
#include "CubeDemo.h"

#include "Application.h"
#include "CommandQueue.h"
#include "Helpers.h"
#include "Window.h"
#include "DXRHelper.h"
#include "nv_helpers_dx12/BottomLevelASGenerator.h"
#include "nv_helpers_dx12/RaytracingPipelineGenerator.h"
#include "nv_helpers_dx12/RootSignatureGenerator.h"

#include <d3dx12.h>
#include <d3dcompiler.h>

#include <algorithm>

using namespace DirectX;

std::vector<KeyCode::Key> cameraKeyArgument{};
std::pair<int, int> cameraMouseArgument{INT_MIN, INT_MIN};
std::pair<int, int> startingMousePos{};

struct VertexPosColor
{
	XMFLOAT3 Position;
	XMFLOAT3 Color;
};


static VertexPosColor vertices[8]
{
    { XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT3(0.0f, 0.0f, 0.0f) },
    { XMFLOAT3(-1.0f,  1.0f, -1.0f), XMFLOAT3(0.0f, 1.0f, 0.0f) }, 
    { XMFLOAT3(1.0f,  1.0f, -1.0f), XMFLOAT3(1.0f, 1.0f, 0.0f) }, 
    { XMFLOAT3(1.0f, -1.0f, -1.0f), XMFLOAT3(1.0f, 0.0f, 0.0f) }, 
    { XMFLOAT3(-1.0f, -1.0f,  1.0f), XMFLOAT3(0.0f, 0.0f, 1.0f) }, 
    { XMFLOAT3(-1.0f,  1.0f,  1.0f), XMFLOAT3(0.0f, 1.0f, 1.0f) },
    { XMFLOAT3(1.0f,  1.0f,  1.0f), XMFLOAT3(1.0f, 1.0f, 1.0f) },
    { XMFLOAT3(1.0f, -1.0f,  1.0f), XMFLOAT3(1.0f, 0.0f, 1.0f) }
};

static uint32_t indices[36]
{
    0,1,2,0,2,3,
    4,6,5,4,7,6,
    4,5,1,4,1,0,
    3,2,6,3,6,7,
    1,5,6,1,6,2,
    4,0,3,4,3,7
};

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

ComPtr<ID3D12RootSignature> CubeDemo::CreateRayGenSignature()
{
    nv_helpers_dx12::RootSignatureGenerator rsc;
    rsc.AddHeapRangesParameter({ {0, 1, 0, D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 0}, {0, 1, 0, D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1}, {0, 1, 0, D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 2} });

    return rsc.Generate(Application::Get().GetDevice().Get(), true);
}

ComPtr<ID3D12RootSignature> CubeDemo::CreateHitSignature()
{
    nv_helpers_dx12::RootSignatureGenerator rsc;
    rsc.AddRootParameter(D3D12_ROOT_PARAMETER_TYPE_SRV, 0);
    rsc.AddRootParameter(D3D12_ROOT_PARAMETER_TYPE_SRV, 1);

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

    pipeline.AddLibrary(m_rayGenLibrary.Get(), { L"RayGen" });
    pipeline.AddLibrary(m_hitLibrary.Get(), { L"ClosestHit" });
    pipeline.AddLibrary(m_missLibrary.Get(), { L"Miss" });

    m_rayGenSignature = CreateRayGenSignature();
    m_missSignature = CreateMissSignature();
    m_hitSignature = CreateHitSignature();

    pipeline.AddHitGroup(L"HitGroup", L"ClosestHit");

    pipeline.AddRootSignatureAssociation(m_rayGenSignature.Get(), { L"RayGen" });
    pipeline.AddRootSignatureAssociation(m_missSignature.Get(), { L"Miss" });
    pipeline.AddRootSignatureAssociation(m_hitSignature.Get(), { L"HitGroup" });

    pipeline.SetMaxPayloadSize(4 * sizeof(float));
    pipeline.SetMaxAttributeSize(2 * sizeof(float));
    pipeline.SetMaxRecursionDepth(1);

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
    resDesc.Width = 1280;
    resDesc.Height = 720;
    resDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    resDesc.MipLevels = 1;
    resDesc.SampleDesc.Count = 1;

    ThrowIfFailed(device->CreateCommittedResource(&nv_helpers_dx12::kDefaultHeapProps, D3D12_HEAP_FLAG_NONE, &resDesc, D3D12_RESOURCE_STATE_COPY_SOURCE, nullptr, IID_PPV_ARGS(&m_outputResource)));

}

void CubeDemo::CreateShaderResourceHeap()
{
    auto device = Application::Get().GetDevice().Get();

    m_srvUavHeap = nv_helpers_dx12::CreateDescriptorHeap(device, 3, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, true);

    D3D12_CPU_DESCRIPTOR_HANDLE srvHandle = m_srvUavHeap->GetCPUDescriptorHandleForHeapStart();

    D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
    uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
    device->CreateUnorderedAccessView(m_outputResource.Get(), nullptr, &uavDesc, srvHandle);

    srvHandle.ptr += device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc;
    srvDesc.Format = DXGI_FORMAT_UNKNOWN;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.RaytracingAccelerationStructure.Location = m_topLevelASBuffers.result->GetGPUVirtualAddress();

    D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc{};
    cbvDesc.BufferLocation = m_cameraBuffer->GetGPUVirtualAddress();
    cbvDesc.SizeInBytes = m_cameraBufferSize;
    device->CreateConstantBufferView(&cbvDesc, srvHandle);

    device->CreateShaderResourceView(nullptr, &srvDesc, srvHandle);
}

void CubeDemo::CreateShaderBindingTable()
{
    auto device = Application::Get().GetDevice().Get();

    m_sbtHelper.Reset();
    D3D12_GPU_DESCRIPTOR_HANDLE srvUavHeapHandle = m_srvUavHeap->GetGPUDescriptorHandleForHeapStart();

    auto heapPointer = reinterpret_cast<uint64_t*>(srvUavHeapHandle.ptr);
    m_sbtHelper.AddRayGenerationProgram(L"RayGen", { heapPointer });
    m_sbtHelper.AddMissProgram(L"Miss", {});
    m_sbtHelper.AddHitGroup(L"HitGroup", { {(void*)(_vertexBuffer->GetGPUVirtualAddress()), (void*)(_indexBuffer->GetGPUVirtualAddress())}});

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

    matrices[0] = XMMatrixLookAtRH(Eye, At, Up);

    float fovAngleY = 45.0f * XM_PI / 180.0f;

    matrices[1] = XMMatrixPerspectiveFovRH(fovAngleY, 4.0f / 3.0f, 0.1f, 1000.0f);

    DirectX::XMVECTOR det;
    matrices[2] = DirectX::XMMatrixInverse(&det, matrices[0]);
    matrices[3] = DirectX::XMMatrixInverse(&det, matrices[1]);

    uint8_t* pData;
    ThrowIfFailed(m_cameraBuffer->Map(0, nullptr, reinterpret_cast<void**>(&pData)));
    memcpy(pData, matrices.data(), m_cameraBufferSize);
    m_cameraBuffer->Unmap(0, nullptr);

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
    _camera->SetPosition(0, 0, -35);
    auto device = Application::Get().GetDevice();
    auto commandQueue = Application::Get().GetCommandQueue(D3D12_COMMAND_LIST_TYPE_COPY);
    auto commandList = commandQueue->GetCommandList();

    ComPtr<ID3D12Resource> intermediateVertexBuffer;
    UpdateBufferResource(commandList.Get(),
        &_vertexBuffer, &intermediateVertexBuffer, _countof(vertices), sizeof(VertexPosColor), vertices);

    _vertexBufferView.BufferLocation = _vertexBuffer->GetGPUVirtualAddress();
    _vertexBufferView.SizeInBytes = sizeof(vertices);
    _vertexBufferView.StrideInBytes = sizeof(VertexPosColor);

    ComPtr<ID3D12Resource> intermediateIndexBuffer;
    UpdateBufferResource(commandList.Get(), &_indexBuffer, &intermediateIndexBuffer,
        _countof(indices), sizeof(uint32_t), indices);

    _indexBufferView.BufferLocation = _indexBuffer->GetGPUVirtualAddress();
    _indexBufferView.Format = DXGI_FORMAT_R32_UINT;
    _indexBufferView.SizeInBytes = sizeof(indices);

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
        D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS
    };

    CD3DX12_ROOT_PARAMETER1 rootParameters[1];
    rootParameters[0].InitAsConstants(sizeof(XMMATRIX) / 4, 0, 0, D3D12_SHADER_VISIBILITY_VERTEX);

    CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDescription;
    rootSignatureDescription.Init_1_1(_countof(rootParameters), rootParameters, 0, nullptr, rootSignatureFlags);

    ComPtr<ID3DBlob> rootSignatureBlob;
    ComPtr<ID3DBlob> errorBlob;
    ThrowIfFailedI(D3DX12SerializeVersionedRootSignature(&rootSignatureDescription, featureData.HighestVersion, &rootSignatureBlob, &errorBlob));
    ThrowIfFailedI(device->CreateRootSignature(0, rootSignatureBlob->GetBufferPointer(), rootSignatureBlob->GetBufferSize(), IID_PPV_ARGS(&_rootSignature)));

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

    auto commandQueue2 = Application::Get().GetCommandQueue(D3D12_COMMAND_LIST_TYPE_DIRECT); //D3D12_COMMAND_LIST_TYPE_COPY

    commandList = commandQueue2->GetCommandList();

    AccelerationStructureBuffers bottomLevelBuffers = CreateBottomLevelAS({ {_vertexBuffer.Get(), 8} }, { {_indexBuffer.Get(), 36} }, commandList.Get());
    m_bottomLevelASInstances = { { bottomLevelBuffers.result, XMMatrixIdentity() } };
    CreateTopLevelAS(m_bottomLevelASInstances, commandList.Get());

    fenceValue = commandQueue2->ExecuteCommandList(commandList);
    commandQueue2->WaitForFenceValue(fenceValue);

    m_bottomLevelAS = bottomLevelBuffers.result;

    CreateRayTracingPipeline();
    CreateRaytracingOutputBuffer();
    CreateCameraBuffer();
    CreateShaderResourceHeap();
    CreateShaderBindingTable();

    _contentLoaded = true;
    ResizeDepthBuffer(1280, 720);

    return true;
}

bool CubeDemo::UnloadContent()
{
    return false;
}

void CubeDemo::OnUpdate(UpdateEventArgs& args)
{

    static uint64_t frameCount = 0;
    static double totalTime = 0.0;

    super::OnUpdate(args);
    _camera->CheckForInput(cameraKeyArgument, cameraMouseArgument, args);
    _camera->Update(args);
    
    //Needs to be revised/removed.
    XMVECTOR eyePosition = XMLoadFloat3(&_camera->_position);
    XMVECTOR direction = XMLoadFloat3(&_camera->_direction);
    XMVECTOR upDirection = XMLoadFloat3(&_camera->_up);

    XMMATRIX viewMatrix = XMMatrixLookToLH(eyePosition, direction, upDirection);
    std::vector<XMMATRIX> matrices{ 4 };
    matrices[0] = viewMatrix;
    matrices[1] = XMMatrixPerspectiveFovRH(XMConvertToRadians(45.0f), 4.0f / 3.0f, 0.1f, 1000.0f);
    matrices[2] = DirectX::XMMatrixInverse(nullptr, viewMatrix);
    matrices[3] = DirectX::XMMatrixInverse(nullptr, matrices[0]);

    uint8_t* pData;
    ThrowIfFailed(m_cameraBuffer->Map(0, nullptr, reinterpret_cast<void**>(&pData)));
    memcpy(pData, matrices.data(), m_cameraBufferSize);
    m_cameraBuffer->Unmap(0, nullptr);

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

    float angle = static_cast<float>(args.TotalTime * 0);
    const XMVECTOR rotationAxis = XMVectorSet(0, 1, 1, 0);
    _modelMatrix = XMMatrixRotationAxis(rotationAxis, XMConvertToRadians(angle));
}

void CubeDemo::OnRender(RenderEventArgs& args)
{
    super::OnRender(args);

    auto commandQueue = Application::Get().GetCommandQueue(D3D12_COMMAND_LIST_TYPE_DIRECT);
    auto commandList = commandQueue->GetCommandList();

    UINT currentBackBufferIndex = _window->GetCurrentBackBufferIndex();
    auto backBuffer = _window->GetCurrentBackBuffer();
    auto rtv = _window->GetCurrentRenderTargetView();
    auto dsv = _DSVHeap->GetCPUDescriptorHandleForHeapStart();

    {
        TransitionResource(commandList, backBuffer,
            D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);

        //FLOAT clearColor[]{ 0.4f, 0.6f, 0.9f, 1.0f };
        //ClearRTV(commandList, rtv, clearColor);
        //ClearDepth(commandList, dsv);
    }

    XMMATRIX mvpMatrix = XMMatrixMultiply(_modelMatrix, _camera->ViewProjectionMatrix());

    commandList->SetPipelineState(_pipelineState.Get());
    commandList->SetGraphicsRootSignature(_rootSignature.Get());
    commandList->IASetIndexBuffer(&_indexBufferView);
    commandList->RSSetViewports(1, &_viewport);
    commandList->RSSetScissorRects(1, &_scissorRect);
    commandList->OMSetRenderTargets(1, &rtv, false, &dsv);

    if (_isRaster)
    {
        commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        commandList->IASetVertexBuffers(0, 1, &_vertexBufferView);
        commandList->SetGraphicsRoot32BitConstants(0, sizeof(XMMATRIX) / 4, &mvpMatrix, 0);
        FLOAT clearColor[]{ 0.4f, 0.6f, 0.9f, 1.0f };
        ClearRTV(commandList, rtv, clearColor);
        ClearDepth(commandList, dsv);
        commandList->DrawIndexedInstanced(_countof(indices), 1, 0, 0, 0);
    }

    else
    {

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

        desc.Width = 1280;
        desc.Height = 720;
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
    case KeyCode::Tab:
        _camera->ResetOrientation();
        break;
    case KeyCode::Space:
        _isRaster = !_isRaster;
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
            bottomLevelAS.AddVertexBuffer(vVertexBuffers[i].first.Get(), 0, vVertexBuffers[i].second, sizeof(VertexPosColor), vIndexBuffers[i].first.Get(), 0, vIndexBuffers[i].second, nullptr, 0, true);
            auto vertexBufferVirtualAddress = vVertexBuffers[i].first.Get()->GetGPUVirtualAddress();
            auto indexBufferVirtualAddress = vIndexBuffers[i].first.Get()->GetGPUVirtualAddress();
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

void CubeDemo::CreateTopLevelAS(const std::vector<BottomLevelASInstance>& instances, ID3D12GraphicsCommandList4* commandList)
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
    if (args.Width != 1280 || args.Height != 720)
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
    if (args.LeftButton)
    {
        SetCapture(_window->GetWindowHandle());
        _camera->SetIsAcceptingMouseMovementInputs(true);
        startingMousePos.first = args.X;
        startingMousePos.second = args.Y;
    }
}

void CubeDemo::OnMouseButtonReleased(MouseButtonEventArgs& args)
{
    if (!args.LeftButton)
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
