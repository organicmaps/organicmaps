#pragma once

#include "pointers.hpp"
#include "texture.hpp"
#include "uniform_value.hpp"
#include "uniform_values_storage.hpp"

class GLState
{
public:
  GLState(uint32_t gpuProgramIndex, int16_t depthLayer, const TextureBinding & texture);

  int GetProgramIndex() const;
  const TextureBinding & GetTextureBinding() const;
  TextureBinding & GetTextureBinding();
  const UniformValuesStorage & GetUniformValues() const;
  UniformValuesStorage & GetUniformValues();

  bool operator<(const GLState & other) const
  {
    if (m_depthLayer != other.m_depthLayer)
      return m_depthLayer < other.m_depthLayer;
    if (m_gpuProgramIndex != other.m_gpuProgramIndex)
      return m_gpuProgramIndex < other.m_gpuProgramIndex;

    return m_uniforms < other.m_uniforms;
  }

private:
  uint32_t m_gpuProgramIndex;
  uint16_t m_depthLayer;
  TextureBinding m_texture;
  UniformValuesStorage m_uniforms;
};

void ApplyUniforms(const UniformValuesStorage & uniforms, RefPointer<GpuProgram> program);
void ApplyState(GLState state, RefPointer<GpuProgram> program);
