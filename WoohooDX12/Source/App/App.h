#pragma once

#include "MainWindow.h"
#include "Renderer.h"

namespace WoohooDX12
{
  class App
  {
  public:
    App();
    ~App();
    bool Init();
    void Run();

  public:
    MainWindow* m_window = nullptr;
    Renderer* m_renderer = nullptr;
    bool m_quit = false;
  };
}
