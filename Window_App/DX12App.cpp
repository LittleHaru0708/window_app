#include "DX12App.h"
#include <stdexcept>

using namespace DirectX;

// -----------------------------------------------------------
// コンストラクタ / デストラクタ
// -----------------------------------------------------------
DX12App::DX12App(HWND hwnd, UINT width, UINT height)
    : m_hWnd(hwnd), m_width(width), m_height(height), m_fenceEvent(nullptr)
{
}

DX12App::~DX12App()
{
    WaitForGPU();
    if (m_fenceEvent) CloseHandle(m_fenceEvent);
}

// -----------------------------------------------------------
// 初期化
// -----------------------------------------------------------
bool DX12App::Initialize()
{
    if (!CreateFactory()) return false;
    if (!SelectAdapter()) return false;
    if (!CreateDevice()) return false;
    if (!CreateCommandQueue()) return false;
    if (!CreateSwapChain()) return false;
    if (!CreateRTVHeap()) return false;
    if (!CreateRenderTargets()) return false;
    if (!CreateCommandAllocators()) return false;
    if (!CreateCommandList()) return false;
    if (!CreateFence()) return false;

    if (!CompileShaders()) return false;
    if (!CreateRootSignature()) return false;
    if (!CreatePipelineState()) return false;
    if (!CreateTriangleResources()) return false;

    return true;
}

// -----------------------------------------------------------
// DXGI Factory
// -----------------------------------------------------------
bool DX12App::CreateFactory()
{
#if defined(_DEBUG)
    UINT flags = DXGI_CREATE_FACTORY_DEBUG;
#else
    UINT flags = 0;
#endif

    return SUCCEEDED(CreateDXGIFactory2(flags, IID_PPV_ARGS(&m_factory)));
}

// -----------------------------------------------------------
// アダプタ選択
// -----------------------------------------------------------
bool DX12App::SelectAdapter()
{
    for (UINT i = 0;; i++)
    {
        ComPtr<IDXGIAdapter1> adapter;
        if (DXGI_ERROR_NOT_FOUND == m_factory->EnumAdapters1(i, &adapter)) break;

        DXGI_ADAPTER_DESC1 desc;
        adapter->GetDesc1(&desc);

        // ソフトウェアを除外
        if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) continue;

        if (SUCCEEDED(D3D12CreateDevice(adapter.Get(),
            D3D_FEATURE_LEVEL_11_0, _uuidof(ID3D12Device), nullptr)))
        {
            m_adapter = adapter;
            return true;
        }
    }
    return false;
}

// -----------------------------------------------------------
// Device
// -----------------------------------------------------------
bool DX12App::CreateDevice()
{
    return SUCCEEDED(D3D12CreateDevice(
        m_adapter.Get(),
        D3D_FEATURE_LEVEL_11_0,
        IID_PPV_ARGS(&m_device)
    ));
}

// -----------------------------------------------------------
// Command Queue
// -----------------------------------------------------------
bool DX12App::CreateCommandQueue()
{
    D3D12_COMMAND_QUEUE_DESC desc{};
    desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

    return SUCCEEDED(
        m_device->CreateCommandQueue(&desc, IID_PPV_ARGS(&m_commandQueue))
    );
}

// -----------------------------------------------------------
// Swap Chain
// -----------------------------------------------------------
bool DX12App::CreateSwapChain()
{
    DXGI_SWAP_CHAIN_DESC1 desc{};
    desc.BufferCount = 2;
    desc.Width = m_width;
    desc.Height = m_height;
    desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    desc.SampleDesc.Count = 1;

    ComPtr<IDXGISwapChain1> temp;
    if (FAILED(m_factory->CreateSwapChainForHwnd(
        m_commandQueue.Get(),
        m_hWnd,
        &desc,
        nullptr,
        nullptr,
        &temp)))
        return false;

    return SUCCEEDED(temp.As(&m_swapChain));
}

// -----------------------------------------------------------
// RTV Heap
// -----------------------------------------------------------
bool DX12App::CreateRTVHeap()
{
    D3D12_DESCRIPTOR_HEAP_DESC desc{};
    desc.NumDescriptors = 2;
    desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;

    bool ok = SUCCEEDED(
        m_device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&m_rtvHeap))
    );

    m_rtvDescriptorSize =
        m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

    return ok;
}

// -----------------------------------------------------------
// Render Target
// -----------------------------------------------------------
bool DX12App::CreateRenderTargets()
{
    for (UINT i = 0; i < 2; i++)
    {
        if (FAILED(m_swapChain->GetBuffer(i, IID_PPV_ARGS(&m_renderTargets[i]))))
            return false;

        D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle =
            m_rtvHeap->GetCPUDescriptorHandleForHeapStart();
        rtvHandle.ptr += i * m_rtvDescriptorSize;

        m_device->CreateRenderTargetView(m_renderTargets[i].Get(), nullptr, rtvHandle);
    }
    return true;
}

// -----------------------------------------------------------
// Command Allocator
// -----------------------------------------------------------
bool DX12App::CreateCommandAllocators()
{
    for (UINT i = 0; i < 2; i++)
    {
        if (FAILED(m_device->CreateCommandAllocator(
            D3D12_COMMAND_LIST_TYPE_DIRECT,
            IID_PPV_ARGS(&m_commandAllocators[i]))))
            return false;
    }
    return true;
}

// -----------------------------------------------------------
// Command List
// -----------------------------------------------------------
bool DX12App::CreateCommandList()
{
    if (FAILED(m_device->CreateCommandList(
        0,
        D3D12_COMMAND_LIST_TYPE_DIRECT,
        m_commandAllocators[0].Get(),
        nullptr,
        IID_PPV_ARGS(&m_commandList))))
        return false;

    m_commandList->Close();
    return true;
}

// -----------------------------------------------------------
// Fence
// -----------------------------------------------------------
bool DX12App::CreateFence()
{
    if (FAILED(m_device->CreateFence(
        0,
        D3D12_FENCE_FLAG_NONE,
        IID_PPV_ARGS(&m_fence))))
        return false;

    m_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    return m_fenceEvent != nullptr;
}
// -----------------------------------------------------------
// Shader コンパイル
// -----------------------------------------------------------
bool DX12App::CompileShaders() 
{
    UINT flags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;

    if (FAILED(D3DCompileFromFile(
        L"VertexShader.hlsl", nullptr, nullptr,
        "VSMain", "vs_5_0",
        flags, 0,
        &m_vsBlob, nullptr)))
        return false;

    if (FAILED(D3DCompileFromFile(
        L"PixelShader.hlsl", nullptr, nullptr,
        "PSMain", "ps_5_0",
        flags, 0,
        &m_psBlob, nullptr)))
        return false;

    return true;
}
// -----------------------------------------------------------
// Root Signature
// -----------------------------------------------------------
bool DX12App::CreateRootSignature() 
{
    D3D12_ROOT_SIGNATURE_DESC
}
