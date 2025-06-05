#pragma once

#include "shaders/program_params.hpp"

#include "drape/drape_global.hpp"
#include "drape/framebuffer.hpp"
#include "drape/pointers.hpp"
#include "drape/render_state.hpp"
#include "drape/viewport.hpp"

#include "geometry/screenbase.hpp"

#include <cstdint>

namespace dp
{
class GraphicsContext;
class Texture;
}  // namespace dp

namespace gpu
{
class ProgramManager;
}  // namespace gpu

namespace df
{
class ScreenQuadRenderer;

struct PostprocessStaticTextures
{
  ref_ptr<dp::Texture> m_smaaAreaTexture;
  ref_ptr<dp::Texture> m_smaaSearchTexture;
};

using PrerenderFrame = std::function<void(ScreenBase const & modelView)>;

class PostprocessRenderer
{
public:
  enum Effect
  {
    Antialiasing = 1
  };

  PostprocessRenderer() = default;
  ~PostprocessRenderer();

  void Init(ref_ptr<dp::GraphicsContext> context, dp::FramebufferFallback && fallback,
            PrerenderFrame && prerenderFrame);
  void ClearContextDependentResources();
  void Resize(ref_ptr<dp::GraphicsContext> context, uint32_t width, uint32_t height);
  void SetStaticTextures(drape_ptr<PostprocessStaticTextures> && textures);

  bool IsEnabled() const;
  void SetEffectEnabled(ref_ptr<dp::GraphicsContext> context, Effect effect, bool enabled);
  bool IsEffectEnabled(Effect effect) const;

  bool OnFramebufferFallback(ref_ptr<dp::GraphicsContext> context);
  void OnChangedRouteFollowingMode(ref_ptr<dp::GraphicsContext> context, bool isRouteFollowingActive);

  bool BeginFrame(ref_ptr<dp::GraphicsContext> context, ScreenBase const & modelView, bool activeFrame);
  bool EndFrame(ref_ptr<dp::GraphicsContext> context, ref_ptr<gpu::ProgramManager> gpuProgramManager,
                dp::Viewport const & viewport);

  void EnableWritingToStencil(ref_ptr<dp::GraphicsContext> context) const;
  void DisableWritingToStencil(ref_ptr<dp::GraphicsContext> context) const;

private:
  void UpdateFramebuffers(ref_ptr<dp::GraphicsContext> context, uint32_t width, uint32_t height);
  bool CanRenderAntialiasing() const;

  dp::ApiVersion m_apiVersion = dp::ApiVersion::Invalid;
  uint32_t m_effects = 0;

  drape_ptr<ScreenQuadRenderer> m_screenQuadRenderer;
  dp::FramebufferFallback m_framebufferFallback;
  PrerenderFrame m_prerenderFrame;
  drape_ptr<PostprocessStaticTextures> m_staticTextures;
  uint32_t m_width = 0;
  uint32_t m_height = 0;

  drape_ptr<dp::Framebuffer> m_mainFramebuffer;
  bool m_isMainFramebufferRendered = false;
  drape_ptr<dp::Framebuffer> m_edgesFramebuffer;
  drape_ptr<dp::Framebuffer> m_blendingWeightFramebuffer;
  drape_ptr<dp::Framebuffer> m_smaaFramebuffer;
  bool m_isSmaaFramebufferRendered = false;

  bool m_frameStarted = false;
  bool m_isRouteFollowingActive = false;
};

class StencilWriterGuard
{
public:
  StencilWriterGuard(ref_ptr<PostprocessRenderer> renderer, ref_ptr<dp::GraphicsContext> context);
  ~StencilWriterGuard();

private:
  ref_ptr<PostprocessRenderer> const m_renderer;
  ref_ptr<dp::GraphicsContext> const m_context;
};
}  // namespace df
