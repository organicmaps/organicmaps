#include "drape/render_state.hpp"
#include "drape/glfunctions.hpp"

#include "base/buffer_vector.hpp"

namespace dp
{
BlendingParams::BlendingParams()
  : m_blendFunction(gl_const::GLAddBlend)
  , m_blendSrcFactor(gl_const::GLSrcAlfa)
  , m_blendDstFactor(gl_const::GLOneMinusSrcAlfa)
{}

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

bool Blending::operator<(Blending const & other) const { return m_isEnabled < other.m_isEnabled; }

bool Blending::operator==(Blending const & other) const { return m_isEnabled == other.m_isEnabled; }

glConst RenderState::GetDepthFunction() const
{
  return m_depthFunction;
}

void RenderState::SetDepthFunction(glConst functionName)
{
  m_depthFunction = functionName;
}

bool RenderState::GetDepthTestEnabled() const
{
  return m_depthTestEnabled;
}

void RenderState::SetDepthTestEnabled(bool enabled)
{
  m_depthTestEnabled = enabled;
}

glConst RenderState::GetTextureFilter() const
{
  return m_textureFilter;
}

void RenderState::SetTextureFilter(glConst filter)
{
  m_textureFilter = filter;
}

bool RenderState::GetDrawAsLine() const
{
  return m_drawAsLine;
}

void RenderState::SetDrawAsLine(bool drawAsLine)
{
  m_drawAsLine = drawAsLine;
}

int RenderState::GetLineWidth() const
{
  return m_lineWidth;
}

void RenderState::SetLineWidth(int width)
{
  m_lineWidth = width;
}

bool RenderState::operator<(RenderState const & other) const
{
  if (!m_renderStateExtension->Equal(other.m_renderStateExtension))
    return m_renderStateExtension->Less(other.m_renderStateExtension);
  if (!(m_blending == other.m_blending))
    return m_blending < other.m_blending;
  if (m_gpuProgram != other.m_gpuProgram)
    return m_gpuProgram < other.m_gpuProgram;
  if (m_gpuProgram3d != other.m_gpuProgram3d)
    return m_gpuProgram3d < other.m_gpuProgram3d;
  if (m_depthFunction != other.m_depthFunction)
    return m_depthFunction < other.m_depthFunction;
  if (m_colorTexture != other.m_colorTexture)
    return m_colorTexture < other.m_colorTexture;
  if (m_maskTexture != other.m_maskTexture)
    return m_maskTexture < other.m_maskTexture;
  if (m_textureFilter != other.m_textureFilter)
    return m_textureFilter < other.m_textureFilter;
  if (m_drawAsLine != other.m_drawAsLine)
    return m_drawAsLine < other.m_drawAsLine;

  return m_lineWidth < other.m_lineWidth;
}

bool RenderState::operator==(RenderState const & other) const
{
  return m_renderStateExtension->Equal(other.m_renderStateExtension) &&
         m_gpuProgram == other.m_gpuProgram &&
         m_gpuProgram3d == other.m_gpuProgram3d &&
         m_blending == other.m_blending &&
         m_colorTexture == other.m_colorTexture &&
         m_maskTexture == other.m_maskTexture &&
         m_textureFilter == other.m_textureFilter &&
         m_depthFunction == other.m_depthFunction &&
         m_drawAsLine == other.m_drawAsLine &&
         m_lineWidth == other.m_lineWidth;
}

bool RenderState::operator!=(RenderState const & other) const
{
  return !operator==(other);
}

uint8_t TextureState::m_usedSlots = 0;

void TextureState::ApplyTextures(RenderState const & state, ref_ptr<GpuProgram> program)
{
  m_usedSlots = 0;

  ref_ptr<Texture> tex = state.GetColorTexture();
  int8_t colorTexLoc = -1;
  if (tex != nullptr && (colorTexLoc = program->GetUniformLocation("u_colorTex")) >= 0)
  {
    GLFunctions::glActiveTexture(gl_const::GLTexture0);
    tex->Bind();
    GLFunctions::glUniformValuei(colorTexLoc, 0);
    tex->SetFilter(state.GetTextureFilter());
    m_usedSlots++;
  }

  tex = state.GetMaskTexture();
  int8_t maskTexLoc = -1;
  if (tex != nullptr && (maskTexLoc = program->GetUniformLocation("u_maskTex")) >= 0)
  {
    GLFunctions::glActiveTexture(gl_const::GLTexture0 + 1);
    tex->Bind();
    GLFunctions::glUniformValuei(maskTexLoc, 1);
    tex->SetFilter(state.GetTextureFilter());
    m_usedSlots++;
  }
}

uint8_t TextureState::GetLastUsedSlots()
{
  return m_usedSlots;
}

void ApplyBlending(RenderState const & state)
{
  state.GetBlending().Apply();
}

void ApplyState(RenderState const & state, ref_ptr<GpuProgram> program)
{
  TextureState::ApplyTextures(state, program);
  ApplyBlending(state);
  if (state.GetDepthTestEnabled())
  {
    GLFunctions::glEnable(gl_const::GLDepthTest);
    GLFunctions::glDepthFunc(state.GetDepthFunction());
  }
  else
  {
    GLFunctions::glDisable(gl_const::GLDepthTest);
  }
  ASSERT_GREATER_OR_EQUAL(state.GetLineWidth(), 0, ());
  GLFunctions::glLineWidth(static_cast<uint32_t>(state.GetLineWidth()));
}
}  // namespace dp
