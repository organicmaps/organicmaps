#include "drape/instancing.hpp"
#include "drape/vulkan/vulkan_base_context.hpp"
#include "drape/vulkan/vulkan_param_descriptor.hpp"

#include "base/assert.hpp"

namespace dp
{
class VulkanInstancingImpl : public InstancingImpl
{
public:
  explicit VulkanInstancingImpl(ref_ptr<dp::vulkan::VulkanBaseContext> vulkanContext)
    : m_descriptorUpdater(vulkanContext->GetObjectManager())
  {}

  ~VulkanInstancingImpl() override { m_descriptorUpdater.Destroy(); }

  void DrawInstancedTriangleStrip(ref_ptr<dp::GraphicsContext> context, uint32_t instanceCount,
                                  uint32_t verticesCount) override
  {
    ref_ptr<dp::vulkan::VulkanBaseContext> vulkanContext = context;
    VkCommandBuffer commandBuffer = vulkanContext->GetCurrentRenderingCommandBuffer();
    CHECK(commandBuffer != nullptr, ("Vulkan command buffer is required"));

    vulkanContext->SetPrimitiveTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP);
    vulkanContext->SetBindingInfo(BindingInfoArray{}, 0);

    m_descriptorUpdater.Update(context);
    auto descriptorSet = m_descriptorUpdater.GetDescriptorSet();

    uint32_t dynamicOffset = vulkanContext->GetCurrentDynamicBufferOffset();
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vulkanContext->GetCurrentPipelineLayout(),
                            0, 1, &descriptorSet, 1, &dynamicOffset);

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vulkanContext->GetCurrentPipeline());
    vkCmdDraw(commandBuffer, verticesCount, instanceCount, 0, 0);
  }

private:
  vulkan::ParamDescriptorUpdater m_descriptorUpdater;
};

std::unique_ptr<InstancingImpl> CreateVulkanInstancingImpl(ref_ptr<dp::GraphicsContext> context)
{
  return std::make_unique<VulkanInstancingImpl>(context);
}
}  // namespace dp
