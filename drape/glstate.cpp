#include "glstate.hpp"
#include "glfunctions.hpp"

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

const vector<UniformValue> & GLState::GetUniformValues() const
{
  return m_uniforms;
}

vector<UniformValue> & GLState::GetUniformValues()
{
  return m_uniforms;
}

void ApplyState(GLState state, ReferencePoiner<GpuProgram> program)
{
  TextureBinding & binding = state.GetTextureBinding();
  program->Bind();
  if (binding.IsEnabled())
  {
    int8_t textureLocation = program->GetUniformLocation(binding.GetUniformName());
    if (textureLocation != -1)
      binding.Bind(textureLocation);
  }

  vector<UniformValue> & uniforms = state.GetUniformValues();
  for (size_t i = 0; i < uniforms.size(); ++i)
    uniforms[i].Apply(program);
}
