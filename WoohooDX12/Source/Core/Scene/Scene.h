#pragma once

#include <memory>
#include <vector>
#include "Entity.h"

// Scene class holds the entities in a scene

namespace WoohooDX12
{
  class Scene
  {
    friend class SceneRenderer;

  public:
    Scene() {}
    ~Scene();

    void AddTriangle();

  private:
    // simple meshes
    std::vector<std::shared_ptr<Entity>> m_entities;
  };
}
