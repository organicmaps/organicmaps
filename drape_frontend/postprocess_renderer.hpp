#pragma once

#include "drape/drape_global.hpp"
#include "drape/framebuffer.hpp"
#include "drape/pointers.hpp"

#include <cstdint>

namespace dp
{
class Texture;
}  // namespace dp

namespace gpu
{
class ProgramManager;
}  // namespace gpu

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

  void Init(dp::ApiVersion apiVersion, dp::FramebufferFallback && fallback);
  void ClearGLDependentResources();
  void Resize(uint32_t width, uint32_t height);
  void SetStaticTextures(drape_ptr<PostprocessStaticTextures> && textures);

  bool IsEnabled() const;
  void SetEffectEnabled(Effect effect, bool enabled);
  bool IsEffectEnabled(Effect effect) const;

  bool OnFramebufferFallback();
  void OnChangedRouteFollowingMode(bool isRouteFollowingActive);

  bool BeginFrame(bool activeFrame);
  bool EndFrame(ref_ptr<gpu::ProgramManager> gpuProgramManager);

  void EnableWritingToStencil() const;
  void DisableWritingToStencil() const;

private:
  void UpdateFramebuffers(uint32_t width, uint32_t height);
  bool CanRenderAntialiasing() const;

  dp::ApiVersion m_apiVersion;
  uint32_t m_effects = 0;

  drape_ptr<ScreenQuadRenderer> m_screenQuadRenderer;
  dp::FramebufferFallback m_framebufferFallback;
  drape_ptr<PostprocessStaticTextures> m_staticTextures;
  uint32_t m_width = 0;
  uint32_t m_height = 0;

  drape_ptr<dp::Framebuffer> m_mainFramebuffer;
  bool m_isMainFramebufferRendered = false;
  drape_ptr<dp::Framebuffer> m_edgesFramebuffer;
  drape_ptr<dp::Framebuffer> m_blendingWeightFramebuffer;
  drape_ptr<dp::Framebuffer> m_smaaFramebuffer;
  bool m_isSmaaFramebufferRendered = false;

  drape_ptr<RendererContext> m_edgesRendererContext;
  drape_ptr<RendererContext> m_bwRendererContext;
  drape_ptr<RendererContext> m_smaaFinalRendererContext;
  drape_ptr<RendererContext> m_defaultScreenQuadContext;

  bool m_frameStarted = false;
  bool m_isRouteFollowingActive = false;
};

class StencilWriterGuard
{
public:
  explicit StencilWriterGuard(ref_ptr<PostprocessRenderer> renderer);
  ~StencilWriterGuard();
private:
  ref_ptr<PostprocessRenderer> const m_renderer;
};
}  // namespace df
