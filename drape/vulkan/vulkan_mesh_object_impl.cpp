#include "drape/mesh_object.hpp"
#include "drape/pointers.hpp"
#include "drape/vulkan/vulkan_base_context.hpp"
#include "drape/vulkan/vulkan_device_holder.hpp"
#include "drape/vulkan/vulkan_utils.hpp"

#include "base/assert.hpp"

#include <vulkan_wrapper.h>
#include <vulkan/vulkan.h>

#include <cstdint>
#include <vector>

namespace dp
{
namespace vulkan
{
class VulkanMeshObjectImpl : public MeshObjectImpl
{
public:
  explicit VulkanMeshObjectImpl(ref_ptr<dp::MeshObject> mesh)
    : m_mesh(std::move(mesh))
  {}

  void Build(ref_ptr<dp::GraphicsContext> context, ref_ptr<dp::GpuProgram> program) override
  {
    ref_ptr<dp::vulkan::VulkanBaseContext> vulkanContext = context;
    m_objectManager = vulkanContext->GetObjectManager();

    m_geometryBuffers.resize(m_mesh->m_buffers.size());
    for (size_t i = 0; i < m_mesh->m_buffers.size(); i++)
    {
      if (m_mesh->m_buffers[i].m_data.empty())
        continue;

      auto const sizeInBytes = static_cast<uint32_t>(m_mesh->m_buffers[i].m_data.size() *
                                                     sizeof(m_mesh->m_buffers[i].m_data[0]));
      m_geometryBuffers[i] = m_objectManager->CreateBuffer(VulkanMemoryManager::ResourceType::Geometry,
                                                           sizeInBytes, 0 /* batcherHash */);
      //TODO: map, copy, unmap, bind.
    }
  }

  void Reset() override
  {
    for (auto const & b : m_geometryBuffers)
      m_objectManager->DestroyObject(b);
    m_geometryBuffers.clear();
  }

  void UpdateBuffer(ref_ptr<dp::GraphicsContext> context, uint32_t bufferInd) override
  {
    CHECK_LESS(bufferInd, static_cast<uint32_t>(m_geometryBuffers.size()), ());

    auto & buffer = m_mesh->m_buffers[bufferInd];
    CHECK(!buffer.m_data.empty(), ());

    //TODO: stage, map, copy, unmap, barrier.

//  uint8_t * bufferPointer = (uint8_t *)[m_geometryBuffers[bufferInd] contents];
//  auto const sizeInBytes = buffer.m_data.size() * sizeof(buffer.m_data[0]);
//  memcpy(bufferPointer, buffer.m_data.data(), sizeInBytes);
  }

  void DrawPrimitives(ref_ptr<dp::GraphicsContext> context, uint32_t verticesCount) override
  {
    //TODO (@rokuz, @darina): Implement.
    CHECK(false, ());
  }

  void Bind(ref_ptr<dp::GpuProgram> program) override {}
  void Unbind() override {}

private:
  ref_ptr<dp::MeshObject> m_mesh;
  ref_ptr<VulkanObjectManager> m_objectManager;
  std::vector<VulkanObject> m_geometryBuffers;
};
}  // namespace vulkan

void MeshObject::InitForVulkan()
{
  m_impl = make_unique_dp<vulkan::VulkanMeshObjectImpl>(make_ref(this));
}
}  // namespace dp
