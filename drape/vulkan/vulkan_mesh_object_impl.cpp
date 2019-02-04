#include "drape/mesh_object.hpp"
#include "drape/pointers.hpp"
#include "drape/vulkan/vulkan_base_context.hpp"
#include "drape/vulkan/vulkan_staging_buffer.hpp"
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
    VkDevice device = vulkanContext->GetDevice();

    m_geometryBuffers.resize(m_mesh->m_buffers.size());
    for (size_t i = 0; i < m_mesh->m_buffers.size(); i++)
    {
      if (m_mesh->m_buffers[i].m_data.empty())
        continue;

      auto const sizeInBytes = static_cast<uint32_t>(m_mesh->m_buffers[i].m_data.size() *
                                                     sizeof(m_mesh->m_buffers[i].m_data[0]));
      m_geometryBuffers[i] = m_objectManager->CreateBuffer(VulkanMemoryManager::ResourceType::Geometry,
                                                           sizeInBytes, 0 /* batcherHash */);
      void * gpuPtr = m_objectManager->Map(m_geometryBuffers[i]);
      memcpy(gpuPtr, m_mesh->m_buffers[i].m_data.data(), sizeInBytes);
      m_objectManager->Flush(m_geometryBuffers[i]);
      m_objectManager->Unmap(m_geometryBuffers[i]);

      CHECK_VK_CALL(vkBindBufferMemory(device, m_geometryBuffers[i].m_buffer,
                                       m_geometryBuffers[i].GetMemory(),
                                       m_geometryBuffers[i].GetAlignedOffset()));
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

    ref_ptr<dp::vulkan::VulkanBaseContext> vulkanContext = context;
    VkCommandBuffer commandBuffer = vulkanContext->GetCurrentCommandBuffer();
    CHECK(commandBuffer != nullptr, ());

    auto & buffer = m_mesh->m_buffers[bufferInd];
    CHECK(!buffer.m_data.empty(), ());

    // Copy to default or temporary staging buffer.
    auto const sizeInBytes = static_cast<uint32_t>(buffer.m_data.size() * sizeof(buffer.m_data[0]));
    auto stagingBuffer = m_objectManager->GetDefaultStagingBuffer();
    if (stagingBuffer->HasEnoughSpace(sizeInBytes))
    {
      auto staging = stagingBuffer->Reserve(sizeInBytes);
      memcpy(staging.m_pointer, buffer.m_data.data(), sizeInBytes);

      // Schedule command to copy from the staging buffer to our geometry buffer.
      VkBufferCopy copyRegion = {};
      copyRegion.dstOffset = 0;
      copyRegion.srcOffset = staging.m_offset;
      copyRegion.size = sizeInBytes;
      vkCmdCopyBuffer(commandBuffer, staging.m_stagingBuffer, m_geometryBuffers[bufferInd].m_buffer,
                      1, &copyRegion);
    }
    else
    {
      // Here we use temporary staging object, which will be destroyed after the nearest
      // command queue submitting.
      VulkanStagingBuffer tempStagingBuffer(m_objectManager, sizeInBytes);
      CHECK(tempStagingBuffer.HasEnoughSpace(sizeInBytes), ());
      auto staging = tempStagingBuffer.Reserve(sizeInBytes);
      memcpy(staging.m_pointer, buffer.m_data.data(), sizeInBytes);
      tempStagingBuffer.Flush();

      // Schedule command to copy from the staging buffer to our geometry buffer.
      VkBufferCopy copyRegion = {};
      copyRegion.dstOffset = 0;
      copyRegion.srcOffset = staging.m_offset;
      copyRegion.size = sizeInBytes;
      vkCmdCopyBuffer(commandBuffer, staging.m_stagingBuffer, m_geometryBuffers[bufferInd].m_buffer,
                      1, &copyRegion);
    }

    // Set up a barrier to prevent data collisions.
    VkBufferMemoryBarrier barrier = {};
    barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
    barrier.pNext = nullptr;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.buffer = m_geometryBuffers[bufferInd].m_buffer;
    barrier.offset = 0;
    barrier.size = sizeInBytes;
    vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT,
                         VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, 0, 0, nullptr,
                         1, &barrier, 0, nullptr);
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
