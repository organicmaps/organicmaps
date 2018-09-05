#pragma once
#import <MetalKit/MetalKit.h>

#include "drape/gpu_program.hpp"

#include <cstdint>
#include <map>
#include <string>
#include <utility>

namespace dp
{
namespace metal
{
class MetalGpuProgram : public GpuProgram
{
public:
  static int8_t constexpr kInvalidBindingIndex = -1;
  struct TextureBindingInfo
  {
    int8_t m_textureBindingIndex = kInvalidBindingIndex;
    int8_t m_samplerBindingIndex = kInvalidBindingIndex;
  };
  using TexturesBindingInfo = std::map<std::string, TextureBindingInfo>;
  
  MetalGpuProgram(std::string const & programName,
                  id<MTLFunction> vertexShader, id<MTLFunction> fragmentShader,
                  int8_t vsUniformsBindingIndex, int8_t fsUniformsBindingIndex,
                  TexturesBindingInfo && textureBindingInfo)
    : GpuProgram(programName)
    , m_vertexShader(vertexShader)
    , m_fragmentShader(fragmentShader)
    , m_vsUniformsBindingIndex(vsUniformsBindingIndex)
    , m_fsUniformsBindingIndex(fsUniformsBindingIndex)
    , m_textureBindingInfo(std::move(textureBindingInfo))
  {}

  void Bind() override {}
  void Unbind() override {}
  
  id<MTLFunction> GetVertexShader() const { return m_vertexShader; }
  id<MTLFunction> GetFragmentShader() const { return m_fragmentShader; }
  
  int8_t GetVertexShaderUniformsBindingIndex() const { return m_vsUniformsBindingIndex; }
  int8_t GetFragmentShaderUniformsBindingIndex() const { return m_fsUniformsBindingIndex; }
  
  // Now textures can be bound only in fragment shaders.
  TextureBindingInfo const & GetTextureBindingInfo(std::string const & textureName) const
  {
    static TextureBindingInfo kEmptyBinding;
    auto const it = m_textureBindingInfo.find(textureName);
    if (it == m_textureBindingInfo.cend())
      return kEmptyBinding;
    return it->second;
  }

private:
  id<MTLFunction> m_vertexShader;
  id<MTLFunction> m_fragmentShader;
  int8_t const m_vsUniformsBindingIndex;
  int8_t const m_fsUniformsBindingIndex;
  TexturesBindingInfo const m_textureBindingInfo;
};
}  // namespace metal
}  // namespace dp
