#pragma once
#import <MetalKit/MetalKit.h>

#include "drape/gpu_program.hpp"

#include <string>

namespace dp
{
namespace metal
{
class MetalGpuProgram : public GpuProgram
{
public:
  MetalGpuProgram(std::string const & programName, id<MTLFunction> vertexShader,
                  id<MTLFunction> fragmentShader)
    : GpuProgram(programName)
    , m_vertexShader(vertexShader)
    , m_fragmentShader(fragmentShader)
  {}

  void Bind() override {}
  void Unbind() override {}
  
  id<MTLFunction> GetVertexShader() const { return m_vertexShader; }
  id<MTLFunction> GetFragmentShader() const { return m_fragmentShader; }

private:
  id<MTLFunction> m_vertexShader;
  id<MTLFunction> m_fragmentShader;
};
}  // namespace metal
}  // namespace dp
