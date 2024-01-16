#include "Mesh.h"

#include "Utils.h"
#include "RendererUtils.h"

namespace WoohooDX12
{
  bool Mesh::Init(ID3D12Device* device)
  {
    AssertAndReturn(!m_initialized, "This mesh is already initialized.");

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

      ReturnIfFailed(device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &vertexBufferResourceDesc,
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

      ReturnIfFailed(device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &indexBufferResourceDesc,
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

    return true;
  }

  bool Mesh::UnUnit()
  {
    ReturnIfFalse(m_initialized);

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

    return true;
  }
}
