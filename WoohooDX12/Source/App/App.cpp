#include "App.h"

#include "Utils.h"
#include "Graphics/Renderer.h"

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

    ReturnIfFailed(m_sceneRenderer->Init(1280, 720, m_window->m_window));
    
    return 0;
  }

  void App::Run()
  {
    while (!m_quit)
    {
      bool shouldRender = true;

      m_window->m_eventQueue->update();

      while (!m_window->m_eventQueue->empty())
      {
        const xwin::Event& event = m_window->m_eventQueue->front();

        if (event.type == xwin::EventType::Resize)
        {
          const xwin::ResizeData& data = event.data.resize;
          m_sceneRenderer->Resize(data.width, data.height);
          shouldRender = false;
        }

        if (event.type == xwin::EventType::Close)
        {
          m_window->Close();
          shouldRender = false;
          m_quit = true;
        }

        m_window->m_eventQueue->pop();
      }

      if (shouldRender)
      {
        int check = m_sceneRenderer->Render();
        m_quit = check < 0 ? true : false;
      }
    }

    m_sceneRenderer->UnInit();
  }

  int App::InitScene()
  {
    m_scene->AddTriangle();

    ReturnIfFailed(m_sceneRenderer->SetScene(m_scene));
  }
}
