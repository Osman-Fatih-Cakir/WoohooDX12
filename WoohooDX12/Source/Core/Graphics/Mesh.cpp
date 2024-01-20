#include "Mesh.h"

#include "Utils.h"
#include "d3dx12.h"

namespace WoohooDX12
{
  int Mesh::Init(ID3D12Device* device, ID3D12GraphicsCommandList* commandList)
  {
    AssertAndReturn(!m_initialized, "This mesh is already initialized.");

    // Create vertex buffer
    {
      const uint32 vertexBufferSize = sizeof(m_vertexBufferData);

      // Upload heap buffer to upload vertex data to gpu mem
      D3D12_HEAP_PROPERTIES uploadheapProps = {};
      uploadheapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
      uploadheapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
      uploadheapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
      uploadheapProps.CreationNodeMask = 1;
      uploadheapProps.VisibleNodeMask = 1;

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

      ReturnIfFailed(device->CreateCommittedResource(&uploadheapProps, D3D12_HEAP_FLAG_NONE, &vertexBufferResourceDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&m_uploadVertexBuffer)));

      // default heap holds vertex buffer
      D3D12_HEAP_PROPERTIES defaultheapProps = {};
      defaultheapProps.Type = D3D12_HEAP_TYPE_DEFAULT;
      defaultheapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
      defaultheapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
      defaultheapProps.CreationNodeMask = 1;
      defaultheapProps.VisibleNodeMask = 1;

      ReturnIfFailed(device->CreateCommittedResource(&defaultheapProps, D3D12_HEAP_FLAG_NONE, &vertexBufferResourceDesc,
        D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, nullptr, IID_PPV_ARGS(&m_vertexBuffer)));

      // Initialize the vertex buffer view.
      m_vertexBufferView.BufferLocation = m_vertexBuffer->GetGPUVirtualAddress();
      m_vertexBufferView.StrideInBytes = sizeof(Vertex);
      m_vertexBufferView.SizeInBytes = vertexBufferSize;

      D3D12_SUBRESOURCE_DATA vertexData = {};
      vertexData.pData = m_vertexBufferData;
      vertexData.RowPitch = sizeof(m_vertexBufferData);
      vertexData.SlicePitch = 0;

      // upload vertex data to gpu memory
      const CD3DX12_RESOURCE_BARRIER firstBarrier = CD3DX12_RESOURCE_BARRIER::Transition(m_vertexBuffer, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER,
        D3D12_RESOURCE_STATE_COPY_DEST);
      const CD3DX12_RESOURCE_BARRIER secondBarrier = CD3DX12_RESOURCE_BARRIER::Transition(m_vertexBuffer, D3D12_RESOURCE_STATE_COPY_DEST,
        D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);

      commandList->ResourceBarrier(1, &firstBarrier);
      UpdateSubresources(commandList, m_vertexBuffer, m_uploadVertexBuffer, 0, 0, 1, &vertexData);
      commandList->ResourceBarrier(1, &secondBarrier);
    }

    // Create index buffer
    {
      const uint32 indexBufferSize = sizeof(m_indexBufferData);

      D3D12_HEAP_PROPERTIES uploadHeapProps = {};
      uploadHeapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
      uploadHeapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
      uploadHeapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
      uploadHeapProps.CreationNodeMask = 1;
      uploadHeapProps.VisibleNodeMask = 1;

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

      ReturnIfFailed(device->CreateCommittedResource(&uploadHeapProps, D3D12_HEAP_FLAG_NONE, &indexBufferResourceDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&m_uploadIndexBuffer)));

      D3D12_HEAP_PROPERTIES defaultHeapProps = {};
      defaultHeapProps.Type = D3D12_HEAP_TYPE_DEFAULT;
      defaultHeapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
      defaultHeapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
      defaultHeapProps.CreationNodeMask = 1;
      defaultHeapProps.VisibleNodeMask = 1;

      ReturnIfFailed(device->CreateCommittedResource(&defaultHeapProps, D3D12_HEAP_FLAG_NONE, &indexBufferResourceDesc,
        D3D12_RESOURCE_STATE_INDEX_BUFFER, nullptr, IID_PPV_ARGS(&m_indexBuffer)));

      // Initialize the index buffer view.
      m_indexBufferView.BufferLocation = m_indexBuffer->GetGPUVirtualAddress();
      m_indexBufferView.Format = DXGI_FORMAT_R32_UINT;
      m_indexBufferView.SizeInBytes = indexBufferSize;

      D3D12_SUBRESOURCE_DATA indexData = {};
      indexData.pData = m_indexBufferData;
      indexData.RowPitch = sizeof(m_indexBufferData);
      indexData.SlicePitch = 0;

      // upload index data to gpu memory
      const CD3DX12_RESOURCE_BARRIER firstBarrier = CD3DX12_RESOURCE_BARRIER::Transition(m_indexBuffer, D3D12_RESOURCE_STATE_INDEX_BUFFER,
        D3D12_RESOURCE_STATE_COPY_DEST);
      const CD3DX12_RESOURCE_BARRIER secondBarrier = CD3DX12_RESOURCE_BARRIER::Transition(m_indexBuffer, D3D12_RESOURCE_STATE_COPY_DEST,
        D3D12_RESOURCE_STATE_INDEX_BUFFER);

      commandList->ResourceBarrier(1, &firstBarrier);
      UpdateSubresources(commandList, m_indexBuffer, m_uploadIndexBuffer, 0, 0, 1, &indexData);
      commandList->ResourceBarrier(1, &secondBarrier);
    }

    return 0;
  }

  int Mesh::UnUnit()
  {
    if (!m_initialized)
      return 0;

    if (m_uploadVertexBuffer)
    {
      m_uploadVertexBuffer->Release();
      m_uploadVertexBuffer = nullptr;
    }

    if (m_uploadIndexBuffer)
    {
      m_uploadIndexBuffer->Release();
      m_uploadIndexBuffer = nullptr;
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

    return 0;
  }
}
