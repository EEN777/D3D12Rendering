//#pragma once
//#include "pch.h"
//#include <windows.h>
//#include <wrl.h>
//#include <dxgi1_6.h>
//#include <d3d12.h>
//#include <D3Dcompiler.h>
//#include <DirectXMath.h>
//#include <DirectXPackedVector.h>
//#include <DirectXColors.h>
//#include <DirectXCollision.h>
//#include <string>
//#include <memory>
//#include <algorithm>
//#include <vector>
//#include <array>
//#include <unordered_map>
//#include <cstdint>
//#include <fstream>
//#include <sstream>
//#include <cassert>
//#include <chrono>
//#include <d3dx12.h>
//#include "Helpers.h"
//#include "Window.h"
//
//#if defined(min)
//#undef min
//#endif
//
//#if defined(max)
//#undef max
//#endif
//
//#if defined(CreateWindow)
//#undef CreateWindow
//#endif
//
//using namespace DirectX;
//using namespace Microsoft::WRL;
//
//const uint8_t g_NumFrames{ 3 };
//bool g_UseWarp{ false };
//
//uint32_t g_ClientWidth{ 1280 };
//uint32_t g_ClientHeight{ 720 };
//
//bool g_IsInitialized{ false };
//
//HWND g_hWnd;
//RECT g_WindowRect;
//
//ComPtr<ID3D12Device2> g_Device;
//ComPtr<ID3D12CommandQueue> g_CommandQueue;
//ComPtr<IDXGISwapChain4> g_SwapChain;
//ComPtr<ID3D12Resource> g_BackBuffers[g_NumFrames];
//ComPtr<ID3D12GraphicsCommandList> g_CommandList;
//ComPtr<ID3D12CommandAllocator> g_CommandAllocators[g_NumFrames];
//ComPtr<ID3D12DescriptorHeap> g_RTVDescriptorHeap;
//
//UINT g_RTVDescriptorSize;
//UINT g_CurrentBackBufferIndex;
//
//ComPtr<ID3D12Fence> g_Fence;
//uint64_t g_FenceValue{ 0 };
//uint64_t g_FrameFenceValues[g_NumFrames]{};
//HANDLE g_FenceEvent;
//
//bool g_VSync{ true };
//bool g_TearingSupported{ false };
//bool g_Fullscreen{ false };
//
//
//LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
//
//void EnableDebugLayer()
//{
//#if defined(_DEBUG)
//    ComPtr<ID3D12Debug> debugInterface;
//    ThrowIfFailed(D3D12GetDebugInterface(IID_PPV_ARGS(&debugInterface)));
//    debugInterface->EnableDebugLayer();
//#endif
//}
//
//ComPtr<IDXGIAdapter4> GetAdapter(bool g_UseWarp)
//{
//    ComPtr<IDXGIFactory4> dxgiFactory;
//    UINT createFactoryFlags{ 0 };
//
//#if defined(_DEBUG)
//    createFactoryFlags = DXGI_CREATE_FACTORY_DEBUG;
//#endif
//
//    ThrowIfFailed(CreateDXGIFactory2(createFactoryFlags, IID_PPV_ARGS(&dxgiFactory)));
//    ComPtr<IDXGIAdapter1> dxgiAdapter1;
//    ComPtr<IDXGIAdapter4> dxgiAdapter4;
//
//    if (g_UseWarp)
//    {
//        ThrowIfFailed(dxgiFactory->EnumWarpAdapter(IID_PPV_ARGS(&dxgiAdapter1)));
//        ThrowIfFailed(dxgiAdapter1.As(&dxgiAdapter4));
//    }
//
//    else
//    {
//        std::size_t maxDedicatedVideoMemory{ 0 };
//        for (UINT i{ 0 }; dxgiFactory->EnumAdapters1(i, &dxgiAdapter1) != DXGI_ERROR_NOT_FOUND; ++i)
//        {
//            DXGI_ADAPTER_DESC1 dxgiAdapterDesc1;
//            dxgiAdapter1->GetDesc1(&dxgiAdapterDesc1);
//            auto deviceDesc = dxgiAdapterDesc1.Description;
//            OutputDebugString(deviceDesc);
//
//            if ((dxgiAdapterDesc1.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) == 0 && SUCCEEDED(D3D12CreateDevice(dxgiAdapter1.Get(), D3D_FEATURE_LEVEL_11_0, __uuidof(ID3D12Device), nullptr)) && dxgiAdapterDesc1.DedicatedVideoMemory > maxDedicatedVideoMemory)
//            {
//                maxDedicatedVideoMemory = dxgiAdapterDesc1.DedicatedVideoMemory;
//                ThrowIfFailed(dxgiAdapter1.As(&dxgiAdapter4));
//            }
//        }
//    }
//
//    return dxgiAdapter4;
//}
//
//ComPtr<ID3D12Device2> CreateDevice(ComPtr<IDXGIAdapter3> adapter)
//{
//    ComPtr<ID3D12Device2> d3d12Device2;
//    ThrowIfFailed(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&d3d12Device2)));
//
//#if defined(_DEBUG)
//    ComPtr<ID3D12InfoQueue> pInfoQueue;
//    if (SUCCEEDED(d3d12Device2.As(&pInfoQueue)))
//    {
//        pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, true);
//        pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true);
//        pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, true);
//
//        D3D12_MESSAGE_SEVERITY Severities[]
//        {
//            D3D12_MESSAGE_SEVERITY_INFO
//        };
//
//        D3D12_MESSAGE_ID DenyIDs[]
//        {
//        D3D12_MESSAGE_ID_CLEARRENDERTARGETVIEW_MISMATCHINGCLEARVALUE,
//        D3D12_MESSAGE_ID_MAP_INVALID_NULLRANGE,
//        D3D12_MESSAGE_ID_UNMAP_INVALID_NULLRANGE
//        };
//
//        D3D12_INFO_QUEUE_FILTER NewFilter{};
//        NewFilter.DenyList.NumSeverities = _countof(Severities);
//        NewFilter.DenyList.pSeverityList = Severities;
//        NewFilter.DenyList.NumIDs = _countof(DenyIDs);
//        NewFilter.DenyList.pIDList = DenyIDs;
//
//        ThrowIfFailed(pInfoQueue->PushStorageFilter(&NewFilter));
//    }
//#endif
//
//    return d3d12Device2;
//}
//
//ComPtr<ID3D12CommandQueue> CreateCommandQueue(ComPtr<ID3D12Device2> g_Device, D3D12_COMMAND_LIST_TYPE type)
//{
//    ComPtr<ID3D12CommandQueue> d3d12CommandQueue;
//
//    D3D12_COMMAND_QUEUE_DESC desc{};
//    desc.Type = type;
//    desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
//    desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
//    desc.NodeMask = 0;
//
//    ThrowIfFailed(g_Device->CreateCommandQueue(&desc, IID_PPV_ARGS(&d3d12CommandQueue)));
//
//    return d3d12CommandQueue;
//}
//
//bool CheckTearingSupport()
//{
//    bool allowTearing{ false };
//
//    ComPtr<IDXGIFactory4> factory4;
//    if (SUCCEEDED(CreateDXGIFactory1(IID_PPV_ARGS(&factory4))))
//    {
//        ComPtr<IDXGIFactory5> factory5;
//        if (SUCCEEDED(factory4.As(&factory5)))
//        {
//            if (FAILED(factory5->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &allowTearing, sizeof(allowTearing))))
//            {
//                allowTearing = false;
//            }
//        }
//    }
//
//    return allowTearing == true;
//}
//
//ComPtr<IDXGISwapChain4> CreateSwapChain(HWND hWnd, ComPtr<ID3D12CommandQueue> g_CommandQueue, uint32_t width, uint32_t height, uint32_t bufferCount)
//{
//    ComPtr<IDXGISwapChain4> dxgiSwapChain4;
//    ComPtr<IDXGIFactory4> dxgiFactory4;
//    UINT createFactoryFlags = 0;
//#if defined(_DEBUG)
//    createFactoryFlags = DXGI_CREATE_FACTORY_DEBUG;
//#endif
//
//    ThrowIfFailed(CreateDXGIFactory2(createFactoryFlags, IID_PPV_ARGS(&dxgiFactory4)));
//
//    DXGI_SWAP_CHAIN_DESC1 swapChainDesc{};
//    swapChainDesc.Width = width;
//    swapChainDesc.Height = height;
//    swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
//    swapChainDesc.Stereo = false;
//    swapChainDesc.SampleDesc = { 1, 0 };
//    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
//    swapChainDesc.BufferCount = bufferCount;
//    swapChainDesc.Scaling = DXGI_SCALING_STRETCH;
//    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
//    swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
//    swapChainDesc.Flags = CheckTearingSupport() ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0;
//
//    ComPtr<IDXGISwapChain1> swapChain1;
//    ThrowIfFailed(dxgiFactory4->CreateSwapChainForHwnd(
//        g_CommandQueue.Get(),
//        hWnd,
//        &swapChainDesc,
//        nullptr,
//        nullptr,
//        &swapChain1
//    ));
//
//    ThrowIfFailed(dxgiFactory4->MakeWindowAssociation(hWnd, DXGI_MWA_NO_ALT_ENTER));
//
//    ThrowIfFailed(swapChain1.As(&dxgiSwapChain4));
//
//    return dxgiSwapChain4;
//}
//
//ComPtr<ID3D12DescriptorHeap> CreateDescriptorHeap(ComPtr<ID3D12Device2> g_Device, D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t numDescriptors)
//{
//    ComPtr<ID3D12DescriptorHeap> descriptorHeap;
//
//    D3D12_DESCRIPTOR_HEAP_DESC desc{};
//    desc.NumDescriptors = numDescriptors;
//    desc.Type = type;
//
//    ThrowIfFailed(g_Device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&descriptorHeap)));
//
//    return descriptorHeap;
//}
//
//void UpdateRenderTargetViews(ComPtr<ID3D12Device2> g_Device, ComPtr<IDXGISwapChain4> g_SwapChain, ComPtr<ID3D12DescriptorHeap> descriptorHeap)
//{
//    auto g_RTVDescriptorSize = g_Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
//
//    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(descriptorHeap->GetCPUDescriptorHandleForHeapStart());
//
//    for (int i{ 0 }; i < g_NumFrames; ++i)
//    {
//        ComPtr<ID3D12Resource> backBuffer;
//        ThrowIfFailed(g_SwapChain->GetBuffer(i, IID_PPV_ARGS(&backBuffer)));
//
//        g_Device->CreateRenderTargetView(backBuffer.Get(), nullptr, rtvHandle);
//
//        g_BackBuffers[i] = backBuffer;
//
//        rtvHandle.Offset(g_RTVDescriptorSize);
//    }
//}
//
//ComPtr<ID3D12CommandAllocator> CreateCommandAllocator(ComPtr<ID3D12Device2> g_Device, D3D12_COMMAND_LIST_TYPE type)
//{
//    ComPtr<ID3D12CommandAllocator> commandAllocator;
//    ThrowIfFailed(g_Device->CreateCommandAllocator(type, IID_PPV_ARGS(&commandAllocator)));
//
//    return commandAllocator;
//}
//
//ComPtr<ID3D12GraphicsCommandList> CreateCommandList(ComPtr<ID3D12Device2> g_Device, ComPtr<ID3D12CommandAllocator> commandAllocator, D3D12_COMMAND_LIST_TYPE type)
//{
//    ComPtr<ID3D12GraphicsCommandList> g_CommandList;
//    ThrowIfFailed(g_Device->CreateCommandList(0, type, commandAllocator.Get(), nullptr, IID_PPV_ARGS(&g_CommandList)));
//
//    ThrowIfFailed(g_CommandList->Close());
//
//    return g_CommandList;
//}
//
//ComPtr<ID3D12Fence> CreateFence(ComPtr<ID3D12Device2> g_Device)
//{
//    ComPtr<ID3D12Fence> fence;
//
//    ThrowIfFailed(g_Device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)));
//
//    return fence;
//}
//
//HANDLE CreateEventHandle()
//{
//    HANDLE fenceEvent;
//
//    fenceEvent = ::CreateEvent(nullptr, false, false, nullptr);
//    assert(fenceEvent && "Failed to create fence event!");
//
//    return fenceEvent;
//}
//
//uint64_t Signal(ComPtr <ID3D12CommandQueue> g_CommandQueue, ComPtr<ID3D12Fence> fence, uint64_t& fenceValue)
//{
//    uint64_t fenceValueForSignal = ++fenceValue;
//    ThrowIfFailed(g_CommandQueue->Signal(fence.Get(), fenceValueForSignal));
//
//    return fenceValueForSignal;
//}
//
//void WaitForFenceValue(ComPtr<ID3D12Fence> fence, uint64_t fenceValue, HANDLE fenceEvent, std::chrono::milliseconds duration = std::chrono::milliseconds::max())
//{
//    if (fence->GetCompletedValue() < fenceValue)
//    {
//        ThrowIfFailed(fence->SetEventOnCompletion(fenceValue, fenceEvent));
//        ::WaitForSingleObject(fenceEvent, static_cast<DWORD>(duration.count()));
//    }
//}
//
//void Flush(ComPtr<ID3D12CommandQueue> g_CommandQueue, ComPtr<ID3D12Fence> fence, uint64_t& fenceValue, HANDLE fenceEvent)
//{
//    uint64_t fenceValueForSignal = Signal(g_CommandQueue, fence, fenceValue);
//    WaitForFenceValue(fence, fenceValueForSignal, fenceEvent);
//}
//
//void Update()
//{
//    static uint64_t frameCounter{ 0 };
//    static double elapsedSeconds{ 0.0 };
//    static std::chrono::high_resolution_clock clock;
//    static auto t0 = clock.now();
//
//    frameCounter++;
//    auto t1 = clock.now();
//    auto deltaTime = t1 - t0;
//    t0 = t1;
//
//    elapsedSeconds += deltaTime.count() * 1e-9;
//    if (elapsedSeconds > 1.0)
//    {
//        char buffer[500];
//        auto fps = frameCounter / elapsedSeconds;
//        sprintf_s(buffer, 500, "FPS: %f\n", fps);
//        OutputDebugStringA(buffer);
//
//        frameCounter = 0;
//        elapsedSeconds = 0.0;
//    }
//}
//
//void Render()
//{
//    auto commandAllocator = g_CommandAllocators[g_CurrentBackBufferIndex];
//    auto backBuffer = g_BackBuffers[g_CurrentBackBufferIndex];
//
//    commandAllocator->Reset();
//    g_CommandList->Reset(commandAllocator.Get(), nullptr);
//
//    {
//        CD3DX12_RESOURCE_BARRIER barrier{ CD3DX12_RESOURCE_BARRIER::Transition(backBuffer.Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET) };
//        g_CommandList->ResourceBarrier(1, &barrier);
//
//        float clearColor[]{ 0.4f, 0.6f, 0.9f, 1.0f };
//        CD3DX12_CPU_DESCRIPTOR_HANDLE rtv{ g_RTVDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), (int)g_CurrentBackBufferIndex, g_RTVDescriptorSize };
//        g_CommandList->ClearRenderTargetView(rtv, clearColor, 0, nullptr);
//    }
//
//    {
//        CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
//            backBuffer.Get(),
//            D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
//        g_CommandList->ResourceBarrier(1, &barrier);
//
//        ThrowIfFailed(g_CommandList->Close());
//
//        ID3D12CommandList* const commandLists[]
//        {
//            g_CommandList.Get()
//        };
//
//        g_CommandQueue->ExecuteCommandLists(_countof(commandLists), commandLists);
//        
//        auto type = g_CommandList->GetType();
//        UINT syncInterval = g_VSync ? 1 : 0;
//        UINT presentFlags = g_TearingSupported && !g_VSync ? DXGI_PRESENT_ALLOW_TEARING : 0;
//        ThrowIfFailed(g_SwapChain->Present(syncInterval, presentFlags));
//
//        g_FrameFenceValues[g_CurrentBackBufferIndex] = Signal(g_CommandQueue, g_Fence, g_FenceValue);
//
//        g_CurrentBackBufferIndex = g_SwapChain->GetCurrentBackBufferIndex();
//
//        WaitForFenceValue(g_Fence, g_FrameFenceValues[g_CurrentBackBufferIndex], g_FenceEvent);
//    }
//}
//
//void Resize(uint32_t width, uint32_t height)
//{
//    if (g_ClientWidth != width || g_ClientHeight != height)
//    {
//        g_ClientWidth = std::max(1u, width);
//        g_ClientHeight = std::max(1u, height);
//
//        Flush(g_CommandQueue, g_Fence, g_FenceValue, g_FenceEvent);
//
//        for (int i{ 0 }; i < g_NumFrames; ++i)
//        {
//            g_BackBuffers[i].Reset();
//            g_FrameFenceValues[i] = g_FrameFenceValues[g_CurrentBackBufferIndex];
//        }
//
//        DXGI_SWAP_CHAIN_DESC swapChainDesc{};
//        ThrowIfFailed(g_SwapChain->GetDesc(&swapChainDesc));
//        ThrowIfFailed(g_SwapChain->ResizeBuffers(g_NumFrames, g_ClientWidth, g_ClientHeight, swapChainDesc.BufferDesc.Format, swapChainDesc.Flags));
//
//        g_CurrentBackBufferIndex = g_SwapChain->GetCurrentBackBufferIndex();
//
//        UpdateRenderTargetViews(g_Device, g_SwapChain, g_RTVDescriptorHeap);
//    }
//}
//
//void SetFullScreen(bool _fullscreen)
//{
//    if (g_Fullscreen != g_Fullscreen)
//    {
//        g_Fullscreen = _fullscreen;
//
//        if (g_Fullscreen)
//        {
//            ::GetWindowRect(g_hWnd, &g_WindowRect);
//            
//            UINT windowStyle = WS_OVERLAPPEDWINDOW & ~(WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX);
//
//            ::SetWindowLongW(g_hWnd, GWL_STYLE, windowStyle);
//
//            HMONITOR hMonitor = ::MonitorFromWindow(g_hWnd, MONITOR_DEFAULTTONEAREST);
//            MONITORINFOEX monitorInfo{};
//            monitorInfo.cbSize = sizeof(MONITORINFOEX);
//            ::GetMonitorInfo(hMonitor, &monitorInfo);
//
//            ::SetWindowPos(g_hWnd, HWND_TOP,
//                monitorInfo.rcMonitor.left,
//                monitorInfo.rcMonitor.top,
//                monitorInfo.rcMonitor.right - monitorInfo.rcMonitor.left,
//                monitorInfo.rcMonitor.bottom - monitorInfo.rcMonitor.top,
//                SWP_FRAMECHANGED | SWP_NOACTIVATE);
//
//            ::ShowWindow(g_hWnd, SW_MAXIMIZE);
//        }
//
//        else
//        {
//            ::SetWindowLong(g_hWnd, GWL_STYLE, WS_OVERLAPPEDWINDOW);
//
//            ::SetWindowPos(g_hWnd, HWND_NOTOPMOST,
//                g_WindowRect.left,
//                g_WindowRect.top,
//                g_WindowRect.right - g_WindowRect.left,
//                g_WindowRect.bottom - g_WindowRect.top,
//                SWP_FRAMECHANGED | SWP_NOACTIVATE);
//
//            ::ShowWindow(g_hWnd, SW_NORMAL);
//        }
//    }
//}
//
//LRESULT CALLBACK WndProc(HWND _hwnd, UINT message, WPARAM wParam, LPARAM lParam)
//{
//    if (g_IsInitialized)
//    {
//        switch (message)
//        {
//        case WM_PAINT:
//            Update();
//            Render();
//            break;
//        case WM_SYSKEYDOWN:
//        case WM_KEYDOWN:
//        {
//            bool alt{ (::GetAsyncKeyState(VK_MENU) & 0x8000) != 0 };
//
//            switch (wParam)
//            {
//            case 'V':
//                g_VSync = !g_VSync;
//                break;
//            case VK_ESCAPE:
//                ::PostQuitMessage(0);
//                break;
//            case VK_RETURN:
//                if (alt)
//                {
//            case VK_F11:
//                SetFullScreen(!g_Fullscreen);
//                }
//                break;
//            }
//        }
//        break;
//        case WM_SYSCHAR:
//            break;
//        case WM_SIZE:
//        {
//            RECT clientRect{};
//            ::GetClientRect(g_hWnd, &clientRect);
//
//            int width = clientRect.right - clientRect.left;
//            int height = clientRect.bottom - clientRect.top;
//
//            Resize(width, height);
//        }
//        break;
//        case WM_DESTROY:
//            ::PostQuitMessage(0);
//            break;
//        default:
//            return ::DefWindowProcW(_hwnd, message, wParam, lParam);
//        }
//    }
//
//    else
//    {
//        return ::DefWindowProcW(_hwnd, message, wParam, lParam);
//    }
//
//    return 0;
//}
//
//int CALLBACK wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR lpCmdLine, int nCmdShow)
//{
//    //SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
//
//    //const wchar_t* windowClassName = L"DX12WindowClass";
//    //const wchar_t* windowTitle = L"Learning DirectX 12";
//
//    //EnableDebugLayer();
//
//    //g_TearingSupported = CheckTearingSupport();
//
//    //Window window{ hInstance, windowClassName, windowTitle, &WndProc, g_ClientWidth, g_ClientHeight };
//    //g_hWnd = window._hwnd;
//    //::GetWindowRect(g_hWnd, &g_WindowRect);
//
//    //ComPtr<IDXGIAdapter4> dxgiAdapter4{ GetAdapter(g_UseWarp) };
//
//    //g_Device = CreateDevice(dxgiAdapter4);
//    //g_CommandQueue = CreateCommandQueue(g_Device, D3D12_COMMAND_LIST_TYPE_DIRECT);
//    //g_SwapChain = CreateSwapChain(g_hWnd, g_CommandQueue, g_ClientWidth, g_ClientHeight, g_NumFrames);
//    //g_CurrentBackBufferIndex = g_SwapChain->GetCurrentBackBufferIndex();
//    //g_RTVDescriptorHeap = CreateDescriptorHeap(g_Device, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, g_NumFrames);
//    //g_RTVDescriptorSize = g_Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
//
//    //UpdateRenderTargetViews(g_Device, g_SwapChain, g_RTVDescriptorHeap);
//
//    //for (int i{ 0 }; i < g_NumFrames; ++i)
//    //{
//    //    g_CommandAllocators[i] = CreateCommandAllocator(g_Device, D3D12_COMMAND_LIST_TYPE_DIRECT);
//    //}
//
//    //g_CommandList = CreateCommandList(g_Device, g_CommandAllocators[g_CurrentBackBufferIndex], D3D12_COMMAND_LIST_TYPE_DIRECT);
//
//    //g_Fence = CreateFence(g_Device);
//    //g_FenceEvent = CreateEventHandle();
//
//    //g_IsInitialized = true;
//
//    //::ShowWindow(g_hWnd, SW_SHOW);
//
//    //MSG msg{};
//    //while (msg.message != WM_QUIT)
//    //{
//    //    if (::PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
//    //    {
//    //        ::TranslateMessage(&msg);
//    //        ::DispatchMessage(&msg);
//    //    }
//    //}
//
//    //Flush(g_CommandQueue, g_Fence, g_FenceValue, g_FenceEvent);
//    //::CloseHandle(g_FenceEvent);
//    //return 0;
//}

#include "pch.h"
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <Shlwapi.h>

#include "Application.h"
#include "CubeDemo.h"

#include <dxgidebug.h>

void ReportLiveObjects()
{
    IDXGIDebug1* dxgiDebug;
    DXGIGetDebugInterface1(0, IID_PPV_ARGS(&dxgiDebug));

    //dxgiDebug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_IGNORE_INTERNAL);
    dxgiDebug->Release();
}

int CALLBACK wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR lpCmdLine, int nCmdShow)
{
    int retCode = 0;

    // Set the working directory to the path of the executable.
    WCHAR path[MAX_PATH];
    HMODULE hModule = GetModuleHandleW(NULL);
    if (GetModuleFileNameW(hModule, path, MAX_PATH) > 0)
    {
        PathRemoveFileSpecW(path);
        SetCurrentDirectoryW(path);
    }

    Application::Create(hInstance);
    {
        std::shared_ptr<CubeDemo> demo = std::make_shared<CubeDemo>(L"Learning DirectX 12 - Lesson 2", 1280, 720);
        retCode = Application::Get().Run(demo);
    }
    Application::Destroy();

    atexit(&ReportLiveObjects);

    return retCode;
}