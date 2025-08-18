#include "drape_frontend/screen_quad_renderer.hpp"
#include "drape_frontend/render_state_extension.hpp"

#include "shaders/program_manager.hpp"

#include "drape/data_buffer.hpp"

#include <vector>

namespace df
{
namespace
{
class TextureRenderParams
{
public:
  TextureRenderParams() : m_state(CreateRenderState(gpu::Program::ScreenQuad, DepthLayer::GeometryLayer))
  {
    m_state.SetDepthTestEnabled(false);
    m_state.SetBlending(dp::Blending(true));
  }

  void SetParams(ref_ptr<gpu::ProgramManager> gpuProgramManager, ref_ptr<dp::Texture> texture, float opacity,
                 bool invertV)
  {
    UNUSED_VALUE(gpuProgramManager);
    m_state.SetTexture("u_colorTex", std::move(texture));
    m_params.m_opacity = opacity;
    m_params.m_invertV = invertV ? 1.0f : 0.0f;
  }

  dp::RenderState const & GetRenderState() const { return m_state; }
  gpu::ScreenQuadProgramParams const & GetProgramParams() const { return m_params; }

private:
  dp::RenderState m_state;
  gpu::ScreenQuadProgramParams m_params;
};
}  // namespace

ScreenQuadRenderer::ScreenQuadRenderer(ref_ptr<dp::GraphicsContext> context)
  : Base(context, DrawPrimitive::TriangleStrip)
{
  Rebuild(context);
}

void ScreenQuadRenderer::Rebuild(ref_ptr<dp::GraphicsContext> context)
{
  std::vector<float> vertices;
  vertices.reserve(4);
  if (context->GetApiVersion() == dp::ApiVersion::Vulkan)
  {
    vertices = {-1.0f, -1.0f, m_textureRect.minX(), m_textureRect.minY(),
                1.0f,  -1.0f, m_textureRect.maxX(), m_textureRect.minY(),
                -1.0f, 1.0f,  m_textureRect.minX(), m_textureRect.maxY(),
                1.0f,  1.0f,  m_textureRect.maxX(), m_textureRect.maxY()};
  }
  else
  {
    vertices = {-1.0f, 1.0f,  m_textureRect.minX(), m_textureRect.maxY(),
                1.0f,  1.0f,  m_textureRect.maxX(), m_textureRect.maxY(),
                -1.0f, -1.0f, m_textureRect.minX(), m_textureRect.minY(),
                1.0f,  -1.0f, m_textureRect.maxX(), m_textureRect.minY()};
  }
  auto const bufferIndex = 0;
  SetBuffer(bufferIndex, std::move(vertices), sizeof(float) * 4 /* stride */);
  SetAttribute("a_pos", bufferIndex, 0 /* offset */, 2 /* componentsCount */);
  SetAttribute("a_tcoord", bufferIndex, sizeof(float) * 2 /* offset */, 2 /* componentsCount */);
}

void ScreenQuadRenderer::RenderTexture(ref_ptr<dp::GraphicsContext> context, ref_ptr<gpu::ProgramManager> mng,
                                       ref_ptr<dp::Texture> texture, float opacity, bool invertV)
{
  TextureRenderParams params;
  params.SetParams(mng, std::move(texture), opacity, invertV);

  auto program = mng->GetProgram(params.GetRenderState().GetProgram<gpu::Program>());
  Base::Render(context, program, params.GetRenderState(), mng->GetParamsSetter(), params.GetProgramParams());
}

void ScreenQuadRenderer::SetTextureRect(ref_ptr<dp::GraphicsContext> context, m2::RectF const & rect)
{
  m_textureRect = rect;
  Rebuild(context);
}
}  // namespace df
