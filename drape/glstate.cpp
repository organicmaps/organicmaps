#include "drape/glstate.hpp"
#include "drape/glfunctions.hpp"

#include "base/buffer_vector.hpp"
#include "std/bind.hpp"

#define TEXTURE_BIT 0x1

namespace dp
{

Blending::Blending(bool isEnabled)
  : m_isEnabled(isEnabled)
  , m_blendFunction(gl_const::GLAddBlend)
  , m_blendSrcFactor(gl_const::GLSrcAlfa)
  , m_blendDstFactor(gl_const::GLOneMinusSrcAlfa)
{
}

void Blending::Apply() const
{
  if (m_isEnabled)
  {
    GLFunctions::glEnable(gl_const::GLBlending);
    GLFunctions::glBlendEquation(m_blendFunction);
    GLFunctions::glBlendFunc(m_blendSrcFactor, m_blendDstFactor);
  }
  else
    GLFunctions::glDisable(gl_const::GLBlending);
}

bool Blending::operator < (Blending const & other) const
{
  if (m_isEnabled != other.m_isEnabled)
    return m_isEnabled < other.m_isEnabled;
  if (m_blendFunction != other.m_blendFunction)
    return m_blendFunction < other.m_blendFunction;
  if (m_blendSrcFactor != other.m_blendSrcFactor)
    return m_blendSrcFactor < other.m_blendSrcFactor;

  return m_blendDstFactor < other.m_blendDstFactor;
}

bool Blending::operator == (Blending const & other) const
{
  return m_isEnabled == other.m_isEnabled &&
         m_blendFunction == other.m_blendFunction &&
         m_blendSrcFactor == other.m_blendSrcFactor &&
         m_blendDstFactor == other.m_blendDstFactor;
}

GLState::GLState(uint32_t gpuProgramIndex, DepthLayer depthLayer)
  : m_gpuProgramIndex(gpuProgramIndex)
  , m_depthLayer(depthLayer)
  , m_colorTexture(nullptr)
  , m_maskTexture(nullptr)
{
}

bool GLState::operator<(GLState const & other) const
{
  if (m_depthLayer != other.m_depthLayer)
    return m_depthLayer < other.m_depthLayer;
  if (!(m_blending == other.m_blending))
    return m_blending < other.m_blending;
  if (m_gpuProgramIndex != other.m_gpuProgramIndex)
    return m_gpuProgramIndex < other.m_gpuProgramIndex;
  if (m_colorTexture != other.m_colorTexture)
    return m_colorTexture < other.m_colorTexture;

  return m_maskTexture < other.m_maskTexture;
}

bool GLState::operator==(GLState const & other) const
{
  return m_depthLayer == other.m_depthLayer &&
         m_gpuProgramIndex == other.m_gpuProgramIndex &&
         m_blending == other.m_blending &&
         m_colorTexture == other.m_colorTexture &&
         m_maskTexture == other.m_maskTexture;
}

namespace
{
  void ApplyUniformValue(UniformValue const & value, ref_ptr<GpuProgram> program)
  {
    value.Apply(program);
  }
}

void ApplyUniforms(UniformValuesStorage const & uniforms, ref_ptr<GpuProgram> program)
{
  uniforms.ForeachValue(bind(&ApplyUniformValue, _1, program));
}

void ApplyState(GLState state, ref_ptr<GpuProgram> program)
{
  ref_ptr<Texture> tex = state.GetColorTexture();
  if (tex != nullptr)
  {
    int8_t const colorTexLoc = program->GetUniformLocation("u_colorTex");
    GLFunctions::glActiveTexture(gl_const::GLTexture0);
    tex->Bind();
    GLFunctions::glUniformValuei(colorTexLoc, 0);
  }

  tex = state.GetMaskTexture();
  if (tex != nullptr)
  {
    int8_t const maskTexLoc = program->GetUniformLocation("u_maskTex");
    GLFunctions::glActiveTexture(gl_const::GLTexture0 + 1);
    tex->Bind();
    GLFunctions::glUniformValuei(maskTexLoc, 1);
  }
  state.GetBlending().Apply();
}

}
