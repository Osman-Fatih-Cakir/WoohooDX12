#include "SceneRenderer.h"

namespace WoohooDX12
{
  SceneRenderer::SceneRenderer()
  {
    m_scene = std::make_shared<Scene>();
    m_defaultMaterial = std::make_shared<Material>();
  }

  SceneRenderer::~SceneRenderer()
  {
    assert(!m_initialized && "Scene Renderer is uninitialized!");

    m_renderJobs.clear();
    m_scene = nullptr;
    m_defaultMaterial = nullptr;
  }

  int SceneRenderer::Init(std::shared_ptr<Renderer> renderer, uint32 width, uint32 height, HWND hwnd)
  {
    if (m_initialized)
      return -1;

    m_renderer = renderer;
    ReturnIfFailed(m_renderer->Init(width, height, hwnd));
    std::vector<std::shared_ptr<Material>> materials = { m_defaultMaterial };
    ReturnIfFailed(m_renderer->InitResources(materials));

    // Create render job datas
    m_renderJobs[m_defaultMaterial] = std::vector<std::shared_ptr<Mesh>>();

    // Init meshes
    for (std::shared_ptr<Entity> ntt : m_scene->m_entities)
    {
      std::shared_ptr<Material> mat = GetMaterialForEntityType(ntt->GetType());

      ReturnIfFailed(ntt->m_mesh->Init(m_renderer->m_device, mat->m_commandList));
      // Execute upload commands
      ReturnIfFailed(mat->m_commandList->Close());
      ID3D12CommandList* ppCommandLists[] = { mat->m_commandList };
      m_renderer->m_commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

      m_renderJobs[mat].push_back(ntt->m_mesh);
    }

    m_initialized = true;
    return 0;
  }

  int SceneRenderer::UnInit()
  {
    if (!m_initialized)
      return 0;

    for (auto renderJob : m_renderJobs)
    {
      for (std::shared_ptr<Mesh> mesh : renderJob.second)
      {
        mesh->UnInit();
      }
      renderJob.second.clear();
      renderJob.first->UnInit();
    }
    m_renderJobs.clear();

    std::vector<std::shared_ptr<Material>> materials = { m_defaultMaterial };
    m_renderer->UnInit(materials);

    m_initialized = false;
    return 0;
  }

  int SceneRenderer::Resize(uint32 width, uint32 height)
  {
    m_renderer->Resize(width, height);

    return 0;
  }

  int SceneRenderer::Render()
  {
    if (!m_initialized)
      return -1;

    for (auto meshesWithSameMaterial : m_renderJobs)
    {
      std::shared_ptr<Material> mat = meshesWithSameMaterial.first;
      for (std::shared_ptr<Mesh> mesh : meshesWithSameMaterial.second)
      {
        m_renderer->Render(mesh, mat);
      }
    }

    return 0;
  }

  std::shared_ptr<Material> SceneRenderer::GetMaterialForEntityType(EntityType type)
  {
    if (type == EntityType::Primitive)
    {
      return m_defaultMaterial;
    }
    else
    {
      assert(false && "Unkown entity type!");
      return nullptr;
    }
  }
}
