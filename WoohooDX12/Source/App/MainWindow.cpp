#include "MainWindow.h"

#include "Utils.h"
#include "SDL.h"

namespace WoohooDX12
{
  int MainWindow::Create(int width, int height)
  {
    m_width = width;
    m_height = height;

    // Register the window class.
    SDL_Window* window = SDL_CreateWindow("WoohooDX12", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width, height, 0);
    g_hwnd = GetActiveWindow();
    if (g_hwnd == 0)
      return -1;

    return 0;
  }

  int MainWindow::Close()
  {
    return 0;
  }
}
