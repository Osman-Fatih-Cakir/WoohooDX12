#include "App.h"

#include "Utils.h"
#include "Graphics/Renderer.h"
#include "SDL.h"

namespace WoohooDX12
{
  App::App()
  {
    m_window = std::make_shared<MainWindow>();
    m_scene = std::make_shared<Scene>();
    m_sceneRenderer = std::make_shared<SceneRenderer>();
  }

  App::~App()
  {
    m_sceneRenderer = nullptr;
    m_window = nullptr;
    m_scene = nullptr;
  }

  int App::Init()
  {
    // Create window
    ReturnIfFailed(m_window->Create(1280, 720));

    InitScene();

    ReturnIfFailed(m_sceneRenderer->Init(1280, 720, m_window->g_hwnd));

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

      if (shouldRender)
      {
        int check = m_sceneRenderer->Render();
        m_quit = m_quit || check < 0;
      }
    }

    m_sceneRenderer->UnInit();
  }

  int App::InitScene()
  {
    m_scene->AddTriangle();

    ReturnIfFailed(m_sceneRenderer->SetScene(m_scene));

    return 0;
  }
}
