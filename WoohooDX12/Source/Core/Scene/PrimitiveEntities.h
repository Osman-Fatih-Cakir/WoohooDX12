#pragma once

#include "Entity.h"

namespace WoohooDX12
{
  class TriangleEntity : public Entity
  {
  public:
    // TODO Create triangle mesh when types of meshes are implmented
    TriangleEntity() { m_mesh = std::make_shared<Mesh>(); }
    virtual ~TriangleEntity() {}
  };
}
