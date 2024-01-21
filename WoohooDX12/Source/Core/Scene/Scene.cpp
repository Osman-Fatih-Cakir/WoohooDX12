#include "Scene.h"
#include "Utils.h"
#include "PrimitiveEntities.h"

namespace WoohooDX12
{
  Scene::~Scene()
  {
    m_entities.clear();
  }

  void Scene::AddTriangle()
  {
    m_entities.push_back(std::make_shared<TriangleEntity>());
  }
}
