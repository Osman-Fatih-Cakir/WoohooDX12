#pragma once

#include <d3d12.h>
#include <dxgi1_3.h>
#include <dxgi1_4.h>
#include "Types.h"
#include "Material.h"

namespace WoohooDX12
{
  struct Vertex
  {
    float position[3];
    float color[3];
  };

  class Mesh
  {
    friend class Renderer;

  public:
    // mesh layout
    // vertex buffers

    Mesh() {}
    virtual ~Mesh();

    int Init(ID3D12Device* device, ID3D12GraphicsCommandList* commandList);
    int UnInit();

  private:
    Vertex m_vertexBufferData[3] =
    {
      {{0.0f, 1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}},
      {{0.5f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}},
      {{-0.5f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}}
    };

    uint32 m_indexBufferData[3] = { 0, 1, 2 };

    ID3D12Resource* m_uploadVertexBuffer = nullptr;
    ID3D12Resource* m_vertexBuffer = nullptr; // On Video memory
    ID3D12Resource* m_uploadIndexBuffer = nullptr;
    ID3D12Resource* m_indexBuffer = nullptr; // On Video memory

    D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView;
    D3D12_INDEX_BUFFER_VIEW m_indexBufferView;

    bool m_initialized = false;

    //TODO later: let mesh hold its type
  };
}
