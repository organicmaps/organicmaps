#include "drape/pointers.hpp"
#include "drape/vertex_array_buffer.hpp"
#include "drape/vulkan/vulkan_base_context.hpp"
#include "drape/vulkan/vulkan_gpu_buffer_impl.hpp"

#include "base/assert.hpp"
#include "base/macros.hpp"

#include <vulkan_wrapper.h>
#include <vulkan/vulkan.h>

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <utility>
#include <vector>

namespace dp
{
namespace vulkan
{
class VulkanVertexArrayBufferImpl : public VertexArrayBufferImpl
{
public:
  explicit VulkanVertexArrayBufferImpl(ref_ptr<VertexArrayBuffer> buffer)
    : m_vertexArrayBuffer(buffer)
  {}
  
  bool Build(ref_ptr<GpuProgram> program) override
  {
    UNUSED_VALUE(program);
    return true;
  }
  
  bool Bind() override { return true; }
  void Unbind() override {}
  void BindBuffers(dp::BuffersMap const & buffers) const override {}

  void AddBindingInfo(dp::BindingInfo const & bindingInfo) override
  {
    auto const id = bindingInfo.GetID();
    auto const it = std::find_if(m_bindingInfo.begin(), m_bindingInfo.end(),
                                 [id](dp::BindingInfo const & info)
    {
      return info.GetID() == id;
    });
    if (it != m_bindingInfo.end())
    {
      CHECK(*it == bindingInfo, ("Incorrect binding info."));
      return;
    }

    m_bindingInfo.push_back(bindingInfo);
    std::sort(m_bindingInfo.begin(), m_bindingInfo.end(),
              [](dp::BindingInfo const & info1, dp::BindingInfo const & info2)
    {
      return info1.GetID() < info2.GetID();
    });
  }
  
  void RenderRange(ref_ptr<GraphicsContext> context, bool drawAsLine,
                   IndicesRange const & range) override
  {
    ref_ptr<dp::vulkan::VulkanBaseContext> vulkanContext = context;
    VkCommandBuffer commandBuffer = vulkanContext->GetCurrentCommandBuffer();
    CHECK(commandBuffer != nullptr, ());

    if (!m_pipeline || (m_lastDrawAsLine != drawAsLine))
    {
      m_lastDrawAsLine = drawAsLine;

      vulkanContext->SetPrimitiveTopology(drawAsLine ? VK_PRIMITIVE_TOPOLOGY_LINE_LIST :
                                                       VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
      vulkanContext->SetBindingInfo(m_bindingInfo);
      m_pipeline = vulkanContext->GetCurrentPipeline();
      if (!m_pipeline)
        return;
    }

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline);

    VkDeviceSize offsets[1] = {0};
    uint32_t bufferIndex = 0;
    for (auto & buffer : m_vertexArrayBuffer->m_staticBuffers)
    {
      ref_ptr<VulkanGpuBufferImpl> b = buffer.second->GetBuffer();
      VkBuffer vulkanBuffer = b->GetVulkanBuffer();
      vkCmdBindVertexBuffers(commandBuffer, bufferIndex, 1, &vulkanBuffer, offsets);
      bufferIndex++;
    }
    for (auto & buffer : m_vertexArrayBuffer->m_dynamicBuffers)
    {
      ref_ptr<VulkanGpuBufferImpl> b = buffer.second->GetBuffer();
      VkBuffer vulkanBuffer = b->GetVulkanBuffer();
      vkCmdBindVertexBuffers(commandBuffer, bufferIndex, 1, &vulkanBuffer, offsets);
      bufferIndex++;
    }

    ref_ptr<VulkanGpuBufferImpl> ib = m_vertexArrayBuffer->m_indexBuffer->GetBuffer();
    VkBuffer vulkanIndexBuffer = ib->GetVulkanBuffer();
    auto const indexType = dp::IndexStorage::IsSupported32bit() ? VK_INDEX_TYPE_UINT32 : VK_INDEX_TYPE_UINT16;
    vkCmdBindIndexBuffer(commandBuffer, vulkanIndexBuffer, 0, indexType);

    vkCmdDrawIndexed(commandBuffer, range.m_idxCount, 1, range.m_idxStart, 0, 0);

    vulkanContext->ClearParamDescriptors();
  }
  
private:
  ref_ptr<VertexArrayBuffer> m_vertexArrayBuffer;
  std::vector<dp::BindingInfo> m_bindingInfo;
  VkPipeline m_pipeline = {};
  bool m_lastDrawAsLine = false;
};
}  // namespace vulkan
  
drape_ptr<VertexArrayBufferImpl> VertexArrayBuffer::CreateImplForVulkan(ref_ptr<VertexArrayBuffer> buffer)
{
  return make_unique_dp<vulkan::VulkanVertexArrayBufferImpl>(buffer);
}
}  // namespace dp
