#include "pch.h"
#include "CubeDemo.h"

#include "Application.h"
#include "CommandQueue.h"
#include "Helpers.h"
#include "Window.h"

#include <wrl.h>
template <typename T>
using ComPtr = Microsoft::WRL::ComPtr<T>;

#include <d3dx12.h>
#include <d3dcompiler.h>

#include <algorithm>

using namespace DirectX;

KeyCode::Key cameraArgument{};

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

static WORD indices[36]
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
        _countof(indices), sizeof(WORD), indices);

    _indexBufferView.BufferLocation = _indexBuffer->GetGPUVirtualAddress();
    _indexBufferView.Format = DXGI_FORMAT_R16_UINT;
    _indexBufferView.SizeInBytes = sizeof(indices);

    D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc{};
    dsvHeapDesc.NumDescriptors = 1;
    dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
    dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    ThrowIfFailed(device->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&_DSVHeap)));

    ComPtr<ID3DBlob> vertexShaderBlob;
    ThrowIfFailed(D3DReadFileToBlob(L"VertexShader.cso", &vertexShaderBlob));

    ComPtr<ID3DBlob> pixelShaderBlob;
    ThrowIfFailed(D3DReadFileToBlob(L"PixelShader.cso", &pixelShaderBlob));

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
    ThrowIfFailed(D3DX12SerializeVersionedRootSignature(&rootSignatureDescription, featureData.HighestVersion, &rootSignatureBlob, &errorBlob));
    ThrowIfFailed(device->CreateRootSignature(0, rootSignatureBlob->GetBufferPointer(), rootSignatureBlob->GetBufferSize(), IID_PPV_ARGS(&_rootSignature)));

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

    ThrowIfFailed(device->CreatePipelineState(&pipelineStateStreamDesc, IID_PPV_ARGS(&_pipelineState)));

    auto fenceValue = commandQueue->ExecuteCommandList(commandList);
    commandQueue->WaitForFenceValue(fenceValue);

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
    _camera->CheckForInput(cameraArgument, args);
    _camera->Update(args);
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

    float angle = static_cast<float>(args.TotalTime * 10);
    const XMVECTOR rotationAxis = XMVectorSet(0, 1, 1, 0);
    _modelMatrix = XMMatrixRotationAxis(rotationAxis, XMConvertToRadians(angle));

    const XMVECTOR eyePosition{ XMVectorSet(0, 0, -10, 1) };
    const XMVECTOR focusPoint{ XMVectorSet(0, 0, 0, 1) };
    const XMVECTOR upDirection{ XMVectorSet(0, 1, 0, 0) };
    _viewMatrix = XMMatrixLookAtLH(eyePosition, focusPoint, upDirection);

    float aspectRatio = 1280 / static_cast<float>(720);
    _projectionMatrix = XMMatrixPerspectiveFovLH(XMConvertToRadians(_fieldOfView), aspectRatio, 0.1f, 100.0f);
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

        FLOAT clearColor[]{ 0.4f, 0.6f, 0.9f, 1.0f };
        ClearRTV(commandList, rtv, clearColor);
        ClearDepth(commandList, dsv);
    }

    commandList->SetPipelineState(_pipelineState.Get());
    commandList->SetGraphicsRootSignature(_rootSignature.Get());
    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    commandList->IASetVertexBuffers(0, 1, &_vertexBufferView);
    commandList->IASetIndexBuffer(&_indexBufferView);
    commandList->RSSetViewports(1, &_viewport);
    commandList->RSSetScissorRects(1, &_scissorRect);
    commandList->OMSetRenderTargets(1, &rtv, false, &dsv);

    //XMMATRIX mvpMatrix = XMMatrixMultiply(_modelMatrix, _viewMatrix);
    auto a = _projectionMatrix;
    auto b = _camera->ViewProjectionMatrix();
    auto c = _viewMatrix;
    auto d = XMMatrixMultiply(_viewMatrix, _projectionMatrix);
    XMMATRIX mvpMatrix = XMMatrixMultiply(_modelMatrix, _camera->ViewProjectionMatrix()); //_projectionMatrix
    //mvpMatrix = XMMatrixMultiply(mvpMatrix, _projectionMatrix); //_projectionMatrix
    commandList->SetGraphicsRoot32BitConstants(0, sizeof(XMMATRIX) / 4, &mvpMatrix, 0);

    commandList->DrawIndexedInstanced(_countof(indices), 1, 0, 0, 0);

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
        cameraArgument = args.Key;
        break;
    default:
        break;
    }
}

void CubeDemo::OnKeyReleased(KeyEventArgs& args)
{
    if (args.Key == cameraArgument)
    {
        cameraArgument = KeyCode::None;
    }
}

void CubeDemo::OnMouseWheel(MouseWheelEventArgs& args)
{


}

void CubeDemo::UpdateBufferResource(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> commandList, ID3D12Resource** destinationResource, ID3D12Resource** intermediateResource, std::size_t numElements, std::size_t elementSize, const void* bufferData, D3D12_RESOURCE_FLAGS flags)
{
    auto device = Application::Get().GetDevice();
    std::size_t bufferSize = numElements * elementSize;
    auto heapProperty1 = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
    auto heapProperty2 = CD3DX12_RESOURCE_DESC::Buffer(bufferSize, flags);
    ThrowIfFailed(device->CreateCommittedResource(
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
        ThrowIfFailed(device->CreateCommittedResource(
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

        ThrowIfFailed(device->CreateCommittedResource(
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

void CubeDemo::TransitionResource(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> commandList, Microsoft::WRL::ComPtr<ID3D12Resource> resource, D3D12_RESOURCE_STATES beforeState, D3D12_RESOURCE_STATES afterState)
{
    CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
        resource.Get(),
        beforeState, afterState);

    commandList->ResourceBarrier(1, &barrier);
}

void CubeDemo::ClearRTV(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> commandList, D3D12_CPU_DESCRIPTOR_HANDLE rtv, float* clearColor)
{
    commandList->ClearRenderTargetView(rtv, clearColor, 0, nullptr);
}

void CubeDemo::ClearDepth(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> commandList, D3D12_CPU_DESCRIPTOR_HANDLE dsv, float depth)
{
    commandList->ClearDepthStencilView(dsv, D3D12_CLEAR_FLAG_DEPTH, depth, 0, 0, nullptr);
}
