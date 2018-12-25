#pragma once

#include "drape/gpu_program.hpp"

#include "base/visitor.hpp"

#include <vulkan_wrapper.h>
#include <vulkan/vulkan.h>

#include <algorithm>
#include <cstdint>
#include <map>
#include <string>
#include <utility>
#include <vector>

namespace dp
{
namespace vulkan
{
class VulkanGpuProgram : public GpuProgram
{
public:
  static int8_t constexpr kInvalidBindingIndex = -1;

  struct TextureBindingInfo
  {
    std::string m_name;
    int8_t m_index = kInvalidBindingIndex;
    DECLARE_VISITOR(visitor(m_name, "name"),
                    visitor(m_index, "idx"))
  };

  struct ReflectionInfo
  {
    int8_t m_vsUniformsIndex = kInvalidBindingIndex;
    int8_t m_fsUniformsIndex = kInvalidBindingIndex;
    std::vector<TextureBindingInfo> m_textures;
    DECLARE_VISITOR(visitor(m_vsUniformsIndex, "vs_uni"),
                    visitor(m_fsUniformsIndex, "fs_uni"),
                    visitor(m_textures, "tex"))
  };

  using TexturesBindingInfo = std::map<std::string, TextureBindingInfo>;

  VulkanGpuProgram(std::string const & programName, ReflectionInfo && reflectionInfo,
                   VkShaderModule vertexShader, VkShaderModule fragmentShader)
    : GpuProgram(programName)
    , m_reflectionInfo(std::move(reflectionInfo))
    , m_vertexShader(vertexShader)
    , m_fragmentShader(fragmentShader)
  {}

  void Bind() override {}
  void Unbind() override {}

  VkShaderModule GetVertexShader() const { return m_vertexShader; }
  VkShaderModule GetFragmentShader() const { return m_fragmentShader; }
  ReflectionInfo GetReflectionInfo() const { return m_reflectionInfo; }

  int8_t GetVertexShaderUniformsBindingIndex() const { return m_reflectionInfo.m_vsUniformsIndex; }
  int8_t GetFragmentShaderUniformsBindingIndex() const { return m_reflectionInfo.m_fsUniformsIndex; }

  TextureBindingInfo const & GetVertexTextureBindingInfo(std::string const & textureName) const
  {
    return GetTextureBindingInfo(textureName);
  }

  TextureBindingInfo const & GetFragmentTextureBindingInfo(std::string const & textureName) const
  {
    return GetTextureBindingInfo(textureName);
  }

private:
  TextureBindingInfo const & GetTextureBindingInfo(std::string const & textureName) const
  {
    static TextureBindingInfo kEmptyBinding;
    auto const it = std::find_if(m_reflectionInfo.m_textures.cbegin(),
                                 m_reflectionInfo.m_textures.cend(),
                                 [&textureName](TextureBindingInfo const & info)
    {
      return info.m_name == textureName;
    });
    if (it == m_reflectionInfo.m_textures.cend())
      return kEmptyBinding;
    return *it;
  }

  ReflectionInfo m_reflectionInfo;
  VkShaderModule m_vertexShader;
  VkShaderModule m_fragmentShader;
};
}  // namespace vulkan
}  // namespace dp
