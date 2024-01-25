#pragma once

#include <memory>
#include <unordered_map>
#include "Scene/Scene.h"
#include "Renderer.h"
#include "Utils.h"

namespace WoohooDX12
{
  /*
  * Scene renderer handles all the rendering jobs like initializing-uninitializing resources, rendering, materials, etc.
  * This class should have one instance and that instance should handle all the rendering. (since all the resources are being handled here
  * if another scene renderer instance uninits the resources, the resources will be gone for the other instance)
  */
  class SceneRenderer
  {
  public:
    SceneRenderer();
    ~SceneRenderer();

    inline int SetScene(std::shared_ptr<Scene> scene) { ReturnIfFailed(UnInit()); m_scene = scene; return 0; }
    int Init(std::shared_ptr<Renderer> renderer, uint32 width, uint32 height32, HWND hwnd);
    int UnInit();
    int Resize(uint32 width, uint32 height);

    int Render();

  private:
    std::shared_ptr<Material> GetMaterialForEntityType(EntityType type);

  private:
    std::shared_ptr<Renderer> m_renderer = nullptr;
    std::shared_ptr<Scene> m_scene = nullptr;
    bool m_initialized = false;

    std::shared_ptr<Material> m_defaultMaterial = nullptr;
    std::unordered_map<std::shared_ptr<Material>, std::vector<std::shared_ptr<Mesh>>> m_renderJobs;
  };
}
