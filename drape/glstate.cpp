#include "glstate.hpp"
#include "glfunctions.hpp"

#include "../base/buffer_vector.hpp"
#include "../std/bind.hpp"

#define COLOR_BIT     0x1
#define TEXTURE_BIT   0x2

Blending::Blending(bool isEnabled)
  : m_isEnabled(isEnabled)
  , m_blendFunction(GLConst::GLAddBlend)
  , m_blendSrcFactor(GLConst::GLSrcAlfa)
  , m_blendDstFactor(GLConst::GLOneMinusSrcAlfa)
{
}

void Blending::Apply() const
{
  if (m_isEnabled)
  {
    GLFunctions::glEnable(GLConst::GLBlending);
    GLFunctions::glBlendEquation(m_blendFunction);
    GLFunctions::glBlendFunc(m_blendSrcFactor, m_blendDstFactor);
  }
  else
    GLFunctions::glDisable(GLConst::GLBlending);
}

bool Blending::operator < (const Blending & other) const
{
  if (m_isEnabled != other.m_isEnabled)
    return m_isEnabled < other.m_isEnabled;
  if (m_blendFunction != other.m_blendFunction)
    return m_blendFunction < other.m_blendFunction;
  if (m_blendSrcFactor != other.m_blendSrcFactor)
    return m_blendSrcFactor < other.m_blendSrcFactor;

  return m_blendDstFactor < other.m_blendDstFactor;
}

bool Blending::operator == (const Blending & other) const
{
  return m_isEnabled == other.m_isEnabled &&
         m_blendFunction == other.m_blendFunction &&
         m_blendSrcFactor == other.m_blendSrcFactor &&
         m_blendDstFactor == other.m_blendDstFactor;
}

GLState::GLState(uint32_t gpuProgramIndex, DepthLayer depthLayer)
  : m_gpuProgramIndex(gpuProgramIndex)
  , m_depthLayer(depthLayer)
  , m_textureSet(-1)
  , m_color(0, 0, 0, 0)
  , m_mask(0)
{
}

void GLState::SetTextureSet(int32_t textureSet)
{
  m_mask |= TEXTURE_BIT;
  m_textureSet = textureSet;
}

int32_t GLState::GetTextureSet() const
{
  return m_textureSet;
}

bool GLState::HasTextureSet() const
{
  return (m_mask & TEXTURE_BIT) != 0;
}

void GLState::SetColor(const Color & c)
{
  m_mask |= COLOR_BIT;
  m_color = c;
}

const Color & GLState::GetColor() const
{
  return m_color;
}

bool GLState::HasColor() const
{
  return (m_mask & COLOR_BIT) != 0;
}

void GLState::SetBlending(const Blending & blending)
{
  m_blending = blending;
}

const Blending & GLState::GetBlending() const
{
  return m_blending;
}

int GLState::GetProgramIndex() const
{
  return m_gpuProgramIndex;
}

bool GLState::operator<(const GLState & other) const
{
  if (m_mask != other.m_mask)
    return m_mask < other.m_mask;

  if (m_depthLayer != other.m_depthLayer)
    return m_depthLayer < other.m_depthLayer;
  if (m_gpuProgramIndex != other.m_gpuProgramIndex)
    return m_gpuProgramIndex < other.m_gpuProgramIndex;

  if (m_textureSet != other.m_textureSet)
    return m_textureSet < other.m_textureSet;

  return m_color < other.m_color;
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

void ApplyState(GLState state, RefPointer<GpuProgram> program,
                               RefPointer<TextureSetController> textures)
{
  if (state.HasColor())
  {
    int8_t location = program->GetUniformLocation("u_color");
    float c[4];
    ::Convert(state.GetColor(), c[0], c[1], c[2], c[3]);
    GLFunctions::glUniformValuef(location, c[0], c[1], c[2], c[3]);
  }

  if (state.HasTextureSet())
  {
    uint32_t textureSet = state.GetTextureSet();
    uint32_t count = textures->GetTextureCount(textureSet);
    textures->BindTextureSet(textureSet);
    buffer_vector<int32_t, 16> ids;
    for (uint32_t i = 0; i < count; ++i)
      ids.push_back(i);

    int8_t location = program->GetUniformLocation("u_textures");
    GLFunctions::glUniformValueiv(location, ids.data(), count);
  }

  state.GetBlending().Apply();
}
