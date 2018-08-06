#include "drape_frontend/screen_quad_renderer.hpp"
#include "drape_frontend/render_state_extension.hpp"

#include "shaders/program_manager.hpp"

#include "drape/data_buffer.hpp"
#include "drape/glconstants.hpp"
#include "drape/glextensions_list.hpp"
#include "drape/glfunctions.hpp"

#include <vector>

namespace df
{
namespace
{
class TextureRendererContext : public RendererContext
{
public:
  gpu::Program GetGpuProgram() const override { return gpu::Program::ScreenQuad; }

  void PreRender(ref_ptr<gpu::ProgramManager> mng) override
  {
    auto prg = mng->GetProgram(GetGpuProgram());

    BindTexture(m_textureId, prg, "u_colorTex", 0 /* slotIndex */,
                gl_const::GLLinear, gl_const::GLClampToEdge);

    mng->GetParamsSetter()->Apply(prg, m_params);

    GLFunctions::glDisable(gl_const::GLDepthTest);
    GLFunctions::glEnable(gl_const::GLBlending);
  }

  void PostRender() override
  {
    GLFunctions::glDisable(gl_const::GLBlending);
    GLFunctions::glBindTexture(0);
  }

  void SetParams(uint32_t textureId, float opacity)
  {
    m_textureId = textureId;
    m_params.m_opacity = opacity;
  }

private:
  uint32_t m_textureId = 0;
  gpu::ScreenQuadProgramParams m_params;
};
}  // namespace

void RendererContext::BindTexture(uint32_t textureId, ref_ptr<dp::GpuProgram> prg,
                                  std::string const & uniformName, uint8_t slotIndex,
                                  uint32_t filteringMode, uint32_t wrappingMode)
{
  int8_t const textureLocation = prg->GetUniformLocation(uniformName);
  ASSERT_NOT_EQUAL(textureLocation, -1, ());
  if (textureLocation < 0)
    return;

  GLFunctions::glActiveTexture(gl_const::GLTexture0 + slotIndex);
  GLFunctions::glBindTexture(textureId);
  GLFunctions::glUniformValuei(textureLocation, slotIndex);
  GLFunctions::glTexParameter(gl_const::GLMinFilter, filteringMode);
  GLFunctions::glTexParameter(gl_const::GLMagFilter, filteringMode);
  GLFunctions::glTexParameter(gl_const::GLWrapS, wrappingMode);
  GLFunctions::glTexParameter(gl_const::GLWrapT, wrappingMode);
}

ScreenQuadRenderer::ScreenQuadRenderer()
  : TBase(DrawPrimitive::TriangleStrip)
  , m_textureRendererContext(make_unique_dp<TextureRendererContext>())
{
  Rebuild();
}

void ScreenQuadRenderer::Rebuild()
{
  std::vector<float> vertices = {-1.0f, 1.0f,  m_textureRect.minX(), m_textureRect.maxY(),
                                 1.0f,  1.0f,  m_textureRect.maxX(), m_textureRect.maxY(),
                                 -1.0f, -1.0f, m_textureRect.minX(), m_textureRect.minY(),
                                 1.0f,  -1.0f, m_textureRect.maxX(), m_textureRect.minY()};
  auto const bufferIndex = 0;
  SetBuffer(bufferIndex, std::move(vertices), sizeof(float) * 4 /* stride */);
  SetAttribute("a_pos", bufferIndex, 0 /* offset */, 2 /* componentsCount */);
  SetAttribute("a_tcoord", bufferIndex, sizeof(float) * 2 /* offset */, 2 /* componentsCount */);
}

void ScreenQuadRenderer::Render(ref_ptr<gpu::ProgramManager> mng, ref_ptr<RendererContext> context)
{
  ref_ptr<dp::GpuProgram> prg = mng->GetProgram(context->GetGpuProgram());
  TBase::Render(prg, [context, mng](){ context->PreRender(mng); }, [context](){ context->PostRender(); });
}

void ScreenQuadRenderer::RenderTexture(ref_ptr<gpu::ProgramManager> mng, uint32_t textureId,
                                       float opacity)
{
  ASSERT(dynamic_cast<TextureRendererContext *>(m_textureRendererContext.get()) != nullptr, ());

  auto context = static_cast<TextureRendererContext *>(m_textureRendererContext.get());
  context->SetParams(textureId, opacity);

  Render(mng, make_ref(m_textureRendererContext));
}

void ScreenQuadRenderer::SetTextureRect(m2::RectF const & rect)
{
  m_textureRect = rect;
  Rebuild();
}
}  // namespace df
