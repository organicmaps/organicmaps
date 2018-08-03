#pragma once

#include "drape/gpu_program.hpp"
#include "drape/pointers.hpp"
#include "drape/texture.hpp"

#include "base/assert.hpp"

#include <utility>

namespace dp
{
struct AlphaBlendingState
{
  static void Apply();
};

struct Blending
{
  explicit Blending(bool isEnabled = true);

  void Apply() const;

  bool operator<(Blending const & other) const;
  bool operator==(Blending const & other) const;

  bool m_isEnabled;
};

enum class DepthFunction : uint8_t
{
  Never,
  Less,
  Equal,
  LessOrEqual,
  Great,
  NotEqual,
  GreatOrEqual,
  Always
};

class BaseRenderStateExtension
{
public:
  virtual ~BaseRenderStateExtension() = default;
  virtual bool Less(ref_ptr<dp::BaseRenderStateExtension> other) const = 0;
  virtual bool Equal(ref_ptr<dp::BaseRenderStateExtension> other) const = 0;
};

class RenderState
{
public:
  template<typename ProgramType>
  RenderState(ProgramType gpuProgram, ref_ptr<BaseRenderStateExtension> renderStateExtension)
    : m_renderStateExtension(std::move(renderStateExtension))
    , m_gpuProgram(static_cast<size_t>(gpuProgram))
    , m_gpuProgram3d(static_cast<size_t>(gpuProgram))
  {
    ASSERT(m_renderStateExtension != nullptr, ());
  }

  template<typename RenderStateExtensionType>
  ref_ptr<RenderStateExtensionType> GetRenderStateExtension() const
  {
    ASSERT(dynamic_cast<RenderStateExtensionType *>(m_renderStateExtension.get()) != nullptr, ());
    return make_ref(static_cast<RenderStateExtensionType *>(m_renderStateExtension.get()));
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

  DepthFunction GetDepthFunction() const;
  void SetDepthFunction(DepthFunction depthFunction);

  bool GetDepthTestEnabled() const;
  void SetDepthTestEnabled(bool enabled);

  TextureFilter GetTextureFilter() const;
  void SetTextureFilter(TextureFilter filter);

  bool GetDrawAsLine() const;
  void SetDrawAsLine(bool drawAsLine);
  int GetLineWidth() const;
  void SetLineWidth(int width);

  bool operator<(RenderState const & other) const;
  bool operator==(RenderState const & other) const;
  bool operator!=(RenderState const & other) const;

private:
  ref_ptr<BaseRenderStateExtension> m_renderStateExtension;
  size_t m_gpuProgram;
  size_t m_gpuProgram3d;
  Blending m_blending;

  bool m_depthTestEnabled = true;
  DepthFunction m_depthFunction = DepthFunction::LessOrEqual;

  TextureFilter m_textureFilter = TextureFilter::Linear;

  ref_ptr<Texture> m_colorTexture;
  ref_ptr<Texture> m_maskTexture;

  bool m_drawAsLine = false;
  int m_lineWidth = 1;
};

class TextureState
{
public:
  static void ApplyTextures(RenderState const & state, ref_ptr<GpuProgram> program);
  static uint8_t GetLastUsedSlots();

private:
  static uint8_t m_usedSlots;
};

void ApplyState(RenderState const & state, ref_ptr<GpuProgram> program);
void ApplyBlending(RenderState const & state);
}  // namespace dp
