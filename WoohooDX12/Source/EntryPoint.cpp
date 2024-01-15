#include <windows.h>
#include <memory>
#include "App/App.h"

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR pCmdLine, int nCmdShow)
{
  WoohooDX12::App app;
  while (!app.m_quit)
  {
    if (!app.Init())
      break;

    app.Run();
  }

  return 0;
}
