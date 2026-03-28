#include "drape_frontend/drape_frontend_tests/shape_test_fixture.hpp"

#include "drape_frontend/shape_view_params.hpp"
#include "drape_frontend/visual_params.hpp"

#include "drape/gl_functions.hpp"
#include "drape/gl_includes.hpp"
#include "drape/oglcontext.hpp"
#include "drape/support_manager.hpp"

#include "platform/platform.hpp"

#include "base/file_name_utils.hpp"

#include <QtCore/QTimer>
#include <QtGui/QOffscreenSurface>
#include <QtGui/QOpenGLContext>
#include <QtGui/QPainter>
#include <QtWidgets/QApplication>
#include <QtWidgets/QWidget>

namespace df::test_support
{
namespace
{
/// Ensures a QApplication exists (required before any Qt GUI objects).
/// Intentionally leaked to avoid crash in Qt's static cleanup hooks at exit.
void EnsureQApplication()
{
  if (QApplication::instance() == nullptr)
  {
    static int argc = 1;
    static char const * argv[] = {"shape_test"};
    [[maybe_unused]] static auto * app = new QApplication(argc, const_cast<char **>(argv));
  }
}

/// Minimal OGLContext subclass backed by a QOpenGLContext + QOffscreenSurface.
class TestOGLContext : public dp::OGLContext
{
public:
  TestOGLContext()
  {
    EnsureQApplication();
    QSurfaceFormat fmt;
    fmt.setVersion(4, 1);
    fmt.setProfile(QSurfaceFormat::CoreProfile);
    fmt.setDepthBufferSize(24);
    fmt.setRedBufferSize(8);
    fmt.setGreenBufferSize(8);
    fmt.setBlueBufferSize(8);
    fmt.setAlphaBufferSize(8);

    m_surface = std::make_unique<QOffscreenSurface>();
    m_surface->setFormat(fmt);
    m_surface->create();
    CHECK(m_surface->isValid(), ("Failed to create offscreen surface"));

    m_glContext = std::make_unique<QOpenGLContext>();
    m_glContext->setFormat(fmt);
    m_glContext->create();
    CHECK(m_glContext->isValid(), ("Failed to create OpenGL context"));
  }

  void MakeCurrent() override { m_glContext->makeCurrent(m_surface.get()); }
  void DoneCurrent() override { m_glContext->doneCurrent(); }
  void Present() override { GLFunctions::glFinish(); }

  void SetFramebuffer(ref_ptr<dp::BaseFramebuffer>) override {}
  void ForgetFramebuffer(ref_ptr<dp::BaseFramebuffer>) override {}
  void ApplyFramebuffer(std::string const &) override {}

private:
  std::unique_ptr<QOffscreenSurface> m_surface;
  std::unique_ptr<QOpenGLContext> m_glContext;
};
}  // namespace

ShapeTestFixture::ShapeTestFixture() = default;

ShapeTestFixture::~ShapeTestFixture()
{
  if (m_fbo != 0)
  {
    glDeleteFramebuffers(1, &m_fbo);
    glDeleteRenderbuffers(1, &m_colorRbo);
    glDeleteRenderbuffers(1, &m_depthRbo);
  }
}

void ShapeTestFixture::Init(uint32_t width, uint32_t height)
{
  m_width = width;
  m_height = height;

  // 1. Create real OpenGL context.
  auto ctx = std::make_unique<TestOGLContext>();
  ctx->MakeCurrent();
  ctx->Init(dp::ApiVersion::OpenGLES3);

  // Initialize SupportManager (queries GL caps).
  dp::SupportManager::Instance().Init(make_ref(ctx));

  m_context = std::move(ctx);
  auto const contextRef = make_ref(m_context);

  // 2. Create FBO for offscreen rendering.
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

  CHECK_EQUAL(glCheckFramebufferStatus(GL_FRAMEBUFFER), GL_FRAMEBUFFER_COMPLETE, ("FBO incomplete"));

  glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);

  m_context->SetViewport(0, 0, width, height);
  m_context->SetDepthTestEnabled(true);
  m_context->SetCullingEnabled(false);
  m_context->SetClearColor(dp::Color::White());
  m_context->Clear(dp::ClearBits::ColorBit | dp::ClearBits::DepthBit, 0);

  // 3. Initialize ProgramManager (compiles all shaders).
  m_progMng = std::make_unique<gpu::ProgramManager>();
  m_progMng->Init(contextRef);

  // 4. Initialize TextureManager with real data.
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

  // 5. Create batcher.
  uint32_t constexpr kBatchSize = 5000;
  m_batcher = std::make_unique<dp::Batcher>(kBatchSize, kBatchSize);
  m_batcher->StartSession([this](dp::RenderState const & state, drape_ptr<dp::RenderBucket> && bucket)
  { m_buckets.push_back({state, std::move(bucket)}); });
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

QImage ShapeTestFixture::Render()
{
  auto const contextRef = make_ref(m_context);

  glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
  m_context->SetClearColor(dp::Color::White());
  m_context->Clear(dp::ClearBits::ColorBit | dp::ClearBits::DepthBit, 0);

  // Coordinate pipeline:
  //   world coords → ToShapeVertex2: (world - tileCenter) * kShapeCoordScalar → shape coords
  //   shape coords → modelView → pixel coords (1 unit = 1 pixel from center)
  //   pixel coords → projection → NDC [-1, 1]
  //
  // modelView undoes kShapeCoordScalar so that world coords map 1:1 to pixels.
  // projection maps pixel range [-halfW, halfW] to NDC.
  // Line width normals are expanded in modelView output space (pixels), so they stay correct.
  float const halfW = static_cast<float>(m_width) / 2.0f;
  float const halfH = static_cast<float>(m_height) / 2.0f;
  float const invScale = 1.0f / static_cast<float>(kShapeCoordScalar);

  gpu::MapProgramParams params;
  // modelView: shape coords → pixel coords (undo kShapeCoordScalar).
  params.m_modelView = glsl::mat4(invScale, 0, 0, 0, 0, invScale, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1);
  // Orthographic projection: pixel coords → NDC.
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
  return img;
}

void ShapeTestFixture::ShowInWindow(char const * title, bool autoExit)
{
  QImage img = m_lastImage.isNull() ? Render() : m_lastImage;

  EnsureQApplication();

  class ImageWidget : public QWidget
  {
    QImage m_img;

  public:
    explicit ImageWidget(QImage const & img) : m_img(img) { setFixedSize(img.size()); }

  protected:
    void paintEvent(QPaintEvent *) override
    {
      QPainter painter(this);
      painter.fillRect(rect(), Qt::darkGray);
      painter.drawImage(0, 0, m_img);
    }
  };

  ImageWidget widget(img);
  widget.setWindowTitle(title);
  widget.show();

  if (autoExit)
    QTimer::singleShot(3000, QApplication::instance(), SLOT(quit()));

  QApplication::instance()->exec();
}
}  // namespace df::test_support
