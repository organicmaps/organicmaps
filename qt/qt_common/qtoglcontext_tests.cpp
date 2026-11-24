#include "testing/testing.hpp"

#include "qt/qt_common/frame_blit.hpp"
#include "qt/qt_common/qtoglcontext.hpp"

#include "qt_tstfrm/test_main_loop.hpp"

#include <QImage>
#include <QOpenGLExtraFunctions>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLVertexArrayObject>
#include <QtResource>

#include <array>

static void InitFrameBlitResources()
{
  Q_INIT_RESOURCE(resources_common);
}

namespace qtoglcontext_tests
{
using qt::common::FrameBlit;
using qt::common::QtRenderOGLContext;

struct Environment
{
  QOpenGLContext * root = QOpenGLContext::currentContext();
  QOffscreenSurface * surface = static_cast<QOffscreenSurface *>(root->surface());
  QtRenderOGLContext context{root, surface};

  Environment()
  {
    context.MakeCurrent();
    context.Init(dp::ApiVersion::OpenGLES3);
  }

  ~Environment() { context.MakeCurrent(); }

  void Resize(uint32_t width, uint32_t height)
  {
    context.MakeCurrent();
    context.Resize(width, height);
  }

  void Present(bool red)
  {
    context.MakeCurrent();
    context.SetFramebuffer(nullptr);
    auto * functions = QOpenGLContext::currentContext()->functions();
    functions->glDisable(GL_SCISSOR_TEST);
    functions->glClearColor(red ? 1.0f : 0.0f, 0.0f, red ? 0.0f : 1.0f, 1.0f);
    functions->glClear(GL_COLOR_BUFFER_BIT);
    context.Present();
  }

  GLuint Acquire()
  {
    TEST(root->makeCurrent(surface), ());
    return context.AcquireFrameTexture();
  }

  void CheckFrame(GLuint texture, GLint width, GLint height, bool red)
  {
    TEST_NOT_EQUAL(texture, 0u, ());
    auto * functions = root->extraFunctions();
    functions->glBindTexture(GL_TEXTURE_2D, texture);
    GLint actualWidth = 0;
    GLint actualHeight = 0;
    functions->glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &actualWidth);
    functions->glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &actualHeight);
    functions->glBindTexture(GL_TEXTURE_2D, 0);
    TEST_EQUAL(actualWidth, width, ());
    TEST_EQUAL(actualHeight, height, ());

    GLuint framebuffer = 0;
    functions->glGenFramebuffers(1, &framebuffer);
    functions->glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    functions->glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);
    TEST_EQUAL(functions->glCheckFramebufferStatus(GL_FRAMEBUFFER), static_cast<GLenum>(GL_FRAMEBUFFER_COMPLETE), ());
    std::array<uint8_t, 4> pixel{};
    functions->glReadPixels(0, 0, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel.data());
    functions->glBindFramebuffer(GL_FRAMEBUFFER, 0);
    functions->glDeleteFramebuffers(1, &framebuffer);
    TEST_EQUAL(pixel[0], red ? 255 : 0, ());
    TEST_EQUAL(pixel[2], red ? 0 : 255, ());
  }
};

UNIT_TEST(QtFrameBlit_PaintsAcquiredTextureAndClearsWithoutOne)
{
  InitFrameBlitResources();
  RunTestInOpenGLOffscreenEnvironment("Blit acquired frame", []
  {
    int constexpr kWidth = 13;
    int constexpr kHeight = 29;
    Environment env;
    env.Resize(kWidth, kHeight);
    env.context.SetFramebuffer(nullptr);

    auto * producer = QOpenGLContext::currentContext()->functions();
    producer->glDisable(GL_SCISSOR_TEST);
    producer->glClearColor(0.0, 0.0, 1.0, 1.0);
    producer->glClear(GL_COLOR_BUFFER_BIT);
    producer->glEnable(GL_SCISSOR_TEST);
    producer->glScissor(0, kHeight / 2, kWidth, kHeight - kHeight / 2);
    producer->glClearColor(1.0, 0.0, 0.0, 1.0);
    producer->glClear(GL_COLOR_BUFFER_BIT);
    producer->glDisable(GL_SCISSOR_TEST);
    env.context.Present();

    GLuint const texture = env.Acquire();
    TEST_NOT_EQUAL(texture, 0u, ());
    QOpenGLFramebufferObject output(kWidth, kHeight);
    TEST(output.bind(), ());
    env.root->functions()->glViewport(0, 0, kWidth, kHeight);
    FrameBlit blit;
    blit.Draw(texture);
    auto const image = output.toImage();
    TEST_EQUAL(image.pixel(0, 0), qRgb(255, 0, 0), ());
    TEST_EQUAL(image.pixel(kWidth - 1, kHeight - 1), qRgb(0, 0, 255), ());

    TEST(output.bind(), ());
    blit.Draw(0);
    auto const cleared = output.toImage();
    TEST_EQUAL(cleared.pixel(0, 0), qRgb(0, 0, 0), ());
    TEST_EQUAL(cleared.pixel(kWidth - 1, kHeight - 1), qRgb(0, 0, 0), ());
  });
}

UNIT_TEST(QtRenderContext_KeepsAcquiredFrameUntilReplacementIsPresented)
{
  RunTestInOpenGLOffscreenEnvironment("Keep acquired frame", []
  {
    Environment env;
    TEST_EQUAL(env.Acquire(), 0u, ());
    env.Resize(13, 29);
    TEST_EQUAL(env.Acquire(), 0u, ());
    env.Present(true);
    GLuint const texture = env.Acquire();
    env.CheckFrame(texture, 13, 29, true);

    env.Resize(31, 13);
    // Allocate the replacement before checking that the acquired frame survives.
    env.context.SetFramebuffer(nullptr);
    GLuint const retainedTexture = env.Acquire();
    TEST_EQUAL(retainedTexture, texture, ());
    env.CheckFrame(retainedTexture, 13, 29, true);

    env.Present(false);
    env.CheckFrame(env.Acquire(), 31, 13, false);
  });
}

UNIT_TEST(QtRenderContext_PendingFrameKeepsItsSizeAcrossResize)
{
  RunTestInOpenGLOffscreenEnvironment("Pending frame size", []
  {
    Environment env;
    env.Resize(13, 29);
    env.Present(true);
    env.Resize(31, 13);
    env.CheckFrame(env.Acquire(), 13, 29, true);
    env.Present(false);
    env.CheckFrame(env.Acquire(), 31, 13, false);
  });
}

UNIT_TEST(QtRenderContext_RecyclesResizedFramesAndKeepsLatestPresentation)
{
  RunTestInOpenGLOffscreenEnvironment("Recycle resized frames", []
  {
    Environment env;
    for (int cycle = 0; cycle < 3; ++cycle)
    {
      env.Resize(13, 29);
      env.Present(true);
      env.Resize(70, 15);
      env.Present(false);
      env.CheckFrame(env.Acquire(), 70, 15, false);
      env.Resize(5, 7);
      env.Present(true);
      env.CheckFrame(env.Acquire(), 5, 7, true);
    }
  });
}

UNIT_TEST(QtRenderContext_FinishesConsumerSamplingBeforeRecycling)
{
  RunTestInOpenGLOffscreenEnvironment("Finish consumer sampling", []
  {
    Environment env;
    env.Resize(64, 64);
    env.Present(true);
    GLuint const texture = env.Acquire();
    TEST_NOT_EQUAL(texture, 0u, ());

    QOpenGLFramebufferObject output(QSize(512, 512));
    TEST(output.isValid(), ());
    QOpenGLShaderProgram program;
    auto const version = env.root->isOpenGLES() ? "#version 300 es\nprecision highp float;\n" : "#version 150\n";
    TEST(program.addShaderFromSourceCode(QOpenGLShader::Vertex, QByteArray(version) + R"(
      out vec2 uv;
      void main()
      {
        vec2 p = vec2((gl_VertexID << 1) & 2, gl_VertexID & 2);
        uv = p;
        gl_Position = vec4(p * 2.0 - 1.0, 0.0, 1.0);
      })"),
         (program.log().toStdString()));
    TEST(program.addShaderFromSourceCode(QOpenGLShader::Fragment, QByteArray(version) + R"(
      uniform sampler2D image;
      in vec2 uv;
      out vec4 color;
      void main()
      {
        vec4 sum = vec4(0.0);
        for (int i = 0; i < 128; ++i)
          sum += texture(image, fract(uv + vec2(float(i) * 0.0031, float(i) * 0.0017)));
        color = sum / 128.0;
      })"),
         (program.log().toStdString()));
    TEST(program.link(), (program.log().toStdString()));
    QOpenGLVertexArrayObject vao;
    TEST(vao.create(), ());

    // Publish before queuing sampling so producer completion cannot also finish the consumer's reads.
    env.Present(false);
    TEST(env.root->makeCurrent(env.surface), ());
    TEST(output.bind(), ());
    TEST(program.bind(), ());
    vao.bind();
    auto * functions = env.root->extraFunctions();
    functions->glDisable(GL_CULL_FACE);
    functions->glDisable(GL_DEPTH_TEST);
    functions->glDisable(GL_SCISSOR_TEST);
    functions->glViewport(0, 0, output.width(), output.height());
    functions->glActiveTexture(GL_TEXTURE0);
    functions->glBindTexture(GL_TEXTURE_2D, texture);
    program.setUniformValue("image", 0);
    // Queue enough sampling to exercise an outstanding consumer read without a synchronous readback.
    for (int i = 0; i < 6; ++i)
      functions->glDrawArrays(GL_TRIANGLES, 0, 3);
    auto completion = functions->glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
    TEST(completion, ());
    functions->glFlush();
    GLuint const nextTexture = env.context.AcquireFrameTexture();
    auto const status = functions->glClientWaitSync(completion, 0, 0);
    functions->glDeleteSync(completion);
    TEST_NOT_EQUAL(nextTexture, 0u, ());
    TEST_NOT_EQUAL(nextTexture, texture, ());
    TEST(status == GL_ALREADY_SIGNALED || status == GL_CONDITION_SATISFIED, (status));
    program.release();
    vao.release();

    // Two producer frames return the sampled texture to the back slot and overwrite it.
    env.Present(false);
    env.Present(false);
    TEST(env.root->makeCurrent(env.surface), ());
    auto const color = output.toImage().pixelColor(128, 128);
    TEST_EQUAL(color.red(), 255, ());
    TEST_EQUAL(color.blue(), 0, ());
    TEST_EQUAL(functions->glGetError(), static_cast<GLenum>(GL_NO_ERROR), ());
  });
}
}  // namespace qtoglcontext_tests
