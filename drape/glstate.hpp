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
  Blending(bool isEnabled = true);

  void Apply() const;

  bool operator < (Blending const & other) const;
  bool operator == (Blending const & other) const;

  bool m_isEnabled;
};

class GLState
{
public:
  enum DepthLayer
  {
    /// Do not change order
    GeometryLayer,
    DynamicGeometry,
    OverlayLayer,
    UserMarkLayer,
    Gui
  };

  GLState(uint32_t gpuProgramIndex, DepthLayer depthLayer);

  DepthLayer const & GetDepthLayer() const { return m_depthLayer; }

  void SetColorTexture(ref_ptr<Texture> tex) { m_colorTexture = tex; }
  ref_ptr<Texture> GetColorTexture() const { return m_colorTexture; }

  void SetMaskTexture(ref_ptr<Texture> tex) { m_maskTexture = tex; }
  ref_ptr<Texture> GetMaskTexture() const { return m_maskTexture; }

  void SetBlending(Blending const & blending) { m_blending = blending; }
  Blending const & GetBlending() const { return m_blending; }

  int GetProgramIndex() const { return m_gpuProgramIndex; }

  bool operator<(GLState const & other) const;
  bool operator==(GLState const & other) const;

private:
  uint32_t m_gpuProgramIndex;
  DepthLayer m_depthLayer;
  Blending m_blending;

  ref_ptr<Texture> m_colorTexture;
  ref_ptr<Texture> m_maskTexture;
};

void ApplyUniforms(UniformValuesStorage const & uniforms, ref_ptr<GpuProgram> program);
void ApplyState(GLState state, ref_ptr<GpuProgram> program);
void ApplyTextures(GLState state, ref_ptr<GpuProgram> program);
void ApplyBlending(GLState state, ref_ptr<GpuProgram> program);

} // namespace dp
