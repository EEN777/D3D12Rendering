#include "GameObject.h"
#include "Application.h"
#include "CommonStructures.h"
#include "CommandQueue.h"

void GameObject::UpdateBufferResource(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList4> commandList, ID3D12Resource** destinationResource, ID3D12Resource** intermediateResource, std::size_t numElements, std::size_t elementSize, const void* bufferData, D3D12_RESOURCE_FLAGS flags)
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

        UpdateSubresources(commandList.Get(), *destinationResource, *intermediateResource, 0, 0, 1, &subresourceData);
    }
}

GameObject::GameObject(std::wstring modelFile, std::wstring textureFile, Library::ContentManager& contentManager) :
    _modelFile{ modelFile }, _textureFile{ textureFile }, _contentManager{ &contentManager }
{
}

void GameObject::Initialize(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList4> /*commandList*/, std::shared_ptr<FirstPersonCamera> camera)
{
    auto device = Application::Get().GetDevice();
    auto commandQueue = Application::Get().GetCommandQueue(D3D12_COMMAND_LIST_TYPE_DIRECT);

    _camera = camera;

    const auto model = _contentManager->Load<Library::Model>(_modelFile);
    Library::Mesh* mesh = model->Meshes().at(0).get();


    auto vertexVector = mesh->Vertices();
    std::vector<DirectX::XMFLOAT3> texCoordVector{};
    texCoordVector = mesh->TextureCoordinates().size() > 0 ? mesh->TextureCoordinates().at(0) : texCoordVector;

    std::vector<DirectX::XMFLOAT3> vertexAndColorVector;
    std::vector<UINT> meshIndices{};

    std::size_t currentPosition{ 0 };
    for (auto& vert : vertexVector)
    {
        vertexAndColorVector.push_back(vert);
        if (currentPosition < texCoordVector.size())
        {
            vertexAndColorVector.push_back(texCoordVector.at(currentPosition));
        }
        ++currentPosition;
    }

    _vertexCount = vertexAndColorVector.size();

    meshIndices = mesh->Indices();

    auto commandList = commandQueue->GetCommandList();

    ComPtr<ID3D12Resource> intermediateVertexBuffer;
    UpdateBufferResource(commandList.Get(),
        &_vertexBuffer, &intermediateVertexBuffer, vertexAndColorVector.size(), sizeof(VertexPosColor), vertexAndColorVector.data());

    _vertexBufferView.BufferLocation = _vertexBuffer->GetGPUVirtualAddress();
    _vertexBufferView.SizeInBytes = sizeof(XMFLOAT3) * static_cast<UINT>(vertexAndColorVector.size());
    _vertexBufferView.StrideInBytes = sizeof(VertexPosColor);

    ComPtr<ID3D12Resource> intermediateIndexBuffer;
    UpdateBufferResource(commandList.Get(), &_indexBuffer, &intermediateIndexBuffer,
        meshIndices.size(), sizeof(uint32_t), meshIndices.data());

    _indexBufferView.BufferLocation = _indexBuffer->GetGPUVirtualAddress();
    _indexBufferView.Format = DXGI_FORMAT_R32_UINT;
    _indexBufferView.SizeInBytes = static_cast<UINT>(meshIndices.size()) * sizeof(UINT);

    _indexCount = static_cast<UINT>(meshIndices.size());

    D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
    srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    srvHeapDesc.NumDescriptors = 1; // Number of textures to bind
    srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    ThrowIfFailedI(device->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(_texDescriptorHeap.GetAddressOf())));

    CD3DX12_CPU_DESCRIPTOR_HANDLE hDescriptor(_texDescriptorHeap->GetCPUDescriptorHandleForHeapStart());


    auto textureCommandList = commandQueue->GetCommandList();

    ThrowIfFailedI(DirectX::CreateDDSTextureFromFile12(device.Get(),
        textureCommandList.Get(), _textureFile.c_str(),
        _textureResource, _textureUploadResource));

    auto textureFenceValue = commandQueue->ExecuteCommandList(textureCommandList);
    commandQueue->WaitForFenceValue(textureFenceValue);

    // Need to load texture first.
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = _textureResource->GetDesc().Format;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = 1; // Only one level of detail
    srvDesc.Texture2D.MostDetailedMip = 0;
    srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

    device->CreateShaderResourceView(_textureResource.Get(), &srvDesc, hDescriptor);

    auto fenceValue = commandQueue->ExecuteCommandList(commandList);
    commandQueue->WaitForFenceValue(fenceValue);
}

void GameObject::UpdateObject()
{
    const XMVECTOR rotationAxis = XMVectorSet(0, 1, 1, 0);
    _modelMatrix = XMMatrixRotationAxis(rotationAxis, XMConvertToRadians(0));

    auto fwd = Library::Vector3Helper::Forward;
    auto up = Library::Vector3Helper::Up;
    auto right = Library::Vector3Helper::Right;
    auto pos = XMFLOAT3{ -15.0f, -15.0f, -15.0f };

    XMMATRIX worldMatrix = XMMatrixIdentity();
    Library::MatrixHelper::SetForward(worldMatrix, fwd);
    Library::MatrixHelper::SetUp(worldMatrix, up);
    Library::MatrixHelper::SetRight(worldMatrix, right);
    Library::MatrixHelper::SetTranslation(worldMatrix, pos);

    XMFLOAT4X4 temp;
    XMStoreFloat4x4(&temp, XMMatrixScaling(1.0f, 1.0f, 1.0f) * worldMatrix);

    worldMatrix = XMLoadFloat4x4(&temp);

    _mvpMatrix = XMMatrixMultiply(_modelMatrix, _camera->ViewProjectionMatrix());
}

Microsoft::WRL::ComPtr<ID3D12Resource>& GameObject::VertexBuffer()
{
    return _vertexBuffer;
}

Microsoft::WRL::ComPtr<ID3D12Resource>& GameObject::IndexBuffer()
{
    return _indexBuffer;
}

D3D12_VERTEX_BUFFER_VIEW GameObject::VertexBufferView()
{
    return _vertexBufferView;
}

D3D12_INDEX_BUFFER_VIEW GameObject::IndexBufferView()
{
    return _indexBufferView;
}

ComPtr<ID3D12DescriptorHeap> GameObject::TextureDecsriptorHeap()
{
    return _texDescriptorHeap;
}

ComPtr<ID3D12Resource>& GameObject::TextureResource()
{
    return _textureResource;
}

UINT GameObject::IndexCount()
{
    return _indexCount;
}

UINT GameObject::VertCount()
{
    return _vertexCount;
}

void GameObject::DrawObject(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList4> commandList)
{
    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    commandList->IASetVertexBuffers(0, 1, &_vertexBufferView);
    commandList->IASetIndexBuffer(&_indexBufferView);
    commandList->SetGraphicsRoot32BitConstants(0, sizeof(XMMATRIX) / 4, &_mvpMatrix, 0);

    ID3D12DescriptorHeap* ppHeaps[] = { _texDescriptorHeap.Get() };
    commandList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);

    commandList->SetGraphicsRootDescriptorTable(1, _texDescriptorHeap->GetGPUDescriptorHandleForHeapStart());

    commandList->DrawIndexedInstanced(_indexCount, 1, 0, 0, 0);
}
