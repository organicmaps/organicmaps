#pragma once

#include "drape/pointers.hpp"
#include "drape/texture.hpp"
#include "drape/gpu_program.hpp"
#include "drape/uniform_values_storage.hpp"

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

  bool operator < (Blending const & other) const;
  bool operator == (Blending const & other) const;

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
  GLState(int gpuProgramIndex, ref_ptr<BaseRenderState> renderState);

  template <typename RenderStateType>
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

  int GetProgramIndex() const { return m_gpuProgramIndex; }

  void SetProgram3dIndex(int gpuProgram3dIndex) { m_gpuProgram3dIndex = gpuProgram3dIndex; }
  int GetProgram3dIndex() const { return m_gpuProgram3dIndex; }

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
  int m_gpuProgramIndex;
  int m_gpuProgram3dIndex;
  Blending m_blending;
  glConst m_depthFunction;
  glConst m_textureFilter;

  ref_ptr<Texture> m_colorTexture;
  ref_ptr<Texture> m_maskTexture;

  bool m_drawAsLine;
  int m_lineWidth;
};

class TextureState
{
public:
  static void ApplyTextures(GLState state, ref_ptr<GpuProgram> program);
  static uint8_t GetLastUsedSlots();

private:
  static uint8_t m_usedSlots;
};

void ApplyUniforms(UniformValuesStorage const & uniforms, ref_ptr<GpuProgram> program);
void ApplyState(GLState const & state, ref_ptr<GpuProgram> program);
void ApplyBlending(GLState const & sstate);
}  // namespace dp
