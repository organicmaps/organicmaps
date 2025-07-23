#pragma once

#include "drape/gpu_program.hpp"
#include "drape/graphics_context.hpp"
#include "drape/pointers.hpp"
#include "drape/texture.hpp"

#include "base/assert.hpp"

#include <cstdint>
#include <map>
#include <utility>

namespace dp
{
struct AlphaBlendingState
{
  static void Apply(ref_ptr<GraphicsContext> context);
};

struct Blending
{
  explicit Blending(bool isEnabled = true);

  void Apply(ref_ptr<GraphicsContext> context, ref_ptr<GpuProgram> program) const;

  bool operator<(Blending const & other) const;
  bool operator==(Blending const & other) const;

  bool m_isEnabled;
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
  template <typename ProgramType>
  RenderState(ProgramType gpuProgram, ref_ptr<BaseRenderStateExtension> renderStateExtension)
    : m_renderStateExtension(std::move(renderStateExtension))
    , m_gpuProgram(static_cast<size_t>(gpuProgram))
    , m_gpuProgram3d(static_cast<size_t>(gpuProgram))
  {
    ASSERT(m_renderStateExtension != nullptr, ());
  }

  template <typename RenderStateExtensionType>
  ref_ptr<RenderStateExtensionType> GetRenderStateExtension() const
  {
    ASSERT(dynamic_cast<RenderStateExtensionType *>(m_renderStateExtension.get()) != nullptr, ());
    return make_ref(static_cast<RenderStateExtensionType *>(m_renderStateExtension.get()));
  }

  void SetColorTexture(ref_ptr<Texture> tex);
  ref_ptr<Texture> GetColorTexture() const;

  void SetMaskTexture(ref_ptr<Texture> tex);
  ref_ptr<Texture> GetMaskTexture() const;

  void SetTexture(std::string const & name, ref_ptr<Texture> tex);
  ref_ptr<Texture> GetTexture(std::string const & name) const;
  std::map<std::string, ref_ptr<Texture>> const & GetTextures() const;

  void SetBlending(Blending const & blending) { m_blending = blending; }
  Blending const & GetBlending() const { return m_blending; }

  template <typename ProgramType>
  ProgramType GetProgram() const
  {
    return static_cast<ProgramType>(m_gpuProgram);
  }

  template <typename ProgramType>
  void SetProgram3d(ProgramType gpuProgram3d)
  {
    m_gpuProgram3d = static_cast<size_t>(gpuProgram3d);
  }

  template <typename ProgramType>
  ProgramType GetProgram3d() const
  {
    return static_cast<ProgramType>(m_gpuProgram3d);
  }

  TestFunction GetDepthFunction() const;
  void SetDepthFunction(TestFunction depthFunction);

  bool GetDepthTestEnabled() const;
  void SetDepthTestEnabled(bool enabled);

  TextureFilter GetTextureFilter() const;
  void SetTextureFilter(TextureFilter filter);

  bool GetDrawAsLine() const;
  void SetDrawAsLine(bool drawAsLine);
  int GetLineWidth() const;
  void SetLineWidth(int width);

  uint32_t GetTextureIndex() const;
  void SetTextureIndex(uint32_t index);

  bool operator<(RenderState const & other) const;
  bool operator==(RenderState const & other) const;
  bool operator!=(RenderState const & other) const;

private:
  ref_ptr<BaseRenderStateExtension> m_renderStateExtension;
  size_t m_gpuProgram;
  size_t m_gpuProgram3d;
  Blending m_blending;

  bool m_depthTestEnabled = true;
  TestFunction m_depthFunction = TestFunction::LessOrEqual;

  TextureFilter m_textureFilter = TextureFilter::Linear;

  std::map<std::string, ref_ptr<Texture>> m_textures;

  bool m_drawAsLine = false;
  int m_lineWidth = 1;
  uint32_t m_textureIndex = 0;
};

class TextureState
{
public:
  static void ApplyTextures(ref_ptr<GraphicsContext> context, RenderState const & state, ref_ptr<GpuProgram> program);
  static uint8_t GetLastUsedSlots();

private:
  static uint8_t m_usedSlots;
};

void ApplyState(ref_ptr<GraphicsContext> context, ref_ptr<GpuProgram> program, RenderState const & state);
}  // namespace dp
