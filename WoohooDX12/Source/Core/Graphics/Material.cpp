#include "Material.h"

#include <fstream>
#include <d3dcompiler.h>
#include "Maths.h"
#include "Utils.h"

namespace WoohooDX12
{
  Material::Material()
  {
    m_uboVS.projectionMatrix = DirectX::XMMatrixPerspectiveLH(1.0f, 1.0f, 0.1f, 1000.0f);

    const Vec3 camPos = Vec3(0.0f, 0.0f, 2.0f);
    m_uboVS.viewMatrix = DirectX::XMMatrixLookAtLH(XMLoadFloat3(&camPos), XMLoadFloat3(&ZeroVector), XMLoadFloat3(&UpVector));

    MakeIdentity(m_uboVS.modelMatrix);
  }

  int Material::Init(ID3D12Device* device)
  {
    AssertAndReturn(!m_initialized, "This material is already initialized.");

    // Create root signature
    {
      D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData = {};

      // This is the highest version the sample supports. If CheckFeatureSupport succeeds,
      // the HighestVersion returned will not be greater than this.
      featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;

      if (FAILED(device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &featureData, sizeof(featureData))))
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
        ThrowIfFailed(D3D12SerializeVersionedRootSignature(&rootSignatureDesc, &signature, &error));
        ThrowIfFailed(device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_rootSignature)));
        m_rootSignature->SetName(L"Main Root Signature");
      }
      catch (std::exception e)
      {
        const char* errStr = (const char*)error->GetBufferPointer();
        Log(errStr, LogType::LT_ERROR);
        error->Release();
        error = nullptr;
        return -1;
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
      ReturnIfFailed(CompileShaders(&vertexShader, &pixelShader));

      // Define the vertex input layout
      D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
      {
          {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
          {"COLOR", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}
      };

      // Create the UBO

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
      ReturnIfFailed(device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&m_uniformBufferHeap)));

      D3D12_RESOURCE_DESC uboResourceDesc = {};
      uboResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
      uboResourceDesc.Alignment = 0;
      uboResourceDesc.Width = (sizeof(m_uboVS) + 255) & ~255;
      uboResourceDesc.Height = 1;
      uboResourceDesc.DepthOrArraySize = 1;
      uboResourceDesc.MipLevels = 1;
      uboResourceDesc.Format = DXGI_FORMAT_UNKNOWN;
      uboResourceDesc.SampleDesc.Count = 1;
      uboResourceDesc.SampleDesc.Quality = 0;
      uboResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
      uboResourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

      ReturnIfFailed(device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &uboResourceDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&m_uniformBuffer)));
      m_uniformBufferHeap->SetName(L"Constant Buffer Upload Resource Heap");

      D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
      cbvDesc.BufferLocation = m_uniformBuffer->GetGPUVirtualAddress();
      cbvDesc.SizeInBytes = (sizeof(m_uboVS) + 255) & ~255; // CB size is required to be 256-byte aligned

      D3D12_CPU_DESCRIPTOR_HANDLE cbvHandle(m_uniformBufferHeap->GetCPUDescriptorHandleForHeapStart());
      cbvHandle.ptr += device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV) * 0;

      device->CreateConstantBufferView(&cbvDesc, cbvHandle);

      // We do not intend to read from this resource on the CPU. (End is less than or equal to begin)
      D3D12_RANGE readRange = {};
      readRange.Begin = 0;
      readRange.End = 0;

      ReturnIfFailed(m_uniformBuffer->Map(0, &readRange, reinterpret_cast<void**>(&m_mappedUniformBuffer)));
      memcpy(m_mappedUniformBuffer, &m_uboVS, sizeof(m_uboVS));
      m_uniformBuffer->Unmap(0, &readRange);

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
        ReturnIfFailed(device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pipelineState)));
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

    return 0;
  }

  int Material::UnInit()
  {
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

    return 0;
  }

  int Material::CompileShaders(ID3DBlob** vertexShader, ID3DBlob** pixelShader)
  {
    Log("Compiling shaders...", LogType::LT_INFO);

    ID3DBlob* errors = nullptr;

#ifdef DX12_DEBUG_LAYER
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
        return -1;

      const char* errStr = (const char*)errors->GetBufferPointer();
      Log(errStr, LogType::LT_ERROR);
      errors->Release();
      errors = nullptr;
      return -1;
    }
    compileResult = D3DCompileFromFile(fragPath.c_str(), nullptr, nullptr, "main", "ps_5_0", compileFlags, 0, pixelShader, &errors);
    if (!SUCCEEDED(compileResult))
    {
      if (errors == nullptr)
        return -1;

      const char* errStr = (const char*)errors->GetBufferPointer();
      Log(errStr, LogType::LT_ERROR);
      errors->Release();
      errors = nullptr;
      return -1;
    }

    std::ofstream vsOut(vertCompiledPath, std::ios::out | std::ios::binary);
    std::ofstream fsOut(fragCompiledPath, std::ios::out | std::ios::binary);

    vsOut.write((const char*)(*vertexShader)->GetBufferPointer(), (*vertexShader)->GetBufferSize());
    fsOut.write((const char*)(*pixelShader)->GetBufferPointer(), (*pixelShader)->GetBufferSize());

    Log("Done: Compiling shaders.", LogType::LT_INFO);

    return 0;
  }

  int Material::Update()
  {
    m_uboVS.modelMatrix *= DirectX::XMMatrixRotationAxis(DirectX::XMLoadFloat3(&UpVector), DirectX::XMConvertToRadians(1.0f));

    D3D12_RANGE readRange = {};
    readRange.Begin = 0;
    readRange.End = 0;

    ReturnIfFailed(m_uniformBuffer->Map(0, &readRange, reinterpret_cast<void**>(&m_mappedUniformBuffer)));
    memcpy(m_mappedUniformBuffer, &m_uboVS, sizeof(m_uboVS));
    m_uniformBuffer->Unmap(0, &readRange);

    return 0;
  }
}
