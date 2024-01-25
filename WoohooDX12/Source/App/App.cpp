#include "App.h"

#include "Utils.h"
#include "Graphics/Renderer.h"
#include "SDL.h"

namespace WoohooDX12
{
  App::App()
  {
    m_window = std::make_shared<MainWindow>();
    m_core = std::make_shared<WohCore>();
    m_scene = std::make_shared<Scene>();
  }

  App::~App()
  {
    UnInit();

    m_core = nullptr;
    m_scene = nullptr;
    m_window = nullptr;
  }

  int App::Init()
  {
    // Create window
    ReturnIfFailed(m_window->Create(1280, 720));

    InitScene();

    ReturnIfFailed(m_core->Init(1280, 720, m_window->g_hwnd, m_scene));

    return 0;
  }

  void App::Run()
  {
    while (!m_quit)
    {
      bool shouldRender = true;

      SDL_Event sdlEvent;
      while (SDL_PollEvent(&sdlEvent))
      {
        if (sdlEvent.type == SDL_QUIT)
        {
          m_quit = true;
        }
      }

      if (shouldRender && !m_quit)
      {
        // TODO create a class that app can access Renderer via WohCore->GetRenderer()->RenderImGui()
        int check = m_core->Render();

        m_quit = m_quit || check < 0;
      }
    }
  }

  int App::UnInit()
  {
    m_window->Close();
    m_core->UnInit();

    return 0;
  }

  int App::InitScene()
  {
    m_scene->AddTriangle();

    return 0;
  }
}
