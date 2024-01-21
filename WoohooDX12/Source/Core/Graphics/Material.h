#pragma once

#include <d3d12.h>
#include <dxgi1_3.h>
#include <dxgi1_4.h>
#include "Types.h"

namespace WoohooDX12
{
  class Material
  {
    friend class Renderer;
    friend class SceneRenderer;

  public:
    Material();
    virtual ~Material();

    int Init(ID3D12Device* device, ID3D12CommandAllocator* commandAllocator);
    int UnInit();

    int Update();

  private:
    int CompileShaders(ID3DBlob** vertexShader, ID3DBlob** pixelShader);

  private:
    // Uniform data

    struct UboVS
    {
      // TODO Use MVP matrix
      Mat projectionMatrix;
      Mat viewMatrix;
      Mat modelMatrix;
    };
    UboVS m_uboVS;

    ID3D12Resource* m_uniformBuffer = nullptr;
    ID3D12DescriptorHeap* m_uniformBufferHeap = nullptr;
    UINT8* m_mappedUniformBuffer = nullptr;

    ID3D12RootSignature* m_rootSignature = nullptr;
    ID3D12PipelineState* m_pipelineState = nullptr;
    ID3D12GraphicsCommandList* m_commandList = nullptr;

    bool m_initialized = false;
  };
}
