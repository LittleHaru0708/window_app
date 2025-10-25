#pragma once
#include <windows.h>
#include <wrl.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <string>

using Microsoft::WRL::ComPtr;

class D3D12App
{
public:
    D3D12App(HWND hwnd, UINT width, UINT height);
    ~D3D12App();

    bool Initialize();
    void Present();

private:
    bool CreateFactory();
    bool SelectAdapter();
    bool CreateDevice();
    bool CreateCommandQueue();
    bool CreateSwapChain();

private:
    HWND m_hWnd;
    UINT m_width;
    UINT m_height;

    ComPtr<IDXGIFactory6> m_factory;
    ComPtr<IDXGIAdapter1> m_adapter;
    ComPtr<ID3D12Device> m_device;
    ComPtr<ID3D12CommandQueue> m_commandQueue;
    ComPtr<IDXGISwapChain3> m_swapChain;
};
