#pragma once

#include "shaders/program_pool.hpp"

#include "drape/graphics_context.hpp"
#include "drape/pointers.hpp"
#include "drape/vulkan/vulkan_gpu_program.hpp"

#include <array>

namespace gpu
{
namespace vulkan
{
class VulkanProgramPool : public ProgramPool
{
public:
  explicit VulkanProgramPool(ref_ptr<dp::GraphicsContext> context);
  ~VulkanProgramPool() override;

  void Destroy(ref_ptr<dp::GraphicsContext> context);
  drape_ptr<dp::GpuProgram> Get(Program program) override;

  uint32_t GetMaxUniformBuffers() const;
  uint32_t GetMaxImageSamplers() const;

private:
  struct ProgramData
  {
    VkShaderModule m_vertexShader;
    VkShaderModule m_fragmentShader;
    VkDescriptorSetLayout m_descriptorSetLayout;
    VkPipelineLayout m_pipelineLayout;
    dp::vulkan::VulkanGpuProgram::TextureBindings m_textureBindings;
  };
  std::array<ProgramData, static_cast<size_t>(Program::ProgramsCount)> m_programData;
  uint32_t m_maxImageSamplers = 0;
};
}  // namespace vulkan
}  // namespace gpu
