#include "glstate.hpp"
#include "glfunctions.hpp"

#include "../std/bind.hpp"

GLState::GLState(uint32_t gpuProgramIndex, int16_t depthLayer, const TextureBinding & texture)
  : m_gpuProgramIndex(gpuProgramIndex)
  , m_depthLayer(depthLayer)
  , m_texture(texture)
{
}

int GLState::GetProgramIndex() const
{
  return m_gpuProgramIndex;
}

const TextureBinding & GLState::GetTextureBinding() const
{
  return m_texture;
}

TextureBinding & GLState::GetTextureBinding()
{
  return m_texture;
}

const UniformValuesStorage &GLState::GetUniformValues() const
{
  return m_uniforms;
}

UniformValuesStorage & GLState::GetUniformValues()
{
  return m_uniforms;
}

namespace
{
  void ApplyUniformValue(const UniformValue & value, RefPointer<GpuProgram> program)
  {
    value.Apply(program);
  }
}

void ApplyUniforms(const UniformValuesStorage & uniforms, RefPointer<GpuProgram> program)
{
  uniforms.ForeachValue(bind(&ApplyUniformValue, _1, program));
}

void ApplyState(GLState state, RefPointer<GpuProgram> program)
{
  TextureBinding & binding = state.GetTextureBinding();
  if (binding.IsEnabled())
  {
    int8_t textureLocation = program->GetUniformLocation(binding.GetUniformName());
    if (textureLocation != -1)
      binding.Bind(textureLocation);
  }

  ApplyUniforms(state.GetUniformValues(), program);
}
