#pragma once

#include <d3d12.h>
#include <dxgi1_3.h>
#include <dxgi1_4.h>
#include "Types.h"
#include "Defines.h"
#include "../App/MainWindow.h"
#include "Material.h"
#include "Mesh.h"

namespace WoohooDX12
{
  class Renderer
  {
  public:
    Renderer();
    ~Renderer();

    bool Init(uint32 width, uint32 height, void* windowPtr);
    bool UnInit();
    bool Resize(uint32 width, uint32 height);

    bool Render();

  protected:
    bool InitAPI();
    bool InitResources();
    bool SetupCommands();
    bool InitFrameBuffer();
    bool CreateCommands(ID3D12PipelineState* pipelineState);

    bool SetupSwapchain(uint32 width, uint32 height);

    bool DestroyAPI();
    bool DestroyResources();
    bool DestroyCommands();
    bool DestroyFrameBuffer();

  private:
    constexpr static uint32 m_backbufferCount = 2;

    void* m_window = nullptr; // This is needed for swapchain creation
    uint32 m_width = 1280;
    uint32 m_height = 720;

    std::shared_ptr<Mesh> m_mesh = nullptr;
    std::shared_ptr<Material> m_material = nullptr;

    // Graphics API structures
    IDXGIFactory4* m_factory = nullptr;
    IDXGIAdapter1* m_adapter = nullptr;
#ifdef DX12_DEBUG_LAYER
    ID3D12Debug1* m_debugController = nullptr;
    ID3D12DebugDevice* m_debugDevice = nullptr;
#endif

    ID3D12Device* m_device = nullptr;
    ID3D12CommandQueue* m_commandQueue = nullptr;
    ID3D12CommandAllocator* m_commandAllocator = nullptr;
    ID3D12GraphicsCommandList* m_commandList = nullptr;

    // Current Frame
    uint32 m_currentBuffer = 0;
    ID3D12DescriptorHeap* m_rtvHeap = nullptr;
    ID3D12Resource* m_renderTargets[m_backbufferCount];
    IDXGISwapChain3* m_swapchain = nullptr;

    // Resources
    D3D12_VIEWPORT m_viewport;
    D3D12_RECT m_surfaceSize;

    uint32 m_rtvDescriptorSize;

    // Sync
    uint32 m_frameIndex;
    HANDLE m_fenceEvent;
    ID3D12Fence* m_fence = nullptr;
    uint64 m_fenceValue;
  };
}
