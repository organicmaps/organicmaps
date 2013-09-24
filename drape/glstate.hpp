#pragma once

#include "pointers.hpp"
#include "texture.hpp"
#include "uniform_value.hpp"

#include "../std/vector.hpp"

class GLState
{
public:
  GLState(uint32_t gpuProgramIndex, int16_t depthLayer, const TextureBinding & texture);

  int GetProgramIndex() const;
  const TextureBinding & GetTextureBinding() const;
  TextureBinding & GetTextureBinding();
  const vector<UniformValue> & GetUniformValues() const;
  vector<UniformValue> & GetUniformValues();

  bool operator<(const GLState & other) const
  {
    return m_depthLayer < other.m_depthLayer
        || m_gpuProgramIndex < other.m_gpuProgramIndex
        //|| m_texture < other.m_texture
        || m_uniforms < other.m_uniforms;
  }

private:
  uint32_t m_gpuProgramIndex;
  uint16_t m_depthLayer;
  TextureBinding m_texture;
  vector<UniformValue> m_uniforms;
};

void ApplyState(GLState state, ReferencePoiner<GpuProgram> program);
