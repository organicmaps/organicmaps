#include "drape_frontend/drape_frontend_tests/shape_test_fixture.hpp"

#include "drape_frontend/shape_view_params.hpp"
#include "drape_frontend/visual_params.hpp"

#include "drape/gl_functions.hpp"
#include "drape/gl_includes.hpp"
#include "drape/oglcontext.hpp"
#include "drape/support_manager.hpp"

#include "qt_tstfrm/test_main_loop.hpp"

#include "platform/platform.hpp"

#include "base/file_name_utils.hpp"

#include <QtGui/QPainter>

namespace df::test_support
{
namespace
{
/// Lightweight OGLContext wrapper that assumes a GL context is already current
/// (e.g., set up by RunTestInOpenGLOffscreenEnvironment).
class CurrentOGLContext : public dp::OGLContext
{
public:
  void MakeCurrent() override {}
  void DoneCurrent() override {}
  void Present() override { GLFunctions::glFinish(); }

  void SetFramebuffer(ref_ptr<dp::BaseFramebuffer>) override {}
  void ForgetFramebuffer(ref_ptr<dp::BaseFramebuffer>) override {}
  void ApplyFramebuffer(std::string const &) override {}
};
}  // namespace

void ShapeTestFixture::Render(char const * title, uint32_t width, uint32_t height, ShapeCreatorFn const & createShapes)
{
  RunTestInOpenGLOffscreenEnvironment(title, [&]()
  {
    if (!Init(width, height))
      return;

    createShapes(*this);

    Flush();
    Render();
    ReleaseGLResources();
  });

  RunTestLoop(title, [this](QPaintDevice * device)
  {
    if (!m_lastImage.isNull())
    {
      QPainter painter(device);
      painter.fillRect(QRectF(0, 0, device->width(), device->height()), Qt::darkGray);
      painter.drawImage(0, 0, m_lastImage);
    }
  }, true /* autoExit */);  // pass false if you need to inspect locally
}

void ShapeTestFixture::ReleaseGLResources()
{
  m_buckets.clear();
  m_batcher.reset();
  m_texMng.reset();
  m_progMng.reset();

  if (m_fbo != 0)
  {
    glDeleteFramebuffers(1, &m_fbo);
    glDeleteRenderbuffers(1, &m_colorRbo);
    glDeleteRenderbuffers(1, &m_depthRbo);
    m_fbo = 0;
  }

  m_context.reset();
}

bool ShapeTestFixture::Init(uint32_t width, uint32_t height)
{
  m_width = width;
  m_height = height;

  // Assumes a GL context is already current (from RunTestInOpenGLOffscreenEnvironment).
  auto ctx = std::make_unique<CurrentOGLContext>();
  ctx->Init(dp::ApiVersion::OpenGLES3);

  dp::SupportManager::Instance().Init(make_ref(ctx));

  m_context = std::move(ctx);
  auto const contextRef = make_ref(m_context);

  // Create FBO for offscreen rendering.
  glGenFramebuffers(1, &m_fbo);
  glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);

  glGenRenderbuffers(1, &m_colorRbo);
  glBindRenderbuffer(GL_RENDERBUFFER, m_colorRbo);
  glRenderbufferStorage(GL_RENDERBUFFER, GL_RGBA8, width, height);
  glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, m_colorRbo);

  glGenRenderbuffers(1, &m_depthRbo);
  glBindRenderbuffer(GL_RENDERBUFFER, m_depthRbo);
  glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, width, height);
  glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, m_depthRbo);

  if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
  {
    LOG(LWARNING, ("FBO incomplete, skipping visual test"));
    return false;
  }

  m_context->SetViewport(0, 0, width, height);
  m_context->SetDepthTestEnabled(true);
  m_context->SetCullingEnabled(false);
  m_context->SetClearColor(dp::Color::White());
  m_context->Clear(dp::ClearBits::ColorBit | dp::ClearBits::DepthBit, 0);

  // Initialize ProgramManager (compiles all shaders).
  m_progMng = std::make_unique<gpu::ProgramManager>();
  m_progMng->Init(contextRef);

  // Initialize TextureManager with real data.
  m_texMng = std::make_unique<dp::TextureManager>();
  dp::TextureManager::Params texParams;
  texParams.m_resPostfix = df::VisualParams::GetResourcePostfix(1.0);
  texParams.m_visualScale = 1.0;
  texParams.m_colors = "colors.txt";
  texParams.m_patterns = "patterns.txt";
  texParams.m_glyphMngParams.m_uniBlocks = base::JoinPath("fonts", "unicode_blocks.txt");
  texParams.m_glyphMngParams.m_whitelist = base::JoinPath("fonts", "whitelist.txt");
  texParams.m_glyphMngParams.m_blacklist = base::JoinPath("fonts", "blacklist.txt");
  GetPlatform().GetFontNames(texParams.m_glyphMngParams.m_fonts);
  m_texMng->Init(contextRef, texParams);

  // Upload textures to GPU (creates hardware texture objects).
  m_texMng->UpdateDynamicTextures(contextRef);

  // Create batcher.
  uint32_t constexpr kBatchSize = 5000;
  m_batcher = std::make_unique<dp::Batcher>(kBatchSize, kBatchSize);
  m_batcher->StartSession([this](dp::RenderState const & state, drape_ptr<dp::RenderBucket> && bucket)
  { m_buckets.push_back({state, std::move(bucket)}); });

  return true;
}

void ShapeTestFixture::AddShape(drape_ptr<MapShape> && shape)
{
  CHECK(m_batcher != nullptr, ("Call Init() before AddShape()"));
  shape->Draw(make_ref(m_context), make_ref(m_batcher), make_ref(m_texMng));
}

void ShapeTestFixture::Flush()
{
  CHECK(m_batcher != nullptr, ("Call Init() before Flush()"));
  m_batcher->EndSession(make_ref(m_context));

  // Upload any new texture data that was requested during AddShape (color/stipple regions).
  m_texMng->UpdateDynamicTextures(make_ref(m_context));
}

void ShapeTestFixture::Render()
{
  auto const contextRef = make_ref(m_context);

  glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
  m_context->SetClearColor(dp::Color::White());
  m_context->Clear(dp::ClearBits::ColorBit | dp::ClearBits::DepthBit, 0);

  // Coordinate pipeline:
  //   world coords -> ToShapeVertex2: (world - tileCenter) * kShapeCoordScalar -> shape coords
  //   shape coords -> modelView -> pixel coords (1 unit = 1 pixel from center)
  //   pixel coords -> projection -> NDC [-1, 1]
  float const halfW = static_cast<float>(m_width) / 2.0f;
  float const halfH = static_cast<float>(m_height) / 2.0f;
  float const invScale = 1.0f / static_cast<float>(kShapeCoordScalar);

  gpu::MapProgramParams params;
  params.m_modelView = glsl::mat4(invScale, 0, 0, 0, 0, invScale, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1);
  params.m_projection = glsl::mat4(1.0f / halfW, 0, 0, 0, 0, 1.0f / halfH, 0, 0, 0, 0, -0.5f, 0, 0, 0, 0.5f, 1);
  params.m_pivotTransform = glsl::mat4(1.0f);
  params.m_opacity = 1.0f;
  params.m_zScale = 1.0f;

  for (auto & entry : m_buckets)
  {
    auto const programId = entry.m_state.GetProgram<gpu::Program>();
    auto program = m_progMng->GetProgram(programId);
    program->Bind();
    dp::ApplyState(contextRef, program, entry.m_state);

    entry.m_bucket->GetBuffer()->Build(contextRef, program);
    m_progMng->GetParamsSetter()->Apply(contextRef, program, params);
    entry.m_bucket->Render(contextRef, entry.m_state.GetDrawAsLine());
  }

  m_context->Flush();

  // Read pixels from FBO.
  std::vector<uint8_t> pixels(m_width * m_height * 4);
  glReadPixels(0, 0, m_width, m_height, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());

  // glReadPixels returns bottom-up; QImage expects top-down.
  QImage img(m_width, m_height, QImage::Format_RGBA8888);
  for (uint32_t y = 0; y < m_height; ++y)
  {
    uint8_t const * src = pixels.data() + (m_height - 1 - y) * m_width * 4;
    memcpy(img.scanLine(y), src, m_width * 4);
  }

  m_lastImage = img;
}

}  // namespace df::test_support
