#pragma once

#include "CrossWindow/CrossWindow.h"

namespace WoohooDX12
{
  class MainWindow
  {
  public:
    int Create(int width, int height);
    int Close();

    inline xwin::WindowDesc GetDesc() { return m_window->getDesc(); }
    inline int GetWidth() { return m_width; }
    inline int GetHeight() { return m_height; }

  public:
    xwin::Window* m_window = nullptr;
    xwin::EventQueue* m_eventQueue = nullptr;

  private:
    int m_width = 1280;
    int m_height = 720;
  };
}
