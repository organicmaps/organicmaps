#include "drape_frontend/postprocess_renderer.hpp"
#include "drape_frontend/render_state_extension.hpp"
#include "drape_frontend/screen_quad_renderer.hpp"

#include "shaders/program_manager.hpp"

#include "drape/glsl_types.hpp"
#include "drape/graphics_context.hpp"
#include "drape/mesh_object.hpp"
#include "drape/texture_manager.hpp"
#include "drape/render_state.hpp"

#include "base/assert.hpp"

namespace df
{
namespace
{
class SMAABaseRenderParams
{
public:
  SMAABaseRenderParams(gpu::Program program)
    : m_state(CreateRenderState(program, DepthLayer::GeometryLayer))
  {
    m_state.SetDepthTestEnabled(false);
    m_state.SetBlending(dp::Blending(false));
  }
  virtual ~SMAABaseRenderParams() = default;

  dp::RenderState const & GetRenderState() const { return m_state; }
  gpu::SMAAProgramParams const & GetProgramParams() const { return m_params; }

protected:
  dp::RenderState m_state;
  gpu::SMAAProgramParams m_params;
};

class EdgesRenderParams : public SMAABaseRenderParams
{
  using TBase = SMAABaseRenderParams;
public:
  EdgesRenderParams(): TBase(gpu::Program::SmaaEdges) {}

  void SetParams(ref_ptr<dp::Texture> texture, uint32_t width, uint32_t height)
  {
    m_state.SetTexture("u_colorTex", texture);

    m_params.m_framebufferMetrics = glsl::vec4(1.0f / width, 1.0f / height,
                                               static_cast<float>(width),
                                               static_cast<float>(height));
  }
};

class BlendingWeightRenderParams : public SMAABaseRenderParams
{
  using TBase = SMAABaseRenderParams;
public:
  BlendingWeightRenderParams() : TBase(gpu::Program::SmaaBlendingWeight) {}

  void SetParams(ref_ptr<dp::Texture> edgesTexture, ref_ptr<dp::Texture> areaTexture,
                 ref_ptr<dp::Texture> searchTexture, uint32_t width, uint32_t height)
  {
    m_state.SetTexture("u_colorTex", edgesTexture);
    m_state.SetTexture("u_smaaArea", areaTexture);
    m_state.SetTexture("u_smaaSearch", searchTexture);

    m_params.m_framebufferMetrics = glsl::vec4(1.0f / width, 1.0f / height,
                                               static_cast<float>(width),
                                               static_cast<float>(height));
  }
};

class SMAAFinalRenderParams : public SMAABaseRenderParams
{
  using TBase = SMAABaseRenderParams;
public:
  SMAAFinalRenderParams(): TBase(gpu::Program::SmaaFinal) {}

  void SetParams(ref_ptr<dp::Texture> colorTexture, ref_ptr<dp::Texture> blendingWeightTexture,
                 uint32_t width, uint32_t height)
  {
    m_state.SetTexture("u_colorTex", colorTexture);
    m_state.SetTexture("u_blendingWeightTex", blendingWeightTexture);

    m_params.m_framebufferMetrics = glsl::vec4(1.0f / width, 1.0f / height,
                                               static_cast<float>(width),
                                               static_cast<float>(height));
  }
};

class DefaultScreenQuadRenderParams
{
public:
  DefaultScreenQuadRenderParams()
    : m_state(CreateRenderState(gpu::Program::ScreenQuad, DepthLayer::GeometryLayer))
  {
    m_state.SetDepthTestEnabled(false);
    m_state.SetBlending(dp::Blending(false));
  }

  void SetParams(ref_ptr<dp::Texture> texture)
  {
    m_state.SetTexture("u_colorTex", texture);
  }

  dp::RenderState const & GetRenderState() const { return m_state; }
  gpu::ScreenQuadProgramParams const & GetProgramParams() const { return m_params; }

private:
  dp::RenderState m_state;
  gpu::ScreenQuadProgramParams m_params;
};

void InitFramebuffer(drape_ptr<dp::Framebuffer> & framebuffer, uint32_t width, uint32_t height,
                     bool depthEnabled, bool stencilEnabled)
{
  if (framebuffer == nullptr)
    framebuffer = make_unique_dp<dp::Framebuffer>(dp::TextureFormat::RGBA8, depthEnabled, stencilEnabled);
  framebuffer->SetSize(width, height);
}

void InitFramebuffer(drape_ptr<dp::Framebuffer> & framebuffer, dp::TextureFormat colorFormat,
                     ref_ptr<dp::Framebuffer::DepthStencil> depthStencilRef,
                     uint32_t width, uint32_t height)
{
  if (framebuffer == nullptr)
    framebuffer = make_unique_dp<dp::Framebuffer>(colorFormat);
  framebuffer->SetDepthStencilRef(depthStencilRef);
  framebuffer->SetSize(width, height);
}

bool IsSupported(drape_ptr<dp::Framebuffer> const & framebuffer)
{
  return framebuffer != nullptr && framebuffer->IsSupported();
}
}  // namespace

PostprocessRenderer::~PostprocessRenderer()
{
  ClearGLDependentResources();
}

void PostprocessRenderer::Init(dp::ApiVersion apiVersion, dp::FramebufferFallback && fallback)
{
  m_apiVersion = apiVersion;
  m_screenQuadRenderer = make_unique_dp<ScreenQuadRenderer>();
  m_framebufferFallback = std::move(fallback);
  ASSERT(m_framebufferFallback != nullptr, ());
}

void PostprocessRenderer::ClearGLDependentResources()
{
  m_screenQuadRenderer.reset();
  m_framebufferFallback = nullptr;
  m_staticTextures.reset();

  m_mainFramebuffer.reset();
  m_isMainFramebufferRendered = false;
  m_edgesFramebuffer.reset();
  m_blendingWeightFramebuffer.reset();
  m_smaaFramebuffer.reset();
  m_isSmaaFramebufferRendered = false;
}

void PostprocessRenderer::Resize(uint32_t width, uint32_t height)
{
  m_width = width;
  m_height = height;

  UpdateFramebuffers(m_width, m_height);
}

void PostprocessRenderer::SetStaticTextures(drape_ptr<PostprocessStaticTextures> && textures)
{
  m_staticTextures = std::move(textures);
}

bool PostprocessRenderer::IsEnabled() const
{
  // Do not use post processing in routing following mode by energy-saving reasons.
  if (m_isRouteFollowingActive)
    return false;

  return IsSupported(m_mainFramebuffer);
}

void PostprocessRenderer::SetEffectEnabled(Effect effect, bool enabled)
{
  // Do not support AA for OpenGLES 2.0.
  if (m_apiVersion == dp::ApiVersion::OpenGLES2 && effect == Effect::Antialiasing)
    return;

  auto const oldValue = m_effects;
  auto const effectMask = static_cast<uint32_t>(effect);
  m_effects = (m_effects & ~effectMask) | (enabled ? effectMask : 0);

  if (m_width != 0 && m_height != 0 && oldValue != m_effects)
    UpdateFramebuffers(m_width, m_height);
}

bool PostprocessRenderer::IsEffectEnabled(Effect effect) const
{
  return (m_effects & static_cast<uint32_t>(effect)) > 0;
}

bool PostprocessRenderer::CanRenderAntialiasing() const
{
  if (!IsEffectEnabled(Effect::Antialiasing))
    return false;

  if (!IsSupported(m_edgesFramebuffer) || !IsSupported(m_blendingWeightFramebuffer) ||
      !IsSupported(m_smaaFramebuffer))
  {
    return false;
  }

  if (m_staticTextures == nullptr)
    return false;

  return m_staticTextures->m_smaaSearchTexture != nullptr &&
         m_staticTextures->m_smaaAreaTexture != nullptr &&
         m_staticTextures->m_smaaAreaTexture->GetID() != 0 &&
         m_staticTextures->m_smaaSearchTexture->GetID() != 0;
}

bool PostprocessRenderer::BeginFrame(ref_ptr<dp::GraphicsContext> context, bool activeFrame)
{
  if (!IsEnabled())
  {
    CHECK(m_framebufferFallback != nullptr, ());
    return m_framebufferFallback();
  }

  m_frameStarted = activeFrame || !m_isMainFramebufferRendered;
  if (m_frameStarted)
    m_mainFramebuffer->Enable();

  if (m_frameStarted && CanRenderAntialiasing())
    context->SetStencilTestEnabled(false);

  m_isMainFramebufferRendered = true;
  return m_frameStarted;
}

bool PostprocessRenderer::EndFrame(ref_ptr<dp::GraphicsContext> context, ref_ptr<gpu::ProgramManager> gpuProgramManager)
{
  if (!IsEnabled())
    return true;

  // Subpixel Morphological Antialiasing (SMAA).
  if (m_frameStarted && CanRenderAntialiasing())
  {
    ASSERT(m_staticTextures->m_smaaAreaTexture != nullptr, ());
    ASSERT_GREATER(m_staticTextures->m_smaaAreaTexture->GetID(), 0, ());

    ASSERT(m_staticTextures->m_smaaSearchTexture != nullptr, ());
    ASSERT_GREATER(m_staticTextures->m_smaaSearchTexture->GetID(), 0, ());

    context->SetClearColor(dp::Color::Transparent());
    context->SetStencilTestEnabled(true);

    // Render edges to texture.
    {
      m_edgesFramebuffer->Enable();

      context->Clear(dp::ClearBits::ColorBit);

      context->SetStencilFunction(dp::StencilFace::FrontAndBack, dp::TestFunction::NotEqual);
      context->SetStencilActions(dp::StencilFace::FrontAndBack, dp::StencilAction::Zero,
                                 dp::StencilAction::Zero, dp::StencilAction::Replace);

      EdgesRenderParams params;
      params.SetParams(m_mainFramebuffer->GetTexture(), m_width, m_height);

      auto program = gpuProgramManager->GetProgram(params.GetRenderState().GetProgram<gpu::Program>());
      m_screenQuadRenderer->Render(context, program, params.GetRenderState(), gpuProgramManager->GetParamsSetter(),
                                   params.GetProgramParams());
    }

    // Render blending weight to texture.
    {
      m_blendingWeightFramebuffer->Enable();

      context->Clear(dp::ClearBits::ColorBit);

      context->SetStencilFunction(dp::StencilFace::FrontAndBack, dp::TestFunction::Equal);
      context->SetStencilActions(dp::StencilFace::FrontAndBack, dp::StencilAction::Keep,
                                 dp::StencilAction::Keep, dp::StencilAction::Keep);

      BlendingWeightRenderParams params;
      params.SetParams(m_edgesFramebuffer->GetTexture(),
                       m_staticTextures->m_smaaAreaTexture,
                       m_staticTextures->m_smaaSearchTexture,
                       m_width, m_height);

      auto program = gpuProgramManager->GetProgram(params.GetRenderState().GetProgram<gpu::Program>());
      m_screenQuadRenderer->Render(context, program, params.GetRenderState(), gpuProgramManager->GetParamsSetter(),
                                   params.GetProgramParams());
    }

    // SMAA final pass.
    context->SetStencilTestEnabled(false);
    {
      m_smaaFramebuffer->Enable();

      context->Clear(dp::ClearBits::ColorBit);

      SMAAFinalRenderParams params;
      params.SetParams(m_mainFramebuffer->GetTexture(),
                       m_blendingWeightFramebuffer->GetTexture(),
                       m_width, m_height);

      auto program = gpuProgramManager->GetProgram(params.GetRenderState().GetProgram<gpu::Program>());
      m_screenQuadRenderer->Render(context, program, params.GetRenderState(), gpuProgramManager->GetParamsSetter(),
                                   params.GetProgramParams());

      m_isSmaaFramebufferRendered = true;
    }
  }

  ref_ptr<dp::Framebuffer> finalFramebuffer;
  if (m_isSmaaFramebufferRendered)
    finalFramebuffer = make_ref(m_smaaFramebuffer);
  else
    finalFramebuffer = make_ref(m_mainFramebuffer);

  CHECK(m_framebufferFallback != nullptr, ());
  bool m_wasRendered = false;
  if (m_framebufferFallback())
  {
    DefaultScreenQuadRenderParams params;
    params.SetParams(finalFramebuffer->GetTexture());

    auto program = gpuProgramManager->GetProgram(params.GetRenderState().GetProgram<gpu::Program>());
    m_screenQuadRenderer->Render(context, program, params.GetRenderState(), gpuProgramManager->GetParamsSetter(),
                                 params.GetProgramParams());

    m_wasRendered = true;
  }
  m_frameStarted = false;
  return m_wasRendered;
}

void PostprocessRenderer::EnableWritingToStencil(ref_ptr<dp::GraphicsContext> context) const
{
  if (!m_frameStarted || !CanRenderAntialiasing())
    return;
  context->SetStencilTestEnabled(true);
  context->SetStencilFunction(dp::StencilFace::FrontAndBack, dp::TestFunction::Always);
  context->SetStencilActions(dp::StencilFace::FrontAndBack, dp::StencilAction::Keep,
                             dp::StencilAction::Keep, dp::StencilAction::Replace);
}

void PostprocessRenderer::DisableWritingToStencil(ref_ptr<dp::GraphicsContext> context) const
{
  if (!m_frameStarted || !CanRenderAntialiasing())
    return;
  context->SetStencilTestEnabled(false);
}

void PostprocessRenderer::UpdateFramebuffers(uint32_t width, uint32_t height)
{
  ASSERT_NOT_EQUAL(width, 0, ());
  ASSERT_NOT_EQUAL(height, 0, ());

  InitFramebuffer(m_mainFramebuffer, width, height,
                  true /* depthEnabled */,
                  m_apiVersion != dp::ApiVersion::OpenGLES2 /* stencilEnabled */);
  m_isMainFramebufferRendered = false;

  m_isSmaaFramebufferRendered = false;
  if (!m_isRouteFollowingActive && IsEffectEnabled(Effect::Antialiasing))
  {
    CHECK(m_apiVersion != dp::ApiVersion::OpenGLES2, ());

    InitFramebuffer(m_edgesFramebuffer, dp::TextureFormat::RedGreen,
                    m_mainFramebuffer->GetDepthStencilRef(),
                    width, height);
    InitFramebuffer(m_blendingWeightFramebuffer, dp::TextureFormat::RGBA8,
                    m_mainFramebuffer->GetDepthStencilRef(),
                    width, height);
    InitFramebuffer(m_smaaFramebuffer, dp::TextureFormat::RGBA8,
                    m_mainFramebuffer->GetDepthStencilRef(),
                    width, height);
  }
  else
  {
    m_edgesFramebuffer.reset();
    m_blendingWeightFramebuffer.reset();
    m_smaaFramebuffer.reset();
  }
}

bool PostprocessRenderer::OnFramebufferFallback()
{
  if (m_frameStarted)
  {
    m_mainFramebuffer->Enable();
    return true;
  }

  return m_framebufferFallback();
}

void PostprocessRenderer::OnChangedRouteFollowingMode(bool isRouteFollowingActive)
{
  if (m_isRouteFollowingActive == isRouteFollowingActive)
    return;

  m_isRouteFollowingActive = isRouteFollowingActive;
  if (m_width != 0 && m_height != 0)
    UpdateFramebuffers(m_width, m_height);
}

StencilWriterGuard::StencilWriterGuard(ref_ptr<PostprocessRenderer> renderer, ref_ptr<dp::GraphicsContext> context)
  : m_renderer(renderer)
  , m_context(context)
{
  ASSERT(m_renderer != nullptr, ());
  m_renderer->EnableWritingToStencil(m_context);
}

StencilWriterGuard::~StencilWriterGuard()
{
  m_renderer->DisableWritingToStencil(m_context);
}
}  // namespace df
