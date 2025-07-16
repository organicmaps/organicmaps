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

  MetalGpuProgram(std::string const & programName, id<MTLFunction> vertexShader, id<MTLFunction> fragmentShader,
                  int8_t vsUniformsBindingIndex, int8_t fsUniformsBindingIndex,
                  TexturesBindingInfo && vertexTextureBindingInfo, TexturesBindingInfo && fragmentTextureBindingInfo,
                  MTLVertexDescriptor * vertexDescriptor)
    : GpuProgram(programName)
    , m_vertexShader(vertexShader)
    , m_fragmentShader(fragmentShader)
    , m_vsUniformsBindingIndex(vsUniformsBindingIndex)
    , m_fsUniformsBindingIndex(fsUniformsBindingIndex)
    , m_vertexTextureBindingInfo(std::move(vertexTextureBindingInfo))
    , m_fragmentTextureBindingInfo(std::move(fragmentTextureBindingInfo))
    , m_vertexDescriptor(vertexDescriptor)
  {}

  void Bind() override {}
  void Unbind() override {}

  id<MTLFunction> GetVertexShader() const { return m_vertexShader; }
  id<MTLFunction> GetFragmentShader() const { return m_fragmentShader; }

  int8_t GetVertexShaderUniformsBindingIndex() const { return m_vsUniformsBindingIndex; }
  int8_t GetFragmentShaderUniformsBindingIndex() const { return m_fsUniformsBindingIndex; }

  TextureBindingInfo const & GetVertexTextureBindingInfo(std::string const & textureName) const
  {
    return GetTextureBindingInfo(m_vertexTextureBindingInfo, textureName);
  }

  TextureBindingInfo const & GetFragmentTextureBindingInfo(std::string const & textureName) const
  {
    return GetTextureBindingInfo(m_fragmentTextureBindingInfo, textureName);
  }

  MTLVertexDescriptor * GetVertexDescriptor() const { return m_vertexDescriptor; }

private:
  TextureBindingInfo const & GetTextureBindingInfo(TexturesBindingInfo const & bindingInfo,
                                                   std::string const & textureName) const
  {
    static TextureBindingInfo kEmptyBinding;
    auto const it = bindingInfo.find(textureName);
    if (it == bindingInfo.cend())
      return kEmptyBinding;
    return it->second;
  }

  id<MTLFunction> m_vertexShader;
  id<MTLFunction> m_fragmentShader;
  int8_t const m_vsUniformsBindingIndex;
  int8_t const m_fsUniformsBindingIndex;
  TexturesBindingInfo const m_vertexTextureBindingInfo;
  TexturesBindingInfo const m_fragmentTextureBindingInfo;
  MTLVertexDescriptor * m_vertexDescriptor;
};
}  // namespace metal
}  // namespace dp
