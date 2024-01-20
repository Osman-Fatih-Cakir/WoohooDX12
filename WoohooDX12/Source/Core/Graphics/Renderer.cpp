#include "Renderer.h"

#include <cassert>
#include "CrossWindow/Graphics.h"
#include "Maths.h"
#include "Utils.h"

namespace WoohooDX12
{
  Renderer::Renderer()
  {
    // Assign default values
    for (size_t i = 0; i < m_backbufferCount; ++i)
    {
      m_renderTargets[i] = nullptr;
    }

    m_mesh = std::make_shared<Mesh>();
    m_material = std::make_shared<Material>();
  }
  
  Renderer::~Renderer()
  {
    m_material->UnInit();
    m_mesh->UnUnit();
    UnInit();

    m_material = nullptr;
    m_mesh = nullptr;
  }

  int Renderer::Init(uint32 width, uint32 height, void* windowPtr)
  {
    m_window = windowPtr;
    m_width = width;
    m_height = height;

    ReturnIfFailed(InitAPI());

    ReturnIfFailed(InitResources());

    ReturnIfFailed(SetupCommands());

    return 0;
  }

  int Renderer::UnInit()
  {
    if (m_swapchain != nullptr)
    {
      m_swapchain->SetFullscreenState(false, nullptr);
      m_swapchain->Release();
      m_swapchain = nullptr;
    }

    DestroyCommands();

    DestroyFrameBuffer();

    DestroyResources();

    DestroyAPI();

    return 0;
  }

  int Renderer::Resize(uint32 width, uint32 height)
  {
    m_width = width;
    m_height = height;

    // NOTE: WAITING FOR THE FRAME TO COMPLETE BEFORE CONTINUING IS NOT BEST PRACTICE.

    // Signal and increment the fence value.
    const uint64 fence = m_fenceValue;
    ReturnIfFailed(m_commandQueue->Signal(m_fence, fence));
    m_fenceValue++;

    // Wait until the previous frame is finished.
    if (m_fence->GetCompletedValue() < fence)
    {
      ReturnIfFailed(m_fence->SetEventOnCompletion(fence, m_fenceEvent));
      WaitForSingleObjectEx(m_fenceEvent, INFINITE, false);
    }

    ReturnIfFailed(DestroyFrameBuffer());
    ReturnIfFailed(SetupSwapchain(width, height));
    ReturnIfFailed(InitFrameBuffer());

    return 0;
  }

  int Renderer::Render()
  {
    // Update Uniforms
    m_material->Update();

    // Record all the commands we need to render the scene into the command
    // list.
    SetupCommands();

    // Execute the command list.
    ID3D12CommandList* ppCommandLists[] = { m_commandList };
    m_commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);
    m_swapchain->Present(1, 0);

    // WAITING FOR THE FRAME TO COMPLETE BEFORE CONTINUING IS NOT BEST PRACTICE.

    // Signal and increment the fence value.
    const UINT64 fence = m_fenceValue;
    ReturnIfFailed(m_commandQueue->Signal(m_fence, fence));
    m_fenceValue++;

    // Wait until the previous frame is finished.
    if (m_fence->GetCompletedValue() < fence)
    {
      ReturnIfFailed(m_fence->SetEventOnCompletion(fence, m_fenceEvent));
      WaitForSingleObject(m_fenceEvent, INFINITE);
    }

    m_frameIndex = m_swapchain->GetCurrentBackBufferIndex();

    return 0;
  }

  int Renderer::InitAPI()
  {
    Log("Initializing API...", LogType::LT_INFO);

    // Create factory
    uint32 dxgiFactoryFlags = 0;
#ifdef DX12_DEBUG_LAYER
    ID3D12Debug* debugController;
    ReturnIfFailed(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)));
    ReturnIfFailed(debugController->QueryInterface(IID_PPV_ARGS(&m_debugController)));
    m_debugController->EnableDebugLayer();
    m_debugController->SetEnableGPUBasedValidation(true);

    dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;

    debugController->Release();
    debugController = nullptr;
#endif

    ReturnIfFailed(CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&m_factory)));

    // Create adapter
    for (uint32 adapterIndex = 0; DXGI_ERROR_NOT_FOUND != m_factory->EnumAdapters1(adapterIndex, &m_adapter); ++adapterIndex)
    {
      DXGI_ADAPTER_DESC1 desc;
      m_adapter->GetDesc1(&desc);

      if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
      {
        // Don't select the Basic Render Driver adapter.
        continue;
      }

      // Check to see if the adapter supports Direct3D 12, but don't create the actual device yet.
      if (SUCCEEDED(D3D12CreateDevice(m_adapter, D3D_FEATURE_LEVEL_12_0, _uuidof(ID3D12Device), nullptr)))
      {
        break;
      }

      // We won't use this adapter, so release it
      m_adapter->Release();
    }

    // Create device
    ID3D12Device* dev = nullptr;
    ReturnIfFailed(D3D12CreateDevice(m_adapter, D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&m_device)));
    m_device->SetName(L"Main Device");

#ifdef DX12_DEBUG_LAYER
    // Get debug device
    ReturnIfFailed(m_device->QueryInterface(&m_debugDevice));
#endif

    // Create command queue
    D3D12_COMMAND_QUEUE_DESC queueDesc = {};
    queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

    ReturnIfFailed(m_device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_commandQueue)));

    // Create command allocator
    ReturnIfFailed(m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_commandAllocator)));

    // Sync
    ReturnIfFailed(m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence)));

    // Create swapchain
    ReturnIfFailed(Resize(m_width, m_height));

    Log("API has been initialized.", LogType::LT_INFO);

    return 0;
  }

  int Renderer::InitResources()
  {
    Log("Initializing API resources...", LogType::LT_INFO);

    ReturnIfFailed(m_material->Init(m_device));

    ReturnIfFailed(CreateCommands(m_material->m_pipelineState));

    // Command lists are created in the recording state, but there is nothing to record yet. The main loop expects it to be closed, so close it now.
    ReturnIfFailed(m_commandList->Close());
    ReturnIfFailed(m_commandAllocator->Reset());
    ReturnIfFailed(m_commandList->Reset(m_commandAllocator, m_material->m_pipelineState));

    ReturnIfFailed(m_mesh->Init(m_device, m_commandList));

    // Execute upload commands
    ReturnIfFailed(m_commandList->Close());
    ID3D12CommandList* ppCommandLists[] = { m_commandList };
    m_commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

    // Create synchronization objects and wait until assets have been uploaded to the GPU.
    {
      m_fenceValue = 1;

      // Create an event handle to use for frame synchronization.
      m_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
      if (m_fenceEvent == nullptr)
      {
        ReturnIfFailed(HRESULT_FROM_WIN32(GetLastError()));
      }

      // Wait for the command list to execute; we are reusing the same command list in our main loop but for now, we just want to wait for setup to
      // complete before continuing.

      // Signal and increment the fence value.
      const uint64 fence = m_fenceValue;
      ReturnIfFailed(m_commandQueue->Signal(m_fence, fence));
      m_fenceValue++;

      // Wait until the previous frame is finished.
      if (m_fence->GetCompletedValue() < fence)
      {
        ReturnIfFailed(m_fence->SetEventOnCompletion(fence, m_fenceEvent));
        WaitForSingleObject(m_fenceEvent, INFINITE);
      }

      m_frameIndex = m_swapchain->GetCurrentBackBufferIndex();
    }

    Log("API resources has been initialized.", LogType::LT_INFO);

    return 0;
  }

  int Renderer::SetupCommands()
  {
    // Command list allocators can only be reset when the associated
    // command lists have finished execution on the GPU; apps should use
    // fences to determine GPU execution progress.
    ReturnIfFailed(m_commandAllocator->Reset());

    // However, when ExecuteCommandList() is called on a particular command
    // list, that command list can then be reset at any time and must be before
    // re-recording.
    ReturnIfFailed(m_commandList->Reset(m_commandAllocator, m_material->m_pipelineState));

    // Set necessary state.
    m_commandList->SetGraphicsRootSignature(m_material->m_rootSignature);
    m_commandList->RSSetViewports(1, &m_viewport);
    m_commandList->RSSetScissorRects(1, &m_surfaceSize);

    ID3D12DescriptorHeap* pDescriptorHeaps[] = { m_material->m_uniformBufferHeap };
    m_commandList->SetDescriptorHeaps(_countof(pDescriptorHeaps), pDescriptorHeaps);

    D3D12_GPU_DESCRIPTOR_HANDLE srvHandle(m_material->m_uniformBufferHeap->GetGPUDescriptorHandleForHeapStart());
    m_commandList->SetGraphicsRootDescriptorTable(0, srvHandle);

    // Indicate that the back buffer will be used as a render target.
    D3D12_RESOURCE_BARRIER renderTargetBarrier = {};
    renderTargetBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    renderTargetBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    renderTargetBarrier.Transition.pResource = m_renderTargets[m_frameIndex];
    renderTargetBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
    renderTargetBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
    renderTargetBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

    m_commandList->ResourceBarrier(1, &renderTargetBarrier);

    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_rtvHeap->GetCPUDescriptorHandleForHeapStart());
    rtvHandle.ptr = rtvHandle.ptr + (m_frameIndex * m_rtvDescriptorSize);
    m_commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);

    // Record commands.
    const float clearColor[] = { 0.2f, 0.2f, 0.2f, 1.0f };
    m_commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
    m_commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    m_commandList->IASetVertexBuffers(0, 1, &m_mesh->m_vertexBufferView);
    m_commandList->IASetIndexBuffer(&m_mesh->m_indexBufferView);

    m_commandList->DrawIndexedInstanced(3, 1, 0, 0, 0);

    // Indicate that the back buffer will now be used to present.
    D3D12_RESOURCE_BARRIER presentBarrier;
    presentBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    presentBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    presentBarrier.Transition.pResource = m_renderTargets[m_frameIndex];
    presentBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
    presentBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
    presentBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

    m_commandList->ResourceBarrier(1, &presentBarrier);

    ReturnIfFailed(m_commandList->Close());

    return 0;
  }

  int Renderer::InitFrameBuffer()
  {
    m_currentBuffer = m_swapchain->GetCurrentBackBufferIndex();

    // Create descriptor heaps.
    {
      // Describe and create a render target view (RTV) descriptor heap.
      D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
      rtvHeapDesc.NumDescriptors = m_backbufferCount;
      rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
      rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
      ReturnIfFailed(m_device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&m_rtvHeap)));

      m_rtvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    }

    // Create frame resources.
    {
      D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_rtvHeap->GetCPUDescriptorHandleForHeapStart());

      // Create a RTV for each frame.
      for (uint32 n = 0; n < m_backbufferCount; n++)
      {
        ReturnIfFailed(m_swapchain->GetBuffer(n, IID_PPV_ARGS(&m_renderTargets[n])));
        m_device->CreateRenderTargetView(m_renderTargets[n], nullptr, rtvHandle);
        rtvHandle.ptr += (1 * m_rtvDescriptorSize);
      }
    }

    return 0;
  }

  int Renderer::CreateCommands(ID3D12PipelineState* pipelineState)
  {
    // Create the command list.
    ReturnIfFailed(m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_commandAllocator, pipelineState, IID_PPV_ARGS(&m_commandList)));
    m_commandList->SetName(L"Main Command List");

    return 0;
  }

  int Renderer::SetupSwapchain(uint32 width, uint32 height)
  {
    m_surfaceSize.left = 0;
    m_surfaceSize.top = 0;
    m_surfaceSize.right = (LONG)m_width;
    m_surfaceSize.bottom = (LONG)m_height;

    //NOTE: These should be same with rendering camera projection parameters
    m_viewport.TopLeftX = 0.0f;
    m_viewport.TopLeftY = 0.0f;
    m_viewport.Width = (float)m_width;
    m_viewport.Height = (float)m_height;
    m_viewport.MinDepth = 0.1f;
    m_viewport.MaxDepth = 1000.f;

    if (m_swapchain != nullptr)
    {
      m_swapchain->ResizeBuffers(m_backbufferCount, m_width, m_height, DXGI_FORMAT_R8G8B8A8_UNORM, 0);
    }
    else
    {
      DXGI_SWAP_CHAIN_DESC1 swapchainDesc = {};
      swapchainDesc.BufferCount = m_backbufferCount;
      swapchainDesc.Width = width;
      swapchainDesc.Height = height;
      swapchainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
      swapchainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
      swapchainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
      swapchainDesc.SampleDesc.Count = 1;

      assert(m_window != nullptr);

      AssertAndReturn(m_window != nullptr, "Renderer does not have window.");

      IDXGISwapChain1* swapchain = xgfx::createSwapchain((xwin::Window*)m_window, m_factory, m_commandQueue, &swapchainDesc);
      HRESULT swapchainSupport = swapchain->QueryInterface(__uuidof(IDXGISwapChain3), (void**)&swapchain);
      if (SUCCEEDED(swapchainSupport))
      {
        m_swapchain = (IDXGISwapChain3*)swapchain;
      }
      else
      {
        return -1;
      }
    }
    m_frameIndex = m_swapchain->GetCurrentBackBufferIndex();

    return 0;
  }

  int Renderer::DestroyCommands()
  {
    if (m_commandList)
    {
      m_commandList->Reset(m_commandAllocator, m_material->m_pipelineState);
      m_commandList->ClearState(m_material->m_pipelineState);
      ReturnIfFailed(m_commandList->Close());
      ID3D12CommandList* ppCommandLists[] = { m_commandList };
      m_commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

      // Wait for GPU to finish work
      const uint64 fence = m_fenceValue;
      ReturnIfFailed(m_commandQueue->Signal(m_fence, fence));
      m_fenceValue++;
      if (m_fence->GetCompletedValue() < fence)
      {
        ReturnIfFailed(m_fence->SetEventOnCompletion(fence, m_fenceEvent));
        WaitForSingleObject(m_fenceEvent, INFINITE);
      }

      m_commandList->Release();
      m_commandList = nullptr;
    }

    return 0;
  }

  int Renderer::DestroyFrameBuffer()
  {
    for (uint32 i = 0; i < m_backbufferCount; ++i)
    {
      if (m_renderTargets[i])
      {
        m_renderTargets[i]->Release();
        m_renderTargets[i] = 0;
      }
    }
    if (m_rtvHeap)
    {
      m_rtvHeap->Release();
      m_rtvHeap = nullptr;
    }

    return 0;
  }

  int Renderer::DestroyResources()
  {
    // Sync
    CloseHandle(m_fenceEvent);

    m_mesh->UnUnit();
    m_material->UnInit();

    return 0;
  }

  int Renderer::DestroyAPI()
  {
    if (m_fence)
    {
      m_fence->Release();
      m_fence = nullptr;
    }

    if (m_commandAllocator)
    {
      ReturnIfFailed(m_commandAllocator->Reset());
      m_commandAllocator->Release();
      m_commandAllocator = nullptr;
    }

    if (m_commandQueue)
    {
      m_commandQueue->Release();
      m_commandQueue = nullptr;
    }

    if (m_device)
    {
      m_device->Release();
      m_device = nullptr;
    }

    if (m_adapter)
    {
      m_adapter->Release();
      m_adapter = nullptr;
    }

    if (m_factory)
    {
      m_factory->Release();
      m_factory = nullptr;
    }

#ifdef DX12_DEBUG_LAYER
    if (m_debugController)
    {
      m_debugController->Release();
      m_debugController = nullptr;
    }

    D3D12_RLDO_FLAGS flags = D3D12_RLDO_SUMMARY | D3D12_RLDO_DETAIL | D3D12_RLDO_IGNORE_INTERNAL;

    m_debugDevice->ReportLiveDeviceObjects(flags);

    if (m_debugDevice)
    {
      m_debugDevice->Release();
      m_debugDevice = nullptr;
    }
#endif

    return 0;
  }

}
