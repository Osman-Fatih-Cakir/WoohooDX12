#include "WohCore.h"
#include "Utils.h"

namespace WoohooDX12
{
  WohCore::WohCore()
  {
    m_renderer = std::make_shared<Renderer>();
    m_sceneRenderer = std::make_shared<SceneRenderer>();
  }

  WohCore::~WohCore()
  {
    assert(m_initialized == false && "WohCore is not uninitialized!");
  }

  int WohCore::Init(uint32 width, uint32 height, HWND hwnd, std::shared_ptr<Scene> scene)
  {
    if (m_initialized)
      return -1;

    // TODO init renderer here. Let materials be initiailized after renderer initialized.
    // So no need to call Renderer::InitResources with materials.

    ReturnIfFailed(m_sceneRenderer->SetScene(scene));

    ReturnIfFailed(m_sceneRenderer->Init(m_renderer, width, height, hwnd));

    m_initialized = true;

    return 0;
  }

  int WohCore::Render()
  {
    ReturnIfFailed(m_sceneRenderer->Render());
    ReturnIfFailed(m_renderer->RenderImGui());

    return 0;
  }

  int WohCore::UnInit()
  {
    if (!m_initialized)
      return 0;

    // SceneRenderer Uninits renderer
    m_sceneRenderer->UnInit();

    m_initialized = false;

    return 0;
  }
}
