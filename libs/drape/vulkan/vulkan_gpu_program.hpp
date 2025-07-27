#pragma once

#include "drape/gpu_program.hpp"

#include "base/visitor.hpp"

#include <vulkan_wrapper.h>

#include <array>
#include <string>

namespace dp
{
namespace vulkan
{
class VulkanGpuProgram : public GpuProgram
{
public:
  using TextureBindings = std::unordered_map<std::string, int8_t>;

  VulkanGpuProgram(std::string const & programName, VkPipelineShaderStageCreateInfo const & vertexShader,
                   VkPipelineShaderStageCreateInfo const & fragmentShader, VkDescriptorSetLayout descriptorSetLayout,
                   VkPipelineLayout pipelineLayout, TextureBindings const & textureBindings)
    : GpuProgram(programName)
    , m_vertexShader(vertexShader)
    , m_fragmentShader(fragmentShader)
    , m_descriptorSetLayout(descriptorSetLayout)
    , m_pipelineLayout(pipelineLayout)
    , m_textureBindings(textureBindings)
  {}

  void Bind() override {}
  void Unbind() override {}

  std::array<VkPipelineShaderStageCreateInfo, 2> GetShaders() const { return {{m_vertexShader, m_fragmentShader}}; }

  VkDescriptorSetLayout GetDescriptorSetLayout() const { return m_descriptorSetLayout; }

  VkPipelineLayout GetPipelineLayout() const { return m_pipelineLayout; }

  TextureBindings const & GetTextureBindings() const { return m_textureBindings; }

private:
  // These objects aren't owned to this class, so must not be destroyed here.
  VkPipelineShaderStageCreateInfo m_vertexShader;
  VkPipelineShaderStageCreateInfo m_fragmentShader;
  VkDescriptorSetLayout m_descriptorSetLayout;
  VkPipelineLayout m_pipelineLayout;

  TextureBindings m_textureBindings;
};
}  // namespace vulkan
}  // namespace dp
