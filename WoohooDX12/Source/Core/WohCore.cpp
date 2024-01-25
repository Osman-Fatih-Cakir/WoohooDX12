#include "WohCore.h"
#include "Utils.h"
#include "imgui.h"
#include "backends/imgui_impl_win32.h"
#include "backends/imgui_impl_dx12.h"

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

    ReturnIfFailed(InitImGui(hwnd));

    m_initialized = true;

    return 0;
  }

  int WohCore::Render()
  {
    m_sceneRenderer->Render();
    RenderImGui();
    m_renderer->PresentBackbuffer();

    return 0;
  }

  int WohCore::InitImGui(HWND hwnd)
  {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGui::StyleColorsDark();

    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX12_Init(m_renderer->m_device, m_renderer->m_backbufferCount,
      DXGI_FORMAT_R8G8B8A8_UNORM, m_renderer->m_imguiHeap,
      m_renderer->m_imguiHeap->GetCPUDescriptorHandleForHeapStart(),
      m_renderer->m_imguiHeap->GetGPUDescriptorHandleForHeapStart());

    return 0;
  }

  int WohCore::RenderImGui()
  {
    ImGui_ImplDX12_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    // 2. Show a simple window that we create ourselves. We use a Begin/End pair to create a named window.
    {
      static float f = 0.0f;
      static int counter = 0;
      static bool show_demo_window = true;
      static bool show_another_window = false;

      ImGui::Begin("Hello, world!");                          // Create a window called "Hello, world!" and append into it.

      ImGui::Text("This is some useful text.");               // Display some text (you can use a format strings too)
      ImGui::Checkbox("Demo Window", &show_demo_window);      // Edit bools storing our window open/close state
      ImGui::Checkbox("Another Window", &show_another_window);

      ImGui::SliderFloat("float", &f, 0.0f, 1.0f);            // Edit 1 float using a slider from 0.0f to 1.0f

      if (ImGui::Button("Button"))                            // Buttons return true when clicked (most widgets return true when edited/activated)
        counter++;
      ImGui::SameLine();
      ImGui::Text("counter = %d", counter);

      ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
      ImGui::End();
    }

    ImGui::Render();

    m_renderer->RenderImGui();

    return 0;
  }

  int WohCore::UnInit()
  {
    if (!m_initialized)
      return 0;

    // SceneRenderer Uninits renderer
    m_sceneRenderer->UnInit();

    ImGui_ImplDX12_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    m_initialized = false;

    return 0;
  }
}
