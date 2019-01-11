#include "drape/vulkan/vulkan_mesh_object_impl.hpp"

#include "drape/vulkan/vulkan_base_context.hpp"
#include "drape/vulkan/vulkan_utils.hpp"

namespace dp
{
namespace vulkan
{
void VulkanMeshObjectImpl::Build(ref_ptr<dp::GraphicsContext> context, ref_ptr<dp::GpuProgram> program)
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
    info.usage = VK_SHARING_MODE_CONCURRENT;
    info.queueFamilyIndexCount = 1;
    info.pQueueFamilyIndices = &queueFamilyIndex;
    CHECK_VK_CALL(vkCreateBuffer(devicePtr->m_device, &info, nullptr, &m_geometryBuffers[i]));


  }
}

void VulkanMeshObjectImpl::Reset()
{
  auto devicePtr = m_deviceHolder.lock();
  CHECK(devicePtr != nullptr, ());

  for (auto b : m_geometryBuffers)
    vkDestroyBuffer(devicePtr->m_device, b, nullptr);

  m_geometryBuffers.clear();
}

void VulkanMeshObjectImpl::UpdateBuffer(uint32_t bufferInd)
{
  CHECK_LESS(bufferInd, static_cast<uint32_t>(m_geometryBuffers.size()), ());

  auto & buffer = m_mesh->m_buffers[bufferInd];
  CHECK(!buffer.m_data.empty(), ());

//  uint8_t * bufferPointer = (uint8_t *)[m_geometryBuffers[bufferInd] contents];
//  auto const sizeInBytes = buffer.m_data.size() * sizeof(buffer.m_data[0]);
//  memcpy(bufferPointer, buffer.m_data.data(), sizeInBytes);
}

void VulkanMeshObjectImpl::DrawPrimitives(ref_ptr<dp::GraphicsContext> context, uint32_t verticesCount)
{
  //TODO (@rokuz, @darina): Implement.
  CHECK(false, ());
}
}  // namespace vulkan
}  // namespace dp
