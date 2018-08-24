#include "drape/render_state.hpp"
#include "drape/glfunctions.hpp"

#include "base/buffer_vector.hpp"

namespace dp
{
namespace
{
std::string const kColorTextureName = "u_colorTex";
std::string const kMaskTextureName = "u_maskTex";
}  // namespace

// static
void AlphaBlendingState::Apply()
{
  GLFunctions::glBlendEquation(gl_const::GLAddBlend);
  GLFunctions::glBlendFunc(gl_const::GLSrcAlpha, gl_const::GLOneMinusSrcAlpha);
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

void RenderState::SetColorTexture(ref_ptr<Texture> tex)
{
  m_textures[kColorTextureName] = tex;
}

ref_ptr<Texture> RenderState::GetColorTexture() const
{
  auto const it = m_textures.find(kColorTextureName);
  if (it != m_textures.end())
    return it->second;
  return nullptr;
}

void RenderState::SetMaskTexture(ref_ptr<Texture> tex)
{
  m_textures[kMaskTextureName] = tex;
}

ref_ptr<Texture> RenderState::GetMaskTexture() const
{
  auto const it = m_textures.find(kMaskTextureName);
  if (it != m_textures.end())
    return it->second;
  return nullptr;
}

void RenderState::SetTexture(std::string const & name, ref_ptr<Texture> tex)
{
  m_textures[name] = tex;
}

ref_ptr<Texture> RenderState::GetTexture(std::string const & name) const
{
  auto const it = m_textures.find(name);
  if (it != m_textures.end())
    return it->second;
  return nullptr;
}

std::map<std::string, ref_ptr<Texture>> const & RenderState::GetTextures() const
{
  return m_textures;
}

TestFunction RenderState::GetDepthFunction() const
{
  return m_depthFunction;
}

void RenderState::SetDepthFunction(TestFunction depthFunction)
{
  m_depthFunction = depthFunction;
}

bool RenderState::GetDepthTestEnabled() const
{
  return m_depthTestEnabled;
}

void RenderState::SetDepthTestEnabled(bool enabled)
{
  m_depthTestEnabled = enabled;
}

TextureFilter RenderState::GetTextureFilter() const
{
  return m_textureFilter;
}

void RenderState::SetTextureFilter(TextureFilter filter)
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
  if (m_textures != other.m_textures)
    return m_textures < other.m_textures;
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
         m_textures == other.m_textures &&
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

  for (auto const & texture : state.GetTextures())
  {
    auto const tex = texture.second;
    int8_t texLoc = -1;
    if (tex != nullptr && (texLoc = program->GetUniformLocation(texture.first)) >= 0)
    {
      GLFunctions::glActiveTexture(gl_const::GLTexture0 + m_usedSlots);
      tex->Bind();
      GLFunctions::glUniformValuei(texLoc, m_usedSlots);
      tex->SetFilter(state.GetTextureFilter());
      m_usedSlots++;
    }
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

void ApplyState(ref_ptr<GraphicsContext> context, ref_ptr<GpuProgram> program, RenderState const & state)
{
  TextureState::ApplyTextures(state, program);
  ApplyBlending(state);
  context->SetDepthTestEnabled(state.GetDepthTestEnabled());
  if (state.GetDepthTestEnabled())
    context->SetDepthTestFunction(state.GetDepthFunction());

  ASSERT_GREATER_OR_EQUAL(state.GetLineWidth(), 0, ());
  GLFunctions::glLineWidth(static_cast<uint32_t>(state.GetLineWidth()));
}
}  // namespace dp
