#pragma once

#include <memory>
#include "Renderer.h"
#include "SceneRenderer.h"
#include "Scene/Scene.h"

namespace WoohooDX12
{
  class WohCore
  {
  public:
    WohCore();
    ~WohCore();

    int Init(uint32 width, uint32 height, HWND hwnd, std::shared_ptr<Scene> scene);
    int UnInit();

    int Render();

  private:
    int InitImGui(HWND hwnd);
    int RenderImGui();

  private:
    bool m_initialized = false;

    std::shared_ptr<SceneRenderer> m_sceneRenderer = nullptr;
    std::shared_ptr<Renderer> m_renderer = nullptr;
  };
}
