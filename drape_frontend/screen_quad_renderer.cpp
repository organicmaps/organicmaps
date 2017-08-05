#include "drape_frontend/screen_quad_renderer.hpp"
#include "drape_frontend/render_state.hpp"
#include "drape_frontend/shader_def.hpp"

#include "drape/data_buffer.hpp"
#include "drape/glconstants.hpp"
#include "drape/glextensions_list.hpp"
#include "drape/glfunctions.hpp"
#include "drape/gpu_program_manager.hpp"
#include "drape/uniform_values_storage.hpp"

#include <vector>

namespace df
{
namespace
{
class TextureRendererContext : public RendererContext
{
public:
  int GetGpuProgram() const override { return gpu::SCREEN_QUAD_PROGRAM; }

  void PreRender(ref_ptr<dp::GpuProgram> prg) override
  {
    BindTexture(m_textureId, prg, "u_colorTex", 0 /* slotIndex */,
                gl_const::GLLinear, gl_const::GLClampToEdge);

    dp::UniformValuesStorage uniforms;
    uniforms.SetFloatValue("u_opacity", m_opacity);
    dp::ApplyUniforms(uniforms, prg);

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
    m_opacity = opacity;
  }

private:
  uint32_t m_textureId = 0;
  float m_opacity = 1.0f;
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
  : m_textureRendererContext(make_unique_dp<TextureRendererContext>())
{}

ScreenQuadRenderer::~ScreenQuadRenderer()
{
  if (m_bufferId != 0)
    GLFunctions::glDeleteBuffer(m_bufferId);

  if (m_VAO != 0)
    GLFunctions::glDeleteVertexArray(m_VAO);
}

void ScreenQuadRenderer::Build(ref_ptr<dp::GpuProgram> prg)
{
  if (dp::GLExtensionsList::Instance().IsSupported(dp::GLExtensionsList::VertexArrayObject))
  {
    m_VAO = GLFunctions::glGenVertexArray();
    GLFunctions::glBindVertexArray(m_VAO);
  }
  m_attributePosition = prg->GetAttributeLocation("a_pos");
  ASSERT_NOT_EQUAL(m_attributePosition, -1, ());

  m_attributeTexCoord = prg->GetAttributeLocation("a_tcoord");
  ASSERT_NOT_EQUAL(m_attributeTexCoord, -1, ());
  
  std::vector<float> vertices = {-1.0f, 1.0f,  m_textureRect.minX(), m_textureRect.maxY(),
                                 1.0f,  1.0f,  m_textureRect.maxX(), m_textureRect.maxY(),
                                 -1.0f, -1.0f, m_textureRect.minX(), m_textureRect.minY(),
                                 1.0f,  -1.0f, m_textureRect.maxX(), m_textureRect.minY()};

  m_bufferId = GLFunctions::glGenBuffer();
  GLFunctions::glBindBuffer(m_bufferId, gl_const::GLArrayBuffer);
  GLFunctions::glBufferData(gl_const::GLArrayBuffer,
                            static_cast<uint32_t>(vertices.size()) * sizeof(vertices[0]),
                            vertices.data(), gl_const::GLStaticDraw);
  if (dp::GLExtensionsList::Instance().IsSupported(dp::GLExtensionsList::VertexArrayObject))
    GLFunctions::glBindVertexArray(0);
  GLFunctions::glBindBuffer(0, gl_const::GLArrayBuffer);
}

void ScreenQuadRenderer::Render(ref_ptr<dp::GpuProgramManager> mng, ref_ptr<RendererContext> context)
{
  ref_ptr<dp::GpuProgram> prg = mng->GetProgram(context->GetGpuProgram());
  prg->Bind();

  if (m_bufferId == 0)
    Build(prg);

  if (m_VAO != 0)
    GLFunctions::glBindVertexArray(m_VAO);

  GLFunctions::glBindBuffer(m_bufferId, gl_const::GLArrayBuffer);

  GLFunctions::glEnableVertexAttribute(m_attributePosition);
  GLFunctions::glVertexAttributePointer(m_attributePosition, 2, gl_const::GLFloatType, false,
                                        sizeof(float) * 4, 0);
  GLFunctions::glEnableVertexAttribute(m_attributeTexCoord);
  GLFunctions::glVertexAttributePointer(m_attributeTexCoord, 2, gl_const::GLFloatType, false,
                                        sizeof(float) * 4, sizeof(float) * 2);

  context->PreRender(prg);
  GLFunctions::glDrawArrays(gl_const::GLTriangleStrip, 0, 4);
  context->PostRender();

  prg->Unbind();
  GLFunctions::glBindBuffer(0, gl_const::GLArrayBuffer);

  if (m_VAO != 0)
    GLFunctions::glBindVertexArray(0);
}

void ScreenQuadRenderer::RenderTexture(ref_ptr<dp::GpuProgramManager> mng, uint32_t textureId,
                                       float opacity)
{
  ASSERT(dynamic_cast<TextureRendererContext *>(m_textureRendererContext.get()) != nullptr, ());

  auto context = static_cast<TextureRendererContext *>(m_textureRendererContext.get());
  context->SetParams(textureId, opacity);

  Render(mng, make_ref(m_textureRendererContext));
}

void ScreenQuadRenderer::SetTextureRect(m2::RectF const & rect, ref_ptr<dp::GpuProgram> prg)
{
  m_textureRect = rect;
  Rebuild(prg);
}

void ScreenQuadRenderer::Rebuild(ref_ptr<dp::GpuProgram> prg)
{
  if (m_bufferId != 0)
    GLFunctions::glDeleteBuffer(m_bufferId);

  prg->Bind();
  Build(prg);
  prg->Unbind();
}
}  // namespace df
