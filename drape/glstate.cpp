#include "glstate.hpp"
#include "glfunctions.hpp"

#include "../std/bind.hpp"

GLState::GLState(uint32_t gpuProgramIndex, int16_t depthLayer)
  : m_gpuProgramIndex(gpuProgramIndex)
  , m_depthLayer(depthLayer)
{
}

int GLState::GetProgramIndex() const
{
  return m_gpuProgramIndex;
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
  ApplyUniforms(state.GetUniformValues(), program);
}
