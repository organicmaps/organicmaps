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
    uint32_t const queueFamilyIndex = vulkanContext->GetRenderingQueueFamilyIndex();
    m_deviceHolder = vulkanContext->GetDeviceHolder();

    auto devicePtr = m_deviceHolder.lock();
    CHECK(devicePtr != nullptr, ());

    m_geometryBuffers.resize(m_mesh->m_buffers.size());
    for (size_t i = 0; i < m_mesh->m_buffers.size(); i++)
    {
      if (m_mesh->m_buffers[i].m_data.empty())
        continue;

      VkBufferCreateInfo info = {};
      info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
      info.pNext = nullptr;
      info.flags = 0;
      info.size = m_mesh->m_buffers[i].m_data.size() * sizeof(m_mesh->m_buffers[i].m_data[0]);
      info.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
      info.usage = VK_SHARING_MODE_EXCLUSIVE;
      info.queueFamilyIndexCount = 1;
      info.pQueueFamilyIndices = &queueFamilyIndex;
      CHECK_VK_CALL(vkCreateBuffer(devicePtr->m_device, &info, nullptr, &m_geometryBuffers[i]));

      VkMemoryRequirements memReqs = {};
      vkGetBufferMemoryRequirements(devicePtr->m_device, m_geometryBuffers[i], &memReqs);
      LOG(LINFO, ("MESH OBJ. memReqs.size =", memReqs.size, "alignment =", memReqs.alignment));
    }
  }

  void Reset() override
  {
    auto devicePtr = m_deviceHolder.lock();
    if (devicePtr != nullptr)
    {
      for (auto b : m_geometryBuffers)
        vkDestroyBuffer(devicePtr->m_device, b, nullptr);
    }
    m_geometryBuffers.clear();
  }

  void UpdateBuffer(uint32_t bufferInd) override
  {
    CHECK_LESS(bufferInd, static_cast<uint32_t>(m_geometryBuffers.size()), ());

    auto & buffer = m_mesh->m_buffers[bufferInd];
    CHECK(!buffer.m_data.empty(), ());

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
  DeviceHolderPtr m_deviceHolder;
  std::vector<VkBuffer> m_geometryBuffers;
};
}  // namespace vulkan

void MeshObject::InitForVulkan()
{
  m_impl = make_unique_dp<vulkan::VulkanMeshObjectImpl>(make_ref(this));
}
}  // namespace dp
