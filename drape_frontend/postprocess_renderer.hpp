#pragma once

#include "drape/framebuffer.hpp"
#include "drape/pointers.hpp"

namespace dp
{
class GpuProgramManager;
class Texture;
}  // namespace dp

namespace df
{
class ScreenQuadRenderer;
class RendererContext;

struct PostprocessStaticTextures
{
  ref_ptr<dp::Texture> m_smaaAreaTexture;
  ref_ptr<dp::Texture> m_smaaSearchTexture;
};

class PostprocessRenderer
{
public:
  enum Effect
  {
    Antialiasing = 1
  };

  PostprocessRenderer();
  ~PostprocessRenderer();

  void Init(dp::FramebufferFallback && fallback);
  void ClearGLDependentResources();
  void Resize(uint32_t width, uint32_t height);
  void SetStaticTextures(drape_ptr<PostprocessStaticTextures> && textures);

  void SetEnabled(bool enabled);
  bool IsEnabled() const;
  void SetEffectEnabled(Effect effect, bool enabled);
  bool IsEffectEnabled(Effect effect) const;

  void OnFramebufferFallback();

  void BeginFrame();
  void EndFrame(ref_ptr<dp::GpuProgramManager> gpuProgramManager);

  void EnableWritingToStencil() const;
  void DisableWritingToStencil() const;

private:
  void UpdateFramebuffers(uint32_t width, uint32_t height);

  bool m_isEnabled;
  uint32_t m_effects;

  drape_ptr<ScreenQuadRenderer> m_screenQuadRenderer;
  dp::FramebufferFallback m_framebufferFallback;
  drape_ptr<PostprocessStaticTextures> m_staticTextures;
  uint32_t m_width;
  uint32_t m_height;

  drape_ptr<dp::Framebuffer> m_mainFramebuffer;
  drape_ptr<dp::Framebuffer> m_edgesFramebuffer;
  drape_ptr<dp::Framebuffer> m_blendingWeightFramebuffer;

  drape_ptr<RendererContext> m_edgesRendererContext;
  drape_ptr<RendererContext> m_bwRendererContext;
  drape_ptr<RendererContext> m_smaaFinalRendererContext;

  bool m_frameStarted;
};

class StencilWriterGuard
{
public:
  StencilWriterGuard(ref_ptr<PostprocessRenderer> renderer);
  ~StencilWriterGuard();
private:
  ref_ptr<PostprocessRenderer> const m_renderer;
};
}  // namespace df
