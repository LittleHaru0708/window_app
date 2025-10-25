#pragma once

#include <windows.h>
#include <wrl.h>
#include <dxgi1_6.h>
#include <d3d12.h>
#include <d3d12sdklayers.h>
#include <iostream>

class DX12Initializer
{
public:
    // コンストラクタ：デバッグレイヤー有効化
    DX12Initializer() { EnableDebugLayer(); }

    // DirectX12 初期化
    bool Initialize(HWND hwnd)
    {
        m_factory = CreateDXGIFactory();
        if (!m_factory) return false;

        m_adapter = GetHardwareAdapter(m_factory);
        if (!m_adapter)
        {
            OutputDebugString("No suitable hardware adapter found\n");
            return false;
        }

        m_device = CreateD3D12Device(m_adapter);
        if (!m_device) return false;

        m_commandQueue = CreateCommandQueue(m_device);
        if (!m_commandQueue) return false;

        m_swapChain = CreateSwapChain(m_factory, m_commandQueue, hwnd);
        if (!m_swapChain) return false;

        OutputDebugString("DirectX12 initialization complete\n");
        return true;
    }

    // ゲッター
    Microsoft::WRL::ComPtr<ID3D12Device> GetDevice() const { return m_device; }
    Microsoft::WRL::ComPtr<ID3D12CommandQueue> GetCommandQueue() const { return m_commandQueue; }
    Microsoft::WRL::ComPtr<IDXGISwapChain3> GetSwapChain() const { return m_swapChain; }
    Microsoft::WRL::ComPtr<IDXGIAdapter1> GetAdapter() const { return m_adapter; }
    Microsoft::WRL::ComPtr<IDXGIFactory4> GetFactory() const { return m_factory; }

private:
    // デバッグレイヤー有効化
    void EnableDebugLayer()
    {
#if defined(_DEBUG)
        Microsoft::WRL::ComPtr<ID3D12Debug> debugController;
        if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
        {
            debugController->EnableDebugLayer();
            Microsoft::WRL::ComPtr<ID3D12Debug1> debugController1;
            if (SUCCEEDED(debugController.As(&debugController1)))
            {
                debugController1->SetEnableGPUBasedValidation(TRUE);
            }
        }
#endif
    }

    // DXGIファクトリー作成
    Microsoft::WRL::ComPtr<IDXGIFactory4> CreateDXGIFactory()
    {
        UINT createFactoryFlags = 0;
#if defined(_DEBUG)
        createFactoryFlags = DXGI_CREATE_FACTORY_DEBUG;
#endif

        Microsoft::WRL::ComPtr<IDXGIFactory4> factory;
        if (FAILED(CreateDXGIFactory2(createFactoryFlags, IID_PPV_ARGS(&factory))))
        {
            OutputDebugString("Failed to create DXGI Factory\n");
            return nullptr;
        }

        return factory;
    }

    // ハードウェアアダプター取得
    Microsoft::WRL::ComPtr<IDXGIAdapter1> GetHardwareAdapter(Microsoft::WRL::ComPtr<IDXGIFactory4> factory)
    {
        Microsoft::WRL::ComPtr<IDXGIAdapter1> adapter;
        for (UINT index = 0;; ++index)
        {
            Microsoft::WRL::ComPtr<IDXGIAdapter1> tempAdapter;
            if (DXGI_ERROR_NOT_FOUND == factory->EnumAdapters1(index, &tempAdapter))
                break;

            DXGI_ADAPTER_DESC1 desc;
            tempAdapter->GetDesc1(&desc);
            if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) continue;

            if (SUCCEEDED(D3D12CreateDevice(tempAdapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&adapter))))
            {
                adapter = tempAdapter;
                break;
            }
        }
        return adapter;
    }

    // D3D12 デバイス作成
    Microsoft::WRL::ComPtr<ID3D12Device> CreateD3D12Device(Microsoft::WRL::ComPtr<IDXGIAdapter1> adapter)
    {
        Microsoft::WRL::ComPtr<ID3D12Device> device;
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
    Microsoft::WRL::ComPtr<ID3D12CommandQueue> CreateCommandQueue(Microsoft::WRL::ComPtr<ID3D12Device> device)
    {
        D3D12_COMMAND_QUEUE_DESC desc = {};
        desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
        desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
        desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
        desc.NodeMask = 0;

        Microsoft::WRL::ComPtr<ID3D12CommandQueue> queue;
        if (FAILED(device->CreateCommandQueue(&desc, IID_PPV_ARGS(&queue))))
        {
            OutputDebugString("Failed to create Command Queue\n");
            return nullptr;
        }
        queue->SetName(L"Main Command Queue");
        return queue;
    }

    // スワップチェーン作成
    Microsoft::WRL::ComPtr<IDXGISwapChain3> CreateSwapChain(Microsoft::WRL::ComPtr<IDXGIFactory4> factory,
        Microsoft::WRL::ComPtr<ID3D12CommandQueue> queue,
        HWND hwnd)
    {
        DXGI_SWAP_CHAIN_DESC1 desc = {};
        desc.BufferCount = 2;
        desc.Width = 1280;
        desc.Height = 720;
        desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
        desc.SampleDesc.Count = 1;

        Microsoft::WRL::ComPtr<IDXGISwapChain1> swapChain1;
        if (FAILED(factory->CreateSwapChainForHwnd(queue.Get(), hwnd, &desc, nullptr, nullptr, &swapChain1)))
        {
            OutputDebugString("Failed to create Swap Chain\n");
            return nullptr;
        }

        Microsoft::WRL::ComPtr<IDXGISwapChain3> swapChain;
        if (FAILED(swapChain1.As(&swapChain)))
        {
            OutputDebugString("Failed to cast to SwapChain3\n");
            return nullptr;
        }
        return swapChain;
    }

private:
    Microsoft::WRL::ComPtr<IDXGIFactory4> m_factory;
    Microsoft::WRL::ComPtr<IDXGIAdapter1> m_adapter;
    Microsoft::WRL::ComPtr<ID3D12Device> m_device;
    Microsoft::WRL::ComPtr<ID3D12CommandQueue> m_commandQueue;
    Microsoft::WRL::ComPtr<IDXGISwapChain3> m_swapChain;
};
