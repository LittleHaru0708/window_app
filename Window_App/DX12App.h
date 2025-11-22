#pragma once
#include <windows.h>
#include <wrl.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>

using Microsoft::WRL::ComPtr;

class DX12App
{
public:
    DX12App(HWND hwnd, UINT width, UINT height);
    ~DX12App();

    bool Initialize();
    void Render();
    void WaitForGPU();

private:
    bool CreateFactory();
    bool SelectAdapter();
    bool CreateDevice();
    bool CreateCommandQueue();
    bool CreateSwapChain();
    bool CreateRTVHeap();
    bool CreateRenderTargets();
    bool CreateCommandAllocators();
    bool CreateCommandList();
    bool CreateFence();

    // 三角形描画専用追加
    bool CreateTriangleResources();      // 頂点/インデックス/ビュー
    bool CompileShaders();               // HLSL → バイトコード
    bool CreateRootSignature();          // ルートシグネチャ
    bool CreatePipelineState();          // PSO 作成

private:
    HWND m_hWnd{};
    UINT m_width{};
    UINT m_height{};

    ComPtr<IDXGIFactory6> m_factory;
    ComPtr<IDXGIAdapter1> m_adapter;
    ComPtr<ID3D12Device> m_device;
    ComPtr<ID3D12CommandQueue> m_commandQueue;
    ComPtr<IDXGISwapChain3> m_swapChain;

    ComPtr<ID3D12DescriptorHeap> m_rtvHeap;
    UINT m_rtvDescriptorSize;
    ComPtr<ID3D12Resource> m_renderTargets[2];

    ComPtr<ID3D12CommandAllocator> m_commandAllocators[2];
    ComPtr<ID3D12GraphicsCommandList> m_commandList;

    ComPtr<ID3D12Fence> m_fence;
    UINT64 m_fenceValues[2]{ 0,0 };
    UINT64 m_fenceValue = 0;
    HANDLE m_fenceEvent;

    // 三角形描画
    struct Vertex
    {
        DirectX::XMFLOAT3 pos;
        DirectX::XMFLOAT4 color;
    };

    ComPtr<ID3D12Resource> m_vertexBuffer;
    ComPtr<ID3D12Resource> m_indexBuffer;
    D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView{};
    D3D12_INDEX_BUFFER_VIEW m_indexBufferView{};
    UINT m_indexCount{};

    ComPtr<ID3DBlob> m_vsBlob;
    ComPtr<ID3DBlob> m_psBlob;

    ComPtr<ID3D12RootSignature> m_rootSignature;
    ComPtr<ID3D12PipelineState> m_pipelineState;
};
