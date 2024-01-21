#pragma once

// entity class creates and holds the mesh
// different type of entities will be there:
// - terrain (an entity responsible for whole terrain)
// - grass (an entity responsible for whole grasses)
// - cube
// - sphere
// - etc.

#include  <memory>
#include "Mesh.h"

namespace WoohooDX12
{
  enum class EntityType
  {
    Primitive,
    //TODO grass, terrain, etc.
  };

  class Entity
  {
    friend class SceneRenderer;

  public:
    virtual ~Entity() { m_mesh = nullptr; }

    inline virtual const EntityType GetType() { return EntityType::Primitive; }

  protected:
    Entity() {}

  protected:
    std::shared_ptr<Mesh> m_mesh = nullptr;
  };
}
