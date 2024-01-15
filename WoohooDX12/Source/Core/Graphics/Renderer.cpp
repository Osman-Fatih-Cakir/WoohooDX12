#include "Renderer.h"

#include <cassert>
#include <d3dcompiler.h>
#include <fstream>
#include "CrossWindow/Graphics.h"
#include "Maths.h"
#include "RendererUtils.h"
#include "Utils.h"

namespace WoohooDX12
{
  Renderer::Renderer()
  {
    MakeIdentity(uboVS.projectionMatrix);
    MakeIdentity(uboVS.viewMatrix);
    MakeIdentity(uboVS.modelMatrix);

    // Assign default values
    for (size_t i = 0; i < m_backbufferCount; ++i)
    {
      m_renderTargets[i] = nullptr;
    }
  }

  Renderer::~Renderer()
  {
    UnInit();
  }

  bool Renderer::Init(uint32 width, uint32 height, void* windowPtr)
  {
    m_window = windowPtr;
    m_width = width;
    m_height = height;

    ReturnIfFalse(InitAPI());

    ReturnIfFalse(InitResources());

    ReturnIfFalse(SetupCommands());

    return true;
  }

  bool Renderer::UnInit()
  {
    if (m_swapchain != nullptr)
    {
      m_swapchain->SetFullscreenState(false, nullptr);
      m_swapchain->Release();
      m_swapchain = nullptr;
    }

    ReturnIfFalse(DestroyCommands());

    ReturnIfFalse(DestroyFrameBuffer());

    ReturnIfFalse(DestroyResources());

    ReturnIfFalse(DestroyAPI());

    return true;
  }

  bool Renderer::Resize(uint32 width, uint32 height)
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

    ReturnIfFalse(DestroyFrameBuffer());
    ReturnIfFalse(SetupSwapchain(width, height));
    ReturnIfFalse(InitFrameBuffer());

    return true;
  }

  bool Renderer::Render()
  {
    // Update Uniforms
    uboVS.modelMatrix *= DirectX::XMMatrixRotationAxis(DirectX::XMLoadFloat3(&UpVector), DirectX::XMConvertToRadians(1.0f));

    D3D12_RANGE readRange = {};
    readRange.Begin = 0;
    readRange.End = 0;

    ReturnIfFailed(m_uniformBuffer->Map(0, &readRange, reinterpret_cast<void**>(&m_mappedUniformBuffer)));
    memcpy(m_mappedUniformBuffer, &uboVS, sizeof(uboVS));
    m_uniformBuffer->Unmap(0, &readRange);

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

    return true;
  }

  bool Renderer::InitAPI()
  {
    Log("Initializing API...", LogType::LT_INFO);

    // Create factory
    uint32 dxgiFactoryFlags = 0;
#ifdef DX12_DEBUG
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

#ifdef DX12_DEBUG
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

    return true;
  }

  bool Renderer::InitResources()
  {
    Log("Initializing API resources...", LogType::LT_INFO);

    // Create root signature
    {
      D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData = {};

      // This is the highest version the sample supports. If CheckFeatureSupport succeeds,
      // the HighestVersion returned will not be greater than this.
      featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;

      if (FAILED(m_device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &featureData, sizeof(featureData))))
      {
        featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
      }

      D3D12_DESCRIPTOR_RANGE1 ranges[1];
      ranges[0].BaseShaderRegister = 0;
      ranges[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
      ranges[0].NumDescriptors = 1;
      ranges[0].RegisterSpace = 0;
      ranges[0].OffsetInDescriptorsFromTableStart = 0;
      ranges[0].Flags = D3D12_DESCRIPTOR_RANGE_FLAG_NONE;

      D3D12_ROOT_PARAMETER1 rootParameters[1];
      rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
      rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;

      rootParameters[0].DescriptorTable.NumDescriptorRanges = 1;
      rootParameters[0].DescriptorTable.pDescriptorRanges = ranges;

      D3D12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc;
      rootSignatureDesc.Version = D3D_ROOT_SIGNATURE_VERSION_1_1;
      rootSignatureDesc.Desc_1_1.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
      rootSignatureDesc.Desc_1_1.NumParameters = 1;
      rootSignatureDesc.Desc_1_1.pParameters = rootParameters;
      rootSignatureDesc.Desc_1_1.NumStaticSamplers = 0;
      rootSignatureDesc.Desc_1_1.pStaticSamplers = nullptr;


      ID3DBlob* signature;
      ID3DBlob* error;
      try
      {
        ReturnIfFailed(D3D12SerializeVersionedRootSignature(&rootSignatureDesc, &signature, &error));
        ReturnIfFailed(m_device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_rootSignature)));
        m_rootSignature->SetName(L"Main Root Signature");
      }
      catch (std::exception e)
      {
        const char* errStr = (const char*)error->GetBufferPointer();
        Log(errStr, LogType::LT_ERROR);
        error->Release();
        error = nullptr;
      }

      if (signature)
      {
        signature->Release();
        signature = nullptr;
      }
    }

    // Create the pipeline state
    {
      ID3DBlob* vertexShader = nullptr;
      ID3DBlob* pixelShader = nullptr;
      ReturnIfFalse(CompileShaders(&vertexShader, &pixelShader));

      // Define the vertex input layout
      D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
      {
          {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
          {"COLOR", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}
      };

      // Create the UBO
      {
        //TODO use GPU memory for vertex buffer
        D3D12_HEAP_PROPERTIES heapProps = {};
        heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
        heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
        heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
        heapProps.CreationNodeMask = 1;
        heapProps.VisibleNodeMask = 1;

        D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
        heapDesc.NumDescriptors = 1;
        heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
        heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        ReturnIfFailed(m_device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&m_uniformBufferHeap)));

        D3D12_RESOURCE_DESC uboResourceDesc = {};
        uboResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        uboResourceDesc.Alignment = 0;
        uboResourceDesc.Width = (sizeof(uboVS) + 255) & ~255;
        uboResourceDesc.Height = 1;
        uboResourceDesc.DepthOrArraySize = 1;
        uboResourceDesc.MipLevels = 1;
        uboResourceDesc.Format = DXGI_FORMAT_UNKNOWN;
        uboResourceDesc.SampleDesc.Count = 1;
        uboResourceDesc.SampleDesc.Quality = 0;
        uboResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        uboResourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

        ReturnIfFailed(m_device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &uboResourceDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&m_uniformBuffer)));
        m_uniformBufferHeap->SetName(L"Constant Buffer Upload Resource Heap");

        D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
        cbvDesc.BufferLocation = m_uniformBuffer->GetGPUVirtualAddress();
        cbvDesc.SizeInBytes = (sizeof(uboVS) + 255) & ~255; // CB size is required to be 256-byte aligned

        D3D12_CPU_DESCRIPTOR_HANDLE cbvHandle(m_uniformBufferHeap->GetCPUDescriptorHandleForHeapStart());
        cbvHandle.ptr += m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV) * 0;

        m_device->CreateConstantBufferView(&cbvDesc, cbvHandle);

        // We do not intend to read from this resource on the CPU. (End is less than or equal to begin)
        D3D12_RANGE readRange = {};
        readRange.Begin = 0;
        readRange.End = 0;

        ReturnIfFailed(m_uniformBuffer->Map(0, &readRange, reinterpret_cast<void**>(&m_mappedUniformBuffer)));
        memcpy(m_mappedUniformBuffer, &uboVS, sizeof(uboVS));
        m_uniformBuffer->Unmap(0, &readRange);
      }

      // Describe and create the graphics pipeline state object (PSO)
      D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
      psoDesc.InputLayout = { inputElementDescs, _countof(inputElementDescs) };
      psoDesc.pRootSignature = m_rootSignature;

      D3D12_SHADER_BYTECODE vsBytecode = {};
      D3D12_SHADER_BYTECODE psBytecode = {};

      vsBytecode.pShaderBytecode = vertexShader->GetBufferPointer();
      vsBytecode.BytecodeLength = vertexShader->GetBufferSize();

      psBytecode.pShaderBytecode = pixelShader->GetBufferPointer();
      psBytecode.BytecodeLength = pixelShader->GetBufferSize();

      psoDesc.VS = vsBytecode;
      psoDesc.PS = psBytecode;

      D3D12_RASTERIZER_DESC rasterDesc = {};
      rasterDesc.FillMode = D3D12_FILL_MODE_SOLID;
      rasterDesc.CullMode = D3D12_CULL_MODE_NONE;
      rasterDesc.FrontCounterClockwise = FALSE;
      rasterDesc.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
      rasterDesc.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
      rasterDesc.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
      rasterDesc.DepthClipEnable = TRUE;
      rasterDesc.MultisampleEnable = FALSE;
      rasterDesc.AntialiasedLineEnable = FALSE;
      rasterDesc.ForcedSampleCount = 0;
      rasterDesc.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

      psoDesc.RasterizerState = rasterDesc;

      D3D12_BLEND_DESC blendDesc;
      blendDesc.AlphaToCoverageEnable = FALSE;
      blendDesc.IndependentBlendEnable = FALSE;
      const D3D12_RENDER_TARGET_BLEND_DESC defaultRenderTargetBlendDesc =
      {
        FALSE,
        FALSE,
        D3D12_BLEND_ONE,
        D3D12_BLEND_ZERO,
        D3D12_BLEND_OP_ADD,
        D3D12_BLEND_ONE,
        D3D12_BLEND_ZERO,
        D3D12_BLEND_OP_ADD,
        D3D12_LOGIC_OP_NOOP,
        D3D12_COLOR_WRITE_ENABLE_ALL,
      };
      for (uint32 i = 0; i < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; ++i)
        blendDesc.RenderTarget[i] = defaultRenderTargetBlendDesc;

      psoDesc.BlendState = blendDesc;
      psoDesc.DepthStencilState.DepthEnable = FALSE;
      psoDesc.DepthStencilState.StencilEnable = FALSE;
      psoDesc.SampleMask = UINT_MAX;
      psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
      psoDesc.NumRenderTargets = 1;
      psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
      psoDesc.SampleDesc.Count = 1;
      try
      {
        ReturnIfFailed(m_device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pipelineState)));
      }
      catch (std::exception e)
      {
        Log("Failed to create Graphics Pipeline!", LogType::LT_ERROR);
      }

      if (vertexShader)
      {
        vertexShader->Release();
        vertexShader = nullptr;
      }

      if (pixelShader)
      {
        pixelShader->Release();
        pixelShader = nullptr;
      }
    }

    ReturnIfFalse(CreateCommands());

    // Command lists are created in the recording state, but there is nothing to record yet. The main loop expects it to be closed, so close it now.
    ReturnIfFailed(m_commandList->Close());

    // Create vertex buffer
    {
      const uint32 vertexBufferSize = sizeof(m_vertexBufferData);

      //TODO use vertex buffer on GPU memory
      D3D12_HEAP_PROPERTIES heapProps = {};
      heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
      heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
      heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
      heapProps.CreationNodeMask = 1;
      heapProps.VisibleNodeMask = 1;

      D3D12_RESOURCE_DESC vertexBufferResourceDesc = {};
      vertexBufferResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
      vertexBufferResourceDesc.Alignment = 0;
      vertexBufferResourceDesc.Width = vertexBufferSize;
      vertexBufferResourceDesc.Height = 1;
      vertexBufferResourceDesc.DepthOrArraySize = 1;
      vertexBufferResourceDesc.MipLevels = 1;
      vertexBufferResourceDesc.Format = DXGI_FORMAT_UNKNOWN;
      vertexBufferResourceDesc.SampleDesc.Count = 1;
      vertexBufferResourceDesc.SampleDesc.Quality = 0;
      vertexBufferResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
      vertexBufferResourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

      ReturnIfFailed(m_device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &vertexBufferResourceDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&m_vertexBuffer)));

      // Copy the triangle data to the vertex buffer.
      uint8* pVertexDataBegin = nullptr;

      // We do not intend to read from this resource on the CPU.
      D3D12_RANGE readRange = {};
      readRange.Begin = 0;
      readRange.End = 0;

      ReturnIfFailed(m_vertexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pVertexDataBegin)));
      memcpy(pVertexDataBegin, m_vertexBufferData, sizeof(m_vertexBufferData));
      m_vertexBuffer->Unmap(0, nullptr);

      // Initialize the vertex buffer view.
      m_vertexBufferView.BufferLocation = m_vertexBuffer->GetGPUVirtualAddress();
      m_vertexBufferView.StrideInBytes = sizeof(Vertex);
      m_vertexBufferView.SizeInBytes = vertexBufferSize;
    }

    // Create index buffer
    {
      const uint32 indexBufferSize = sizeof(m_indexBufferData);

      //TODO use GPU memory
      D3D12_HEAP_PROPERTIES heapProps = {};
      heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
      heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
      heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
      heapProps.CreationNodeMask = 1;
      heapProps.VisibleNodeMask = 1;

      D3D12_RESOURCE_DESC indexBufferResourceDesc = {};
      indexBufferResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
      indexBufferResourceDesc.Alignment = 0;
      indexBufferResourceDesc.Width = indexBufferSize;
      indexBufferResourceDesc.Height = 1;
      indexBufferResourceDesc.DepthOrArraySize = 1;
      indexBufferResourceDesc.MipLevels = 1;
      indexBufferResourceDesc.Format = DXGI_FORMAT_UNKNOWN;
      indexBufferResourceDesc.SampleDesc.Count = 1;
      indexBufferResourceDesc.SampleDesc.Quality = 0;
      indexBufferResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
      indexBufferResourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

      ReturnIfFailed(m_device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &indexBufferResourceDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&m_indexBuffer)));

      // Copy the triangle data to the vertex buffer.
      uint8* pVertexDataBegin = nullptr;

      // We do not intend to read from this resource on the CPU.
      D3D12_RANGE readRange = {};
      readRange.Begin = 0;
      readRange.End = 0;

      ReturnIfFailed(m_indexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pVertexDataBegin)));
      memcpy(pVertexDataBegin, m_indexBufferData, sizeof(m_indexBufferData));
      m_indexBuffer->Unmap(0, nullptr);

      // Initialize the index buffer view.
      m_indexBufferView.BufferLocation = m_indexBuffer->GetGPUVirtualAddress();
      m_indexBufferView.Format = DXGI_FORMAT_R32_UINT;
      m_indexBufferView.SizeInBytes = indexBufferSize;
    }

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

    return true;
  }

  bool Renderer::SetupCommands()
  {
    // Command list allocators can only be reset when the associated
    // command lists have finished execution on the GPU; apps should use
    // fences to determine GPU execution progress.
    ReturnIfFailed(m_commandAllocator->Reset());

    // However, when ExecuteCommandList() is called on a particular command
    // list, that command list can then be reset at any time and must be before
    // re-recording.
    ReturnIfFailed(m_commandList->Reset(m_commandAllocator, m_pipelineState));

    // Set necessary state.
    m_commandList->SetGraphicsRootSignature(m_rootSignature);
    m_commandList->RSSetViewports(1, &m_viewport);
    m_commandList->RSSetScissorRects(1, &m_surfaceSize);

    ID3D12DescriptorHeap* pDescriptorHeaps[] = { m_uniformBufferHeap };
    m_commandList->SetDescriptorHeaps(_countof(pDescriptorHeaps), pDescriptorHeaps);

    D3D12_GPU_DESCRIPTOR_HANDLE srvHandle(m_uniformBufferHeap->GetGPUDescriptorHandleForHeapStart());
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
    m_commandList->IASetVertexBuffers(0, 1, &m_vertexBufferView);
    m_commandList->IASetIndexBuffer(&m_indexBufferView);

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

    return true;
  }

  bool Renderer::InitFrameBuffer()
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

    return true;
  }

  bool Renderer::CompileShaders(ID3DBlob** vertexShader, ID3DBlob** pixelShader)
  {
    Log("Compiling shaders...", LogType::LT_INFO);

    ID3DBlob* errors = nullptr;

#ifdef DX12_DEBUG
    // Enable better shader debugging with the graphics debugging tools.
    uint32 compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
    uint32 compileFlags = 0;
#endif

    WString vertCompiledPath = GetShaderPath() + L"triangle.vert.dxbc";
    WString fragCompiledPath = GetShaderPath() + L"triangle.frag.dxbc";

    WString vertPath = GetShaderPath() + L"triangle.vert.hlsl";
    WString fragPath = GetShaderPath() + L"triangle.frag.hlsl";

    HRESULT compileResult = D3DCompileFromFile(vertPath.c_str(), nullptr, nullptr, "main", "vs_5_0", compileFlags, 0, vertexShader, &errors);
    if (!SUCCEEDED(compileResult))
    {
      if (errors == nullptr)
        return false;

      const char* errStr = (const char*)errors->GetBufferPointer();
      Log(errStr, LogType::LT_ERROR);
      errors->Release();
      errors = nullptr;
      return false;
    }
    compileResult = D3DCompileFromFile(fragPath.c_str(), nullptr, nullptr, "main", "ps_5_0", compileFlags, 0, pixelShader, &errors);
    if (!SUCCEEDED(compileResult))
    {
      if (errors == nullptr)
        return false;

      const char* errStr = (const char*)errors->GetBufferPointer();
      Log(errStr, LogType::LT_ERROR);
      errors->Release();
      errors = nullptr;
      return false;
    }

    std::ofstream vsOut(vertCompiledPath, std::ios::out | std::ios::binary);
    std::ofstream fsOut(fragCompiledPath, std::ios::out | std::ios::binary);

    vsOut.write((const char*)(*vertexShader)->GetBufferPointer(), (*vertexShader)->GetBufferSize());
    fsOut.write((const char*)(*pixelShader)->GetBufferPointer(), (*pixelShader)->GetBufferSize());

    Log("Done: Compiling shaders.", LogType::LT_INFO);

    return true;
  }

  bool Renderer::CreateCommands()
  {
    // Create the command list.
    ReturnIfFailed(m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_commandAllocator, m_pipelineState, IID_PPV_ARGS(&m_commandList)));
    m_commandList->SetName(L"Main Command List");

    return true;
  }

  bool Renderer::SetupSwapchain(uint32 width, uint32 height)
  {
    m_surfaceSize.left = 0;
    m_surfaceSize.top = 0;
    m_surfaceSize.right = (LONG)m_width;
    m_surfaceSize.bottom = (LONG)m_height;

    m_viewport.TopLeftX = 0.0f;
    m_viewport.TopLeftY = 0.0f;
    m_viewport.Width = (float)m_width;
    m_viewport.Height = (float)m_height;
    m_viewport.MinDepth = 0.1f;
    m_viewport.MaxDepth = 1000.f;

    // Update matrices
    uboVS.projectionMatrix = DirectX::XMMatrixPerspectiveLH(1.0f, 1.0f, 0.1f, 1000.0f);

    const Vec3 camPos = Vec3(0.0f, 0.0f, 2.0f);
    uboVS.viewMatrix = DirectX::XMMatrixLookAtLH(XMLoadFloat3(&camPos), XMLoadFloat3(&ZeroVector), XMLoadFloat3(&UpVector));

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
        return false;
      }
    }
    m_frameIndex = m_swapchain->GetCurrentBackBufferIndex();

    return true;
  }

  bool Renderer::DestroyCommands()
  {
    if (m_commandList)
    {
      m_commandList->Reset(m_commandAllocator, m_pipelineState);
      m_commandList->ClearState(m_pipelineState);
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

    return true;
  }

  bool Renderer::DestroyFrameBuffer()
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

    return true;
  }

  bool Renderer::DestroyResources()
  {
    // Sync
    CloseHandle(m_fenceEvent);

    if (m_pipelineState)
    {
      m_pipelineState->Release();
      m_pipelineState = nullptr;
    }

    if (m_rootSignature)
    {
      m_rootSignature->Release();
      m_rootSignature = nullptr;
    }

    if (m_vertexBuffer)
    {
      m_vertexBuffer->Release();
      m_vertexBuffer = nullptr;
    }

    if (m_indexBuffer)
    {
      m_indexBuffer->Release();
      m_indexBuffer = nullptr;
    }

    if (m_uniformBuffer)
    {
      m_uniformBuffer->Release();
      m_uniformBuffer = nullptr;
    }

    if (m_uniformBufferHeap)
    {
      m_uniformBufferHeap->Release();
      m_uniformBufferHeap = nullptr;
    }

    return true;
  }

  bool Renderer::DestroyAPI()
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

#ifdef DX12_DEBUG
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

    return true;
  }

}
