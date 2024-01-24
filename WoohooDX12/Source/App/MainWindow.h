#pragma once

#include <Windows.h>

namespace WoohooDX12
{
  class MainWindow
  {
  public:
    int Create(int width, int height);
    int Close();

    inline int GetWidth() { return m_width; }
    inline int GetHeight() { return m_height; }

  public:
    HWND g_hwnd = 0;

  private:
    int m_width = 1280;
    int m_height = 720;
  };
}
