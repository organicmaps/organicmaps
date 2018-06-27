#pragma once

#include "drape/glconstants.hpp"
#include "drape/gpu_program.hpp"
#include "drape/pointers.hpp"
#include "drape/texture.hpp"
#include "drape/uniform_values_storage.hpp"

#include "base/assert.hpp"

#include <utility>

namespace dp
{
struct BlendingParams
{
  BlendingParams();

  void Apply() const;

  glConst m_blendFunction;
  glConst m_blendSrcFactor;
  glConst m_blendDstFactor;
};

struct Blending
{
  explicit Blending(bool isEnabled = true);

  void Apply() const;

  bool operator<(Blending const & other) const;
  bool operator==(Blending const & other) const;

  bool m_isEnabled;
};

class BaseRenderState
{
public:
  virtual ~BaseRenderState() = default;
  virtual bool Less(ref_ptr<dp::BaseRenderState> other) const = 0;
  virtual bool Equal(ref_ptr<dp::BaseRenderState> other) const = 0;
};

class GLState
{
public:
  template<typename ProgramType>
  GLState(ProgramType gpuProgram, ref_ptr<BaseRenderState> renderState)
    : m_renderState(std::move(renderState))
    , m_gpuProgram(static_cast<size_t>(gpuProgram))
    , m_gpuProgram3d(static_cast<size_t>(gpuProgram))
  {
    ASSERT(m_renderState != nullptr, ());
  }

  template<typename RenderStateType>
  ref_ptr<RenderStateType> GetRenderState() const
  {
    ASSERT(dynamic_cast<RenderStateType *>(m_renderState.get()) != nullptr, ());
    return make_ref(static_cast<RenderStateType *>(m_renderState.get()));
  }

  void SetColorTexture(ref_ptr<Texture> tex) { m_colorTexture = tex; }
  ref_ptr<Texture> GetColorTexture() const { return m_colorTexture; }

  void SetMaskTexture(ref_ptr<Texture> tex) { m_maskTexture = tex; }
  ref_ptr<Texture> GetMaskTexture() const { return m_maskTexture; }

  void SetBlending(Blending const & blending) { m_blending = blending; }
  Blending const & GetBlending() const { return m_blending; }

  template<typename ProgramType>
  ProgramType GetProgram() const { return static_cast<ProgramType>(m_gpuProgram); }

  template<typename ProgramType>
  void SetProgram3d(ProgramType gpuProgram3d) { m_gpuProgram3d = static_cast<size_t>(gpuProgram3d); }

  template<typename ProgramType>
  ProgramType GetProgram3d() const { return static_cast<ProgramType>(m_gpuProgram3d); }

  glConst GetDepthFunction() const;
  void SetDepthFunction(glConst functionName);

  glConst GetTextureFilter() const;
  void SetTextureFilter(glConst filter);

  bool GetDrawAsLine() const;
  void SetDrawAsLine(bool drawAsLine);
  int GetLineWidth() const;
  void SetLineWidth(int width);

  bool operator<(GLState const & other) const;
  bool operator==(GLState const & other) const;
  bool operator!=(GLState const & other) const;

private:
  ref_ptr<BaseRenderState> m_renderState;
  size_t m_gpuProgram;
  size_t m_gpuProgram3d;
  Blending m_blending;
  glConst m_depthFunction = gl_const::GLLessOrEqual;
  glConst m_textureFilter = gl_const::GLLinear;

  ref_ptr<Texture> m_colorTexture;
  ref_ptr<Texture> m_maskTexture;

  bool m_drawAsLine = false;
  int m_lineWidth = 1;
};

class TextureState
{
public:
  static void ApplyTextures(GLState const & state, ref_ptr<GpuProgram> program);
  static uint8_t GetLastUsedSlots();

private:
  static uint8_t m_usedSlots;
};

void ApplyUniforms(UniformValuesStorage const & uniforms, ref_ptr<GpuProgram> program);
void ApplyState(GLState const & state, ref_ptr<GpuProgram> program);
void ApplyBlending(GLState const & state);
}  // namespace dp
