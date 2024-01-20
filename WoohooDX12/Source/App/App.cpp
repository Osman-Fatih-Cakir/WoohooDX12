#include "App.h"

#include "Utils.h"
#include "Graphics/Renderer.h"

namespace WoohooDX12
{
  App::App()
  {
    m_window = new MainWindow();
    m_renderer = new Renderer();
  }

  App::~App()
  {
    SafeDel(m_renderer);
    SafeDel(m_window);
  }

  int App::Init()
  {
    // Create window
    ReturnIfFailed(m_window->Create(1280, 720));

    ReturnIfFailed(m_renderer->Init(1280, 720, m_window->m_window));

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
          m_renderer->Resize(data.width, data.height);
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

      //TODO App Update

      if (shouldRender)
      {
        m_renderer->Render();
      }
    }
  }
}
