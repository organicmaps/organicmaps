#include "drape/vulkan/vulkan_base_context.hpp"

#include "drape/framebuffer.hpp"

#include "base/assert.hpp"

namespace dp
{
namespace metal
{
VulkanBaseContext::VulkanBaseContext(m2::PointU const & screenSize)
{
}

void VulkanBaseContext::SetStencilReferenceValue(uint32_t stencilReferenceValue) override
{
  m_stencilReferenceValue = stencilReferenceValue;
}
}  // namespace vulkan
}  // namespace dp
