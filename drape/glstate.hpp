#pragma once

#include "pointers.hpp"
#include "uniform_values_storage.hpp"
#include "texture_set_controller.hpp"
#include "color.hpp"

namespace dp
{

struct Blending
{
  Blending(bool isEnabled = false);

  void Apply() const;

  bool operator < (Blending const & other) const;
  bool operator == (Blending const & other) const;

  bool m_isEnabled;
  glConst m_blendFunction;
  glConst m_blendSrcFactor;
  glConst m_blendDstFactor;
};

class GLState
{
public:
  enum DepthLayer
  {
    /// Do not change order
    GeometryLayer,
    DynamicGeometry,
    OverlayLayer
  };

  GLState(uint32_t gpuProgramIndex, DepthLayer depthLayer);

  DepthLayer const & GetDepthLayer() const { return m_depthLayer; }

  void SetTextureSet(int32_t textureSet);
  int32_t GetTextureSet() const { return m_textureSet; }
  bool HasTextureSet() const;

  void SetColor(Color const & c);
  Color const & GetColor() const { return m_color; }
  bool HasColor() const;

  void SetBlending(Blending const & blending);
  Blending const & GetBlending() const { return m_blending; }

  int GetProgramIndex() const { return m_gpuProgramIndex; }

  bool operator<(GLState const & other) const;
  bool operator==(GLState const & other) const;

private:
  uint32_t m_gpuProgramIndex;
  DepthLayer m_depthLayer;
  int32_t m_textureSet;
  Blending m_blending;
  Color m_color;

  uint32_t m_mask;
};

void ApplyUniforms(UniformValuesStorage const & uniforms, RefPointer<GpuProgram> program);
void ApplyState(GLState state, RefPointer<GpuProgram> program, RefPointer<TextureSetController> textures);

} // namespace dp
