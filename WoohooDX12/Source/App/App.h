#pragma once

#include <memory>
#include "MainWindow.h"
#include "WohCore.h"

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
    int UnInit();
    int InitScene();

  public:
    std::shared_ptr<MainWindow> m_window = nullptr;
    std::shared_ptr<WohCore> m_core = nullptr;
    std::shared_ptr<Scene> m_scene = nullptr;
    bool m_quit = false;
  };
}
