#pragma once

#include <memory>
#include "MainWindow.h"
#include "SceneRenderer.h"

namespace WoohooDX12
{
  class App
  {
  public:
    App();
    ~App();
    int Init();
    void Run();

  private:
    int InitScene();

  public:
    std::shared_ptr<MainWindow> m_window = nullptr;
    std::shared_ptr<Scene> m_scene = nullptr;
    std::shared_ptr<SceneRenderer> m_sceneRenderer = nullptr;
    bool m_quit = false;
  };
}
