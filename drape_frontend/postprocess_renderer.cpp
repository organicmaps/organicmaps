#include "drape_frontend/postprocess_renderer.hpp"
#include "drape_frontend/render_state.hpp"
#include "drape_frontend/screen_quad_renderer.hpp"
#include "drape_frontend/shader_def.hpp"

#include "drape/glfunctions.hpp"
#include "drape/gpu_program_manager.hpp"
#include "drape/texture_manager.hpp"
#include "drape/uniform_values_storage.hpp"

#include "base/assert.hpp"

namespace df
{
namespace
{
class SMAABaseRendererContext : public RendererContext
{
protected:
  void ApplyFramebufferMetrics(ref_ptr<dp::GpuProgram> prg)
  {
    dp::UniformValuesStorage uniforms;
    uniforms.SetFloatValue("u_framebufferMetrics", 1.0f / m_width, 1.0f / m_height,
                           m_width, m_height);
    dp::ApplyUniforms(uniforms, prg);
  }

  float m_width = 1.0f;
  float m_height = 1.0f;
};

class EdgesRendererContext : public SMAABaseRendererContext
{
public:
  int GetGpuProgram() const override { return gpu::SMAA_EDGES_PROGRAM; }

  void PreRender(ref_ptr<dp::GpuProgram> prg) override
  {
    GLFunctions::glClear(gl_const::GLColorBit);

    BindTexture(m_textureId, prg, "u_colorTex", 0 /* slotIndex */,
                gl_const::GLLinear, gl_const::GLClampToEdge);
    ApplyFramebufferMetrics(prg);

    GLFunctions::glDisable(gl_const::GLDepthTest);
    GLFunctions::glDisable(gl_const::GLBlending);
  }

  void SetParams(uint32_t textureId, uint32_t width, uint32_t height)
  {
    m_textureId = textureId;
    m_width = static_cast<float>(width);
    m_height = static_cast<float>(height);
  }

protected:
  uint32_t m_textureId = 0;
};

class BlendingWeightRendererContext : public SMAABaseRendererContext
{
public:
  int GetGpuProgram() const override { return gpu::SMAA_BLENDING_WEIGHT_PROGRAM; }

  void PreRender(ref_ptr<dp::GpuProgram> prg) override
  {
    GLFunctions::glClear(gl_const::GLColorBit);

    BindTexture(m_edgesTextureId, prg, "u_colorTex", 0 /* slotIndex */,
                gl_const::GLLinear, gl_const::GLClampToEdge);
    BindTexture(m_areaTextureId, prg, "u_smaaArea", 1 /* slotIndex */,
                gl_const::GLLinear, gl_const::GLClampToEdge);
    BindTexture(m_searchTextureId, prg, "u_smaaSearch", 2 /* slotIndex */,
                gl_const::GLLinear, gl_const::GLClampToEdge);
    ApplyFramebufferMetrics(prg);

    GLFunctions::glDisable(gl_const::GLDepthTest);
    GLFunctions::glDisable(gl_const::GLBlending);
  }

  void PostRender() override
  {
    GLFunctions::glActiveTexture(gl_const::GLTexture0 + 2);
    GLFunctions::glBindTexture(0);
  }

  void SetParams(uint32_t edgesTextureId, uint32_t areaTextureId, uint32_t searchTextureId,
                 uint32_t width, uint32_t height)
  {
    m_edgesTextureId = edgesTextureId;
    m_areaTextureId = areaTextureId;
    m_searchTextureId = searchTextureId;
    m_width = static_cast<float>(width);
    m_height = static_cast<float>(height);
  }

private:
  uint32_t m_edgesTextureId = 0;
  uint32_t m_areaTextureId = 0;
  uint32_t m_searchTextureId = 0;
};

class SMAAFinalRendererContext : public SMAABaseRendererContext
{
public:
  int GetGpuProgram() const override { return gpu::SMAA_FINAL_PROGRAM; }

  void PreRender(ref_ptr<dp::GpuProgram> prg) override
  {
    GLFunctions::glClear(gl_const::GLColorBit);

    BindTexture(m_colorTextureId, prg, "u_colorTex", 0 /* slotIndex */,
                gl_const::GLLinear, gl_const::GLClampToEdge);
    BindTexture(m_blendingWeightTextureId, prg, "u_blendingWeightTex", 1 /* slotIndex */,
                gl_const::GLLinear, gl_const::GLClampToEdge);
    ApplyFramebufferMetrics(prg);

    GLFunctions::glDisable(gl_const::GLDepthTest);
    GLFunctions::glDisable(gl_const::GLBlending);
  }

  void PostRender() override
  {
    GLFunctions::glActiveTexture(gl_const::GLTexture0 + 1);
    GLFunctions::glBindTexture(0);
    GLFunctions::glActiveTexture(gl_const::GLTexture0);
    GLFunctions::glBindTexture(0);
  }

  void SetParams(uint32_t colorTextureId, uint32_t blendingWeightTextureId, uint32_t width,
                 uint32_t height)
  {
    m_colorTextureId = colorTextureId;
    m_blendingWeightTextureId = blendingWeightTextureId;
    m_width = static_cast<float>(width);
    m_height = static_cast<float>(height);
  }

private:
  uint32_t m_colorTextureId = 0;
  uint32_t m_blendingWeightTextureId = 0;
};

void InitFramebuffer(drape_ptr<dp::Framebuffer> & framebuffer, uint32_t width, uint32_t height)
{
  if (framebuffer == nullptr)
    framebuffer.reset(new dp::Framebuffer(gl_const::GLRGBA, true /* stencilEnabled */));
  framebuffer->SetSize(width, height);
}

void InitFramebuffer(drape_ptr<dp::Framebuffer> & framebuffer, uint32_t colorFormat,
                     ref_ptr<dp::Framebuffer::DepthStencil> depthStencilRef,
                     uint32_t width, uint32_t height)
{
  if (framebuffer == nullptr)
    framebuffer.reset(new dp::Framebuffer(colorFormat));
  framebuffer->SetDepthStencilRef(depthStencilRef);
  framebuffer->SetSize(width, height);
}

bool IsSupported(drape_ptr<dp::Framebuffer> const & framebuffer)
{
  return framebuffer != nullptr && framebuffer->IsSupported();
}
}  // namespace

PostprocessRenderer::PostprocessRenderer()
  : m_isEnabled(false)
  , m_effects(0)
  , m_width(0)
  , m_height(0)
  , m_edgesRendererContext(make_unique_dp<EdgesRendererContext>())
  , m_bwRendererContext(make_unique_dp<BlendingWeightRendererContext>())
  , m_smaaFinalRendererContext(make_unique_dp<SMAAFinalRendererContext>())
  , m_frameStarted(false)
{}

PostprocessRenderer::~PostprocessRenderer()
{
  ClearGLDependentResources();
}

void PostprocessRenderer::Init(dp::FramebufferFallback && fallback)
{
  m_screenQuadRenderer.reset(new ScreenQuadRenderer());
  m_framebufferFallback = std::move(fallback);
  ASSERT(m_framebufferFallback != nullptr, ());
}

void PostprocessRenderer::ClearGLDependentResources()
{
  m_screenQuadRenderer.reset();
  m_framebufferFallback = nullptr;
  m_staticTextures.reset();

  m_mainFramebuffer.reset();
  m_edgesFramebuffer.reset();
  m_blendingWeightFramebuffer.reset();
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

void PostprocessRenderer::SetEnabled(bool enabled)
{
  m_isEnabled = enabled;
  if (m_isEnabled && m_width != 0 && m_height != 0)
    UpdateFramebuffers(m_width, m_height);
}

bool PostprocessRenderer::IsEnabled() const
{
  if (!m_isEnabled || m_effects == 0 || m_staticTextures == nullptr)
    return false;

  if (!IsSupported(m_mainFramebuffer))
    return false;

  if (IsEffectEnabled(Effect::Antialiasing) &&
      (!IsSupported(m_edgesFramebuffer) || !IsSupported(m_blendingWeightFramebuffer)))
  {
    return false;
  }

  // Insert checking new effects here.

  return true;
}

void PostprocessRenderer::SetEffectEnabled(Effect effect, bool enabled)
{
  uint32_t const oldValue = m_effects;
  uint32_t const effectMask = static_cast<uint32_t>(effect);
  m_effects = (m_effects & ~effectMask) | (enabled ? effectMask : 0);

  if (m_width != 0 && m_height != 0 && oldValue != m_effects)
    UpdateFramebuffers(m_width, m_height);
}

bool PostprocessRenderer::IsEffectEnabled(Effect effect) const
{
  return (m_effects & static_cast<uint32_t>(effect)) > 0;
}

void PostprocessRenderer::BeginFrame()
{
  if (!IsEnabled())
  {
    m_framebufferFallback();
    return;
  }

  // Check if Subpixel Morphological Antialiasing (SMAA) is unavailable.
  ASSERT(m_staticTextures != nullptr, ());
  if (m_staticTextures->m_smaaSearchTexture == nullptr ||
      m_staticTextures->m_smaaAreaTexture == nullptr ||
      m_staticTextures->m_smaaAreaTexture->GetID() == 0 ||
      m_staticTextures->m_smaaSearchTexture->GetID() == 0)
  {
    SetEffectEnabled(Effect::Antialiasing, false);
  }

  m_mainFramebuffer->Enable();
  m_frameStarted = true;

  GLFunctions::glDisable(gl_const::GLStencilTest);
}

void PostprocessRenderer::EndFrame(ref_ptr<dp::GpuProgramManager> gpuProgramManager)
{
  if (!IsEnabled() && !m_frameStarted)
    return;

  bool wasPostEffect = false;

  // Subpixel Morphological Antialiasing (SMAA).
  if (IsEffectEnabled(Effect::Antialiasing))
  {
    wasPostEffect = true;

    ASSERT(m_staticTextures->m_smaaAreaTexture != nullptr, ());
    ASSERT_GREATER(m_staticTextures->m_smaaAreaTexture->GetID(), 0, ());

    ASSERT(m_staticTextures->m_smaaSearchTexture != nullptr, ());
    ASSERT_GREATER(m_staticTextures->m_smaaSearchTexture->GetID(), 0, ());

    GLFunctions::glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    GLFunctions::glEnable(gl_const::GLStencilTest);

    // Render edges to texture.
    {
      m_edgesFramebuffer->Enable();

      GLFunctions::glStencilFuncSeparate(gl_const::GLFrontAndBack, gl_const::GLNotEqual, 1, 1);
      GLFunctions::glStencilOpSeparate(gl_const::GLFrontAndBack, gl_const::GLZero,
                                       gl_const::GLZero, gl_const::GLReplace);

      ASSERT(dynamic_cast<EdgesRendererContext *>(m_edgesRendererContext.get()) != nullptr, ());
      auto context = static_cast<EdgesRendererContext *>(m_edgesRendererContext.get());
      context->SetParams(m_mainFramebuffer->GetTextureId(), m_width, m_height);
      m_screenQuadRenderer->Render(gpuProgramManager, make_ref(m_edgesRendererContext));
    }

    // Render blending weight to texture.
    {
      m_blendingWeightFramebuffer->Enable();

      GLFunctions::glStencilFuncSeparate(gl_const::GLFrontAndBack, gl_const::GLEqual, 1, 1);
      GLFunctions::glStencilOpSeparate(gl_const::GLFrontAndBack, gl_const::GLKeep,
                                       gl_const::GLKeep, gl_const::GLKeep);

      ASSERT(dynamic_cast<BlendingWeightRendererContext *>(m_bwRendererContext.get()) != nullptr, ());
      auto context = static_cast<BlendingWeightRendererContext *>(m_bwRendererContext.get());
      context->SetParams(m_edgesFramebuffer->GetTextureId(),
                         m_staticTextures->m_smaaAreaTexture->GetID(),
                         m_staticTextures->m_smaaSearchTexture->GetID(),
                         m_width, m_height);
      m_screenQuadRenderer->Render(gpuProgramManager, make_ref(m_bwRendererContext));
    }

    // SMAA final pass.
    GLFunctions::glDisable(gl_const::GLStencilTest);
    {
      m_framebufferFallback();
      ASSERT(dynamic_cast<SMAAFinalRendererContext *>(m_smaaFinalRendererContext.get()) != nullptr, ());
      auto context = static_cast<SMAAFinalRendererContext *>(m_smaaFinalRendererContext.get());
      context->SetParams(m_mainFramebuffer->GetTextureId(),
                         m_blendingWeightFramebuffer->GetTextureId(),
                         m_width, m_height);
      m_screenQuadRenderer->Render(gpuProgramManager, make_ref(m_smaaFinalRendererContext));
    }
  }

  if (!wasPostEffect)
  {
    m_framebufferFallback();
    GLFunctions::glClear(gl_const::GLColorBit);
    m_screenQuadRenderer->RenderTexture(gpuProgramManager, m_mainFramebuffer->GetTextureId(),
                                        1.0f /* opacity */);
  }
  m_frameStarted = false;
}

void PostprocessRenderer::EnableWritingToStencil() const
{
  if (!m_frameStarted)
    return;
  GLFunctions::glEnable(gl_const::GLStencilTest);
  GLFunctions::glStencilFuncSeparate(gl_const::GLFrontAndBack, gl_const::GLAlways, 1, 1);
  GLFunctions::glStencilOpSeparate(gl_const::GLFrontAndBack, gl_const::GLKeep,
                                   gl_const::GLKeep, gl_const::GLReplace);
}

void PostprocessRenderer::DisableWritingToStencil() const
{
  if (!m_frameStarted)
    return;
  GLFunctions::glDisable(gl_const::GLStencilTest);
}

void PostprocessRenderer::UpdateFramebuffers(uint32_t width, uint32_t height)
{
  ASSERT_NOT_EQUAL(width, 0, ());
  ASSERT_NOT_EQUAL(height, 0, ());

  if (m_effects != 0)
    InitFramebuffer(m_mainFramebuffer, width, height);
  else
    m_mainFramebuffer.reset();

  if (IsEffectEnabled(Effect::Antialiasing))
  {
    InitFramebuffer(m_edgesFramebuffer, gl_const::GLRedGreen,
                    m_mainFramebuffer->GetDepthStencilRef(),
                    width, height);
    InitFramebuffer(m_blendingWeightFramebuffer, gl_const::GLRGBA,
                    m_mainFramebuffer->GetDepthStencilRef(),
                    width, height);
  }
  else
  {
    m_edgesFramebuffer.reset();
    m_blendingWeightFramebuffer.reset();
  }
}

void PostprocessRenderer::OnFramebufferFallback()
{
  if (m_frameStarted)
    m_mainFramebuffer->Enable();
  else
    m_framebufferFallback();
}

StencilWriterGuard::StencilWriterGuard(ref_ptr<PostprocessRenderer> renderer)
  : m_renderer(renderer)
{
  ASSERT(m_renderer != nullptr, ());
  m_renderer->EnableWritingToStencil();
}

StencilWriterGuard::~StencilWriterGuard()
{
  m_renderer->DisableWritingToStencil();
}
}  // namespace df
