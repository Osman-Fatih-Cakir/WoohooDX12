#include <windows.h>
#include <memory>
#include "App/App.h"

#if WOH_DEBUG
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#endif


int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR pCmdLine, int nCmdShow)
{
#if WOH_DEBUG
  _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

  {
    WoohooDX12::App app;
    while (!app.m_quit)
    {
      if (app.Init() != 0)
        break;

      app.Run();
    }
  }

#if WOH_DEBUG
  _CrtDumpMemoryLeaks();
#endif

  return 0;
}
