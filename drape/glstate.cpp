#include "drape/glstate.hpp"
#include "drape/glfunctions.hpp"

#include "base/buffer_vector.hpp"
#include "std/bind.hpp"

#define TEXTURE_BIT 0x1

namespace dp
{

BlendingParams::BlendingParams()
  : m_blendFunction(gl_const::GLAddBlend)
  , m_blendSrcFactor(gl_const::GLSrcAlfa)
  , m_blendDstFactor(gl_const::GLOneMinusSrcAlfa)
{
}

void BlendingParams::Apply() const
{
  GLFunctions::glBlendEquation(m_blendFunction);
  GLFunctions::glBlendFunc(m_blendSrcFactor, m_blendDstFactor);
}

Blending::Blending(bool isEnabled)
  : m_isEnabled(isEnabled)
{}

void Blending::Apply() const
{
  if (m_isEnabled)
    GLFunctions::glEnable(gl_const::GLBlending);
  else
    GLFunctions::glDisable(gl_const::GLBlending);
}

bool Blending::operator < (Blending const & other) const
{
  return m_isEnabled < other.m_isEnabled;
}

bool Blending::operator == (Blending const & other) const
{
  return m_isEnabled == other.m_isEnabled;
}

GLState::GLState(uint32_t gpuProgramIndex, DepthLayer depthLayer)
  : m_gpuProgramIndex(gpuProgramIndex)
  , m_gpuProgram3dIndex(gpuProgramIndex)
  , m_depthLayer(depthLayer)
  , m_depthFunction(gl_const::GLLessOrEqual)
  , m_textureFilter(gl_const::GLLinear)
  , m_colorTexture(nullptr)
  , m_maskTexture(nullptr)
{
}

glConst GLState::GetDepthFunction() const
{
  return m_depthFunction;
}

void GLState::SetDepthFunction(glConst functionName)
{
  m_depthFunction = functionName;
}

glConst GLState::GetTextureFilter() const
{
  return m_textureFilter;
}

void GLState::SetTextureFilter(glConst filter)
{
  m_textureFilter = filter;
}

bool GLState::operator<(GLState const & other) const
{
  if (m_depthLayer != other.m_depthLayer)
    return m_depthLayer < other.m_depthLayer;
  if (!(m_blending == other.m_blending))
    return m_blending < other.m_blending;
  if (m_gpuProgramIndex != other.m_gpuProgramIndex)
    return m_gpuProgramIndex < other.m_gpuProgramIndex;
  if (m_gpuProgram3dIndex != other.m_gpuProgram3dIndex)
    return m_gpuProgram3dIndex < other.m_gpuProgram3dIndex;
  if (m_colorTexture != other.m_colorTexture)
    return m_colorTexture < other.m_colorTexture;

  return m_maskTexture < other.m_maskTexture;
}

bool GLState::operator==(GLState const & other) const
{
  return m_depthLayer == other.m_depthLayer &&
         m_gpuProgramIndex == other.m_gpuProgramIndex &&
         m_gpuProgram3dIndex == other.m_gpuProgram3dIndex &&
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

void ApplyTextures(GLState state, ref_ptr<GpuProgram> program)
{
  ref_ptr<Texture> tex = state.GetColorTexture();
  int8_t colorTexLoc = -1;
  if (tex != nullptr && (colorTexLoc = program->GetUniformLocation("u_colorTex")) >= 0)
  {
    GLFunctions::glActiveTexture(gl_const::GLTexture0);
    tex->Bind();
    GLFunctions::glUniformValuei(colorTexLoc, 0);
    tex->SetFilter(state.GetTextureFilter());
  }
  else
  {
    // Some Android devices (Galaxy Nexus) require to reset texture state explicitly.
    // It's caused by a bug in OpenGL driver (for Samsung Nexus, maybe others). Normally 
    // we don't need to explicitly call glBindTexture(GL_TEXTURE2D, 0) after glUseProgram
    // in case of the GPU-program doesn't use textures. Here we have to do it to work around
    // graphics artefacts. The overhead isn't significant, we don't know on which devices 
    // it may happen so do it for all Android devices.
#ifdef OMIM_OS_ANDROID
    GLFunctions::glActiveTexture(gl_const::GLTexture0);
    GLFunctions::glBindTexture(0);
#endif
  }

  tex = state.GetMaskTexture();
  int8_t maskTexLoc = -1;
  if (tex != nullptr && (maskTexLoc = program->GetUniformLocation("u_maskTex")) >= 0)
  {
    GLFunctions::glActiveTexture(gl_const::GLTexture0 + 1);
    tex->Bind();
    GLFunctions::glUniformValuei(maskTexLoc, 1);
    tex->SetFilter(state.GetTextureFilter());
  }
  else
  {
    // Some Android devices (Galaxy Nexus) require to reset texture state explicitly.
    // See detailed description above.
#ifdef OMIM_OS_ANDROID
    GLFunctions::glActiveTexture(gl_const::GLTexture0 + 1);
    GLFunctions::glBindTexture(0);
#endif
  }
}

void ApplyBlending(GLState state, ref_ptr<GpuProgram> program)
{
  state.GetBlending().Apply();
}

void ApplyState(GLState state, ref_ptr<GpuProgram> program)
{
  ApplyTextures(state, program);
  ApplyBlending(state, program);
  GLFunctions::glDepthFunc(state.GetDepthFunction());
}

}
