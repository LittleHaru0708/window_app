#include "DX12App.h"
#include <stdexcept>
#include <vector>
#include "d3dx12.h" // ïKê{ÅFDirectX12 Helper

using namespace DirectX;

// -----------------------------------------------------------
// Constructor / Destructor
// -----------------------------------------------------------
DX12App::DX12App(HWND hwnd, UINT width, UINT height)
    : m_hWnd(hwnd), m_width(width), m_height(height)
{
}

DX12App::~DX12App()
{
    WaitForGPU();
    if (m_fenceEvent) CloseHandle(m_fenceEvent);
}

// -----------------------------------------------------------
// Initialize pipeline and resources
// -----------------------------------------------------------
bool DX12App::Initialize()
{
#if defined(_DEBUG)
    {
        ComPtr<ID3D12Debug> debugController;
        if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
        {
            debugController->EnableDebugLayer();
        }
    }
#endif

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
// Factory / Adapter / Device / Queue
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

bool DX12App::SelectAdapter()
{
    for (UINT i = 0;; ++i)
    {
        ComPtr<IDXGIAdapter1> adapter;
        if (DXGI_ERROR_NOT_FOUND == m_factory->EnumAdapters1(i, &adapter))
            break;

        DXGI_ADAPTER_DESC1 desc;
        adapter->GetDesc1(&desc);
        if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) continue;

        if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, _uuidof(ID3D12Device), nullptr)))
        {
            m_adapter = adapter;
            return true;
        }
    }
    return false;
}

bool DX12App::CreateDevice()
{
    return SUCCEEDED(D3D12CreateDevice(m_adapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&m_device)));
}

bool DX12App::CreateCommandQueue()
{
    D3D12_COMMAND_QUEUE_DESC desc{};
    desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    return SUCCEEDED(m_device->CreateCommandQueue(&desc, IID_PPV_ARGS(&m_commandQueue)));
}

// -----------------------------------------------------------
// Swap chain / RTV
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

    ComPtr<IDXGISwapChain1> swapChain1;
    if (FAILED(m_factory->CreateSwapChainForHwnd(m_commandQueue.Get(), m_hWnd, &desc, nullptr, nullptr, &swapChain1)))
        return false;

    if (FAILED(swapChain1.As(&m_swapChain))) return false;

    return true;
}

bool DX12App::CreateRTVHeap()
{
    D3D12_DESCRIPTOR_HEAP_DESC desc{};
    desc.NumDescriptors = 2;
    desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

    if (FAILED(m_device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&m_rtvHeap))))
        return false;

    m_rtvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    return true;
}

bool DX12App::CreateRenderTargets()
{
    for (UINT i = 0; i < 2; ++i)
    {
        if (FAILED(m_swapChain->GetBuffer(i, IID_PPV_ARGS(&m_renderTargets[i]))))
            return false;

        D3D12_CPU_DESCRIPTOR_HANDLE handle = m_rtvHeap->GetCPUDescriptorHandleForHeapStart();
        handle.ptr += i * m_rtvDescriptorSize;
        m_device->CreateRenderTargetView(m_renderTargets[i].Get(), nullptr, handle);
    }
    return true;
}

// -----------------------------------------------------------
// Command allocators / list / fence
// -----------------------------------------------------------
bool DX12App::CreateCommandAllocators()
{
    for (UINT i = 0; i < 2; ++i)
    {
        if (FAILED(m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_commandAllocators[i]))))
            return false;
    }
    return true;
}

bool DX12App::CreateCommandList()
{
    if (FAILED(m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_commandAllocators[0].Get(), nullptr, IID_PPV_ARGS(&m_commandList))))
        return false;

    m_commandList->Close(); ///< ç≈èâÇÕï¬Ç∂ÇƒÇ®Ç≠
    return true;
}

bool DX12App::CreateFence()
{
    if (FAILED(m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence))))
        return false;

    m_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    return (m_fenceEvent != nullptr);
}

// -----------------------------------------------------------
// Shader compile
// -----------------------------------------------------------
bool DX12App::CompileShaders()
{
    UINT compileFlags = D3DCOMPILE_ENABLE_STRICTNESS;
#if defined(_DEBUG)
    compileFlags |= D3DCOMPILE_DEBUG;
#endif

    if (FAILED(D3DCompileFromFile(L"VertexShader.hlsl", nullptr, nullptr, "VSMain", "vs_5_0", compileFlags, 0, &m_vsBlob, nullptr)))
        return false;

    if (FAILED(D3DCompileFromFile(L"PixelShader.hlsl", nullptr, nullptr, "PSMain", "ps_5_0", compileFlags, 0, &m_psBlob, nullptr)))
        return false;

    return true;
}

// -----------------------------------------------------------
// Root signature
// -----------------------------------------------------------
bool DX12App::CreateRootSignature()
{
    D3D12_ROOT_SIGNATURE_DESC rsDesc{};
    rsDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

    ComPtr<ID3DBlob> sig;
    ComPtr<ID3DBlob> err;

    if (FAILED(D3D12SerializeRootSignature(&rsDesc, D3D_ROOT_SIGNATURE_VERSION_1, &sig, &err)))
        return false;

    return SUCCEEDED(m_device->CreateRootSignature(0, sig->GetBufferPointer(), sig->GetBufferSize(), IID_PPV_ARGS(&m_rootSignature)));
}

// -----------------------------------------------------------
// Pipeline State
// -----------------------------------------------------------
bool DX12App::CreatePipelineState()
{
    D3D12_INPUT_ELEMENT_DESC inputLayout[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT     , 0, 0 , D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "COLOR"   , 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
    };

    D3D12_GRAPHICS_PIPELINE_STATE_DESC desc{};
    desc.InputLayout = { inputLayout, _countof(inputLayout) };
    desc.pRootSignature = m_rootSignature.Get();
    desc.VS = { m_vsBlob->GetBufferPointer(), m_vsBlob->GetBufferSize() };
    desc.PS = { m_psBlob->GetBufferPointer(), m_psBlob->GetBufferSize() };
    desc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    desc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    desc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
    desc.DepthStencilState.DepthEnable = FALSE;
    desc.SampleMask = UINT_MAX;
    desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    desc.NumRenderTargets = 1;
    desc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.SampleDesc.Count = 1;

    return SUCCEEDED(m_device->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(&m_pipelineState)));
}

// -----------------------------------------------------------
// Triangle resources
// -----------------------------------------------------------
bool DX12App::CreateTriangleResources()
{
    Vertex vertices[] =
    {
        { { 0.0f, 0.5f, 0.0f },{ 1,0,0,1 } },
        { { 0.5f,-0.5f, 0.0f },{ 0,1,0,1 } },
        { {-0.5f,-0.5f, 0.0f },{ 0,0,1,1 } }
    };

    uint16_t indices[] = { 0,1,2 };
    m_indexCount = _countof(indices);

    // GPUÇ÷ upload heap Çégóp
    UINT vbSize = sizeof(vertices);
    UINT ibSize = sizeof(indices);

    CD3DX12_HEAP_PROPERTIES heap(D3D12_HEAP_TYPE_UPLOAD);
    CD3DX12_RESOURCE_DESC vbDesc = CD3DX12_RESOURCE_DESC::Buffer(vbSize);
    m_device->CreateCommittedResource(&heap, D3D12_HEAP_FLAG_NONE, &vbDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
        IID_PPV_ARGS(&m_vertexBuffer));

    void* mapped = nullptr;
    m_vertexBuffer->Map(0, nullptr, &mapped);
    memcpy(mapped, vertices, vbSize);
    m_vertexBuffer->Unmap(0, nullptr);

    m_vertexBufferView.BufferLocation = m_vertexBuffer->GetGPUVirtualAddress();
    m_vertexBufferView.SizeInBytes = vbSize;
    m_vertexBufferView.StrideInBytes = sizeof(Vertex);

    CD3DX12_RESOURCE_DESC ibDesc = CD3DX12_RESOURCE_DESC::Buffer(ibSize);
    m_device->CreateCommittedResource(&heap, D3D12_HEAP_FLAG_NONE, &ibDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
        IID_PPV_ARGS(&m_indexBuffer));

    m_indexBuffer->Map(0, nullptr, &mapped);
    memcpy(mapped, indices, ibSize);
    m_indexBuffer->Unmap(0, nullptr);

    m_indexBufferView.BufferLocation = m_indexBuffer->GetGPUVirtualAddress();
    m_indexBufferView.SizeInBytes = ibSize;
    m_indexBufferView.Format = DXGI_FORMAT_R16_UINT;

    return true;
}

// -----------------------------------------------------------
// Render
// -----------------------------------------------------------
void DX12App::Render()
{
    UINT backIndex = m_swapChain->GetCurrentBackBufferIndex();

    if (m_fence->GetCompletedValue() < m_fenceValues[backIndex])
    {
        m_fence->SetEventOnCompletion(m_fenceValues[backIndex], m_fenceEvent);
        WaitForSingleObject(m_fenceEvent, INFINITE);
    }

    m_commandAllocators[backIndex]->Reset();
    m_commandList->Reset(m_commandAllocators[backIndex].Get(), m_pipelineState.Get());

    // Present Å® RenderTarget Ç÷ëJà⁄
    CD3DX12_RESOURCE_BARRIER toRT = CD3DX12_RESOURCE_BARRIER::Transition(
        m_renderTargets[backIndex].Get(),
        D3D12_RESOURCE_STATE_PRESENT,
        D3D12_RESOURCE_STATE_RENDER_TARGET);
    m_commandList->ResourceBarrier(1, &toRT);

    D3D12_CPU_DESCRIPTOR_HANDLE rtv =
        m_rtvHeap->GetCPUDescriptorHandleForHeapStart();
    rtv.ptr += backIndex * m_rtvDescriptorSize;

    const float clearColor[] = { 1.0f, 0.0f, 0.0f, 1.0f };//Ç±Ç±ÇÃêîílÇÇ¢Ç∂ÇÍÇŒêFÇ™ïœÇ¶ÇÁÇÍÇÈîwåiÇÃÇPÇ™ç≈ëÂ
    m_commandList->ClearRenderTargetView(rtv, clearColor, 0, nullptr);

    // ÅöViewport / Scissor èCê≥î≈ÅiäÆëSë„ì¸ï˚éÆÅj
    D3D12_VIEWPORT vp;
    vp.TopLeftX = 0.0f;
    vp.TopLeftY = 0.0f;
    vp.Width = (float)m_width;
    vp.Height = (float)m_height;
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;

    D3D12_RECT scissor;
    scissor.left = 0;
    scissor.top = 0;
    scissor.right = m_width;
    scissor.bottom = m_height;

    m_commandList->RSSetViewports(1, &vp);
    m_commandList->RSSetScissorRects(1, &scissor);

    // Draw
    m_commandList->SetPipelineState(m_pipelineState.Get());
    m_commandList->SetGraphicsRootSignature(m_rootSignature.Get());
    m_commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    m_commandList->IASetVertexBuffers(0, 1, &m_vertexBufferView);
    m_commandList->IASetIndexBuffer(&m_indexBufferView);
    m_commandList->OMSetRenderTargets(1, &rtv, FALSE, nullptr);
    m_commandList->DrawIndexedInstanced(m_indexCount, 1, 0, 0, 0);

    // RenderTarget Å® Present Ç÷ñﬂÇ∑
    CD3DX12_RESOURCE_BARRIER toPresent = CD3DX12_RESOURCE_BARRIER::Transition(
        m_renderTargets[backIndex].Get(),
        D3D12_RESOURCE_STATE_RENDER_TARGET,
        D3D12_RESOURCE_STATE_PRESENT);
    m_commandList->ResourceBarrier(1, &toPresent);

    m_commandList->Close();
    ID3D12CommandList* lists[] = { m_commandList.Get() };
    m_commandQueue->ExecuteCommandLists(1, lists);

    ++m_fenceValue;
    m_commandQueue->Signal(m_fence.Get(), m_fenceValue);
    m_fenceValues[backIndex] = m_fenceValue;

    m_swapChain->Present(1, 0);
}

void DX12App::WaitForGPU()
{
    ++m_fenceValue;
    m_commandQueue->Signal(m_fence.Get(), m_fenceValue);
    m_fence->SetEventOnCompletion(m_fenceValue, m_fenceEvent);
    WaitForSingleObject(m_fenceEvent, INFINITE);
}
