#pragma once

#include <d3d12.h>
#include <dxgi1_3.h>
#include <dxgi1_4.h>
#include <vector>
#include "Types.h"
#include "Defines.h"
#include "../App/MainWindow.h"
#include "Material.h"
#include "Mesh.h"

namespace WoohooDX12
{
  class Renderer
  {
    friend class SceneRenderer;
    friend class WohCore;

  public:
    Renderer();
    ~Renderer();

  protected:

    int Init(uint32 width, uint32 height, HWND hwnd);
    int UnInit(std::vector<std::shared_ptr<Material>>& materials);
    int Resize(uint32 width, uint32 height);

    int Render(std::shared_ptr<Mesh> mesh, std::shared_ptr<Material> material);
    int RenderImGui();
    int PresentBackbuffer();

    int InitAPI();
    int InitResources(std::vector<std::shared_ptr<Material>>& materials);
    int SetupCommands(std::shared_ptr<Mesh> mesh, std::shared_ptr<Material> material);
    int InitFrameBuffer();

    int SetupSwapchain(uint32 width, uint32 height);

    int DestroyAPI();
    int DestroyResources();
    int DestroyCommands(std::vector<std::shared_ptr<Material>>& materials);
    int DestroyFrameBuffer();

  private:
    constexpr static uint32 m_backbufferCount = 2;

    bool m_initialized = false;
    HWND m_hwnd = nullptr; // window handle
    uint32 m_width = 1280;
    uint32 m_height = 720;

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

    // Current Frame
    uint32 m_currentBuffer = 0;
    ID3D12DescriptorHeap* m_rtvHeap = nullptr;
    ID3D12DescriptorHeap* m_imguiHeap = nullptr; // srv cbv uav heap
    ID3D12Resource* m_renderTargets[m_backbufferCount];
    IDXGISwapChain3* m_swapchain = nullptr;

    // Resources
    D3D12_VIEWPORT m_viewport;
    D3D12_RECT m_surfaceSize;

    uint32 m_rtvDescriptorSize;
    uint32 m_imguiDescriptorSize;

    // Sync
    uint32 m_frameIndex;
    HANDLE m_fenceEvent;
    ID3D12Fence* m_fence = nullptr;
    uint64 m_fenceValue;
  };
}
