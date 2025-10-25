#include <windows.h>
#include <wrl.h>
#include <dxgi1_6.h>
#include <d3d12.h>
#include <d3d12sdklayers.h>
#include <iostream>

using namespace Microsoft::WRL;

// デバッグレイヤー有効化
void EnableDebugLayer()
{
#if defined(_DEBUG)
    ComPtr<ID3D12Debug> debugController;
    if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
    {
        debugController->EnableDebugLayer();

        ComPtr<ID3D12Debug1> debugController1;
        if (SUCCEEDED(debugController.As(&debugController1)))
        {
            debugController1->SetEnableGPUBasedValidation(TRUE);
        }
    }
#endif
}

// DXGIファクトリー作成
ComPtr<IDXGIFactory4> CreateDXGIFactory()
{
    UINT createFactoryFlags = 0;
#if defined(_DEBUG)
    createFactoryFlags = DXGI_CREATE_FACTORY_DEBUG;
#endif

    ComPtr<IDXGIFactory4> factory;
    if (FAILED(CreateDXGIFactory2(createFactoryFlags, IID_PPV_ARGS(&factory))))
    {
        OutputDebugString("Failed to create DXGI Factory\n");
        return nullptr;
    }

    return factory;
}

// 最適なハードウェアアダプター取得
ComPtr<IDXGIAdapter1> GetHardwareAdapter(ComPtr<IDXGIFactory4> factory)
{
    ComPtr<IDXGIAdapter1> adapter;

    for (UINT adapterIndex = 0;; ++adapterIndex)
    {
        ComPtr<IDXGIAdapter1> tempAdapter;
        if (DXGI_ERROR_NOT_FOUND == factory->EnumAdapters1(adapterIndex, &tempAdapter))
        {
            break;
        }

        DXGI_ADAPTER_DESC1 desc;
        tempAdapter->GetDesc1(&desc);

        if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
            continue;

        if (SUCCEEDED(D3D12CreateDevice(tempAdapter.Get(), D3D_FEATURE_LEVEL_11_0, _uuidof(ID3D12Device), nullptr)))
        {
            adapter = tempAdapter;
            break;
        }
    }

    return adapter;
}

// DirectX12 デバイス作成
ComPtr<ID3D12Device> CreateD3D12Device(ComPtr<IDXGIAdapter1> adapter)
{
    ComPtr<ID3D12Device> device;
    HRESULT hr = D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&device));
    if (FAILED(hr))
    {
        hr = D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&device));
        if (FAILED(hr))
        {
            OutputDebugString("Failed to create D3D12 Device\n");
            return nullptr;
        }
        OutputDebugString("Using software adapter (WARP)\n");
    }
    return device;
}

// コマンドキュー作成
ComPtr<ID3D12CommandQueue> CreateCommandQueue(ComPtr<ID3D12Device> device)
{
    D3D12_COMMAND_QUEUE_DESC queueDesc = {};
    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    queueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
    queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    queueDesc.NodeMask = 0;

    ComPtr<ID3D12CommandQueue> commandQueue;
    if (FAILED(device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&commandQueue))))
    {
        OutputDebugString("Failed to create Command Queue\n");
        return nullptr;
    }

    commandQueue->SetName(L"Main Command Queue");
    return commandQueue;
}

// スワップチェーン作成
ComPtr<IDXGISwapChain3> CreateSwapChain(ComPtr<IDXGIFactory4> factory, ComPtr<ID3D12CommandQueue> commandQueue, HWND hwnd)
{
    DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
    swapChainDesc.BufferCount = 2;
    swapChainDesc.Width = 1280;
    swapChainDesc.Height = 720;
    swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swapChainDesc.SampleDesc.Count = 1;

    ComPtr<IDXGISwapChain1> swapChain1;
    if (FAILED(factory->CreateSwapChainForHwnd(commandQueue.Get(), hwnd, &swapChainDesc, nullptr, nullptr, &swapChain1)))
    {
        OutputDebugString("Failed to create Swap Chain\n");
        return nullptr;
    }

    ComPtr<IDXGISwapChain3> swapChain;
    if (FAILED(swapChain1.As(&swapChain)))
    {
        OutputDebugString("Failed to cast to SwapChain3\n");
        return nullptr;
    }

    return swapChain;
}

// 例：初期化フロー
void InitializeDirectX(HWND hwnd)
{
    EnableDebugLayer();

    auto factory = CreateDXGIFactory();
    if (!factory) return;

    auto adapter = GetHardwareAdapter(factory);
    if (!adapter)
    {
        OutputDebugString("No suitable hardware adapter found\n");
        return;
    }

    auto device = CreateD3D12Device(adapter);
    if (!device) return;

    auto commandQueue = CreateCommandQueue(device);
    if (!commandQueue) return;

    auto swapChain = CreateSwapChain(factory, commandQueue, hwnd);
    if (!swapChain) return;

    OutputDebugString("DirectX12 initialization complete\n");
}
