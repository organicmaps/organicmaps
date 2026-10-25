#include "testing/testing.hpp"

#include "gtk/gl_factory.hpp"

#include "drape/gl_functions.hpp"

#include <gtk/gtk.h>

#include <array>
#include <memory>
#include <optional>
#include <set>
#include <sstream>
#include <string>

namespace gl_factory_tests
{
using Color = std::array<int, 3>;

Color constexpr kRed = {255, 0, 0};
Color constexpr kGreen = {0, 255, 0};
Color constexpr kBlue = {0, 0, 255};
Color constexpr kWhite = {255, 255, 255};

struct ShownFrame
{
  bool operator==(ShownFrame const &) const = default;

  GLuint m_texture = 0;
  GLint m_width = 0;
  GLint m_height = 0;
  Color m_color{};
};

std::string DebugPrint(ShownFrame const & frame)
{
  std::ostringstream out;
  out << "ShownFrame{texture " << frame.m_texture << ", " << frame.m_width << "x" << frame.m_height << ", rgb "
      << ::DebugPrint(frame.m_color) << "}";
  return out.str();
}

// Drives both sides of the frame exchange from one thread: the render context is used as drape
// uses it, and a second context of the display composes frames as the GtkGLArea does.
class FrameExchange
{
public:
  FrameExchange(uint32_t w, uint32_t h)
  {
    TEST(gtk_init_check(), ("Run this test through CTest or run_window_tests.sh"));
    GdkDisplay * display = gdk_display_get_default();
    m_factory = std::make_unique<gtk::GtkContextFactory>(display, w, h, [this]() { ++m_published; });
    TEST(m_factory->IsValid(), ());
    m_producer = m_factory->GetDrawContext();

    GError * error = nullptr;
    m_consumer = gdk_display_create_gl_context(display, &error);
    TEST(m_consumer, (error ? error->message : ""));
    gdk_gl_context_set_allowed_apis(m_consumer, GDK_GL_API_GLES);
    TEST(gdk_gl_context_realize(m_consumer, &error), (error ? error->message : ""));
    int major = 0, minor = 0;
    gdk_gl_context_get_version(m_consumer, &major, &minor);
    // Texture size queries need OpenGL ES 3.1.
    TEST_GREATER_OR_EQUAL(major * 10 + minor, 31, ());

    m_producer->MakeCurrent();
    GLFunctions::Init(dp::ApiVersion::OpenGLES3);
  }

  ~FrameExchange()
  {
    m_factory.reset();
    g_object_unref(m_consumer);
  }

  void Resize(uint32_t w, uint32_t h)
  {
    m_producer->MakeCurrent();
    m_producer->Resize(w, h);
    CheckNoError();
  }

  // Renders a frame as drape does: binds the default framebuffer, draws and presents.
  void Present(Color const & color)
  {
    auto const target = BackTarget();
    TEST_NOT_EQUAL(target.m_depth, 0, ());
    TEST_EQUAL(target.m_stencil, target.m_depth, ());
    TEST_EQUAL(target.m_depthWidth, target.m_width, ());
    TEST_EQUAL(target.m_depthHeight, target.m_height, ());
    TEST_EQUAL(glCheckFramebufferStatus(GL_FRAMEBUFFER), GL_FRAMEBUFFER_COMPLETE, ());
    glClearColor(color[0] / 255.f, color[1] / 255.f, color[2] / 255.f, 1.f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    m_producer->Present();
    CheckNoError();
  }

  struct Target
  {
    GLint m_fbo = 0;
    GLint m_width = 0;
    GLint m_height = 0;
    GLint m_depth = 0;
    GLint m_stencil = 0;
    GLint m_depthWidth = 0;
    GLint m_depthHeight = 0;
  };

  // Binds the framebuffer the next frame is rendered to, as drape does, and describes it.
  Target BackTarget()
  {
    m_producer->MakeCurrent();
    CheckNoError();
    m_producer->SetFramebuffer(nullptr);
    Target target;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &target.m_fbo);
    GLint texture = 0;
    glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME,
                                          &texture);
    glBindTexture(GL_TEXTURE_2D, static_cast<GLuint>(texture));
    glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &target.m_width);
    glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &target.m_height);
    glBindTexture(GL_TEXTURE_2D, 0);
    glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME,
                                          &target.m_depth);
    glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME,
                                          &target.m_stencil);
    glBindRenderbuffer(GL_RENDERBUFFER, static_cast<GLuint>(target.m_depth));
    glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_WIDTH, &target.m_depthWidth);
    glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_HEIGHT, &target.m_depthHeight);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);
    CheckNoError();
    return target;
  }

  // Composes the current frame and reads back what the window would show.
  std::optional<ShownFrame> Draw()
  {
    gdk_gl_context_make_current(m_consumer);
    std::optional<ShownFrame> shown;
    bool const drawn = m_factory->DrawFrame(1'000'000 / 60, [&shown](GLuint texture)
    {
      shown.emplace();
      shown->m_texture = texture;
      glBindTexture(GL_TEXTURE_2D, texture);
      glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &shown->m_width);
      glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &shown->m_height);
      glBindTexture(GL_TEXTURE_2D, 0);

      GLuint fbo = 0;
      glGenFramebuffers(1, &fbo);
      glBindFramebuffer(GL_FRAMEBUFFER, fbo);
      glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);
      std::array<uint8_t, 4> rgba{};
      glReadPixels(shown->m_width / 2, shown->m_height / 2, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, rgba.data());
      shown->m_color = {rgba[0], rgba[1], rgba[2]};
      glBindFramebuffer(GL_FRAMEBUFFER, 0);
      glDeleteFramebuffers(1, &fbo);
    });
    CheckNoError();
    TEST_EQUAL(drawn, shown.has_value(), ());
    gdk_gl_context_clear_current();
    return shown;
  }

  int Published() const { return m_published; }

private:
  static void CheckNoError()
  {
    GLenum const error = glGetError();
    TEST_EQUAL(error, GL_NO_ERROR, ());
  }

  std::unique_ptr<gtk::GtkContextFactory> m_factory;
  dp::GraphicsContext * m_producer = nullptr;
  GdkGLContext * m_consumer = nullptr;
  int m_published = 0;
};

void TestShown(std::optional<ShownFrame> const & shown, Color const & color, GLint w, GLint h)
{
  TEST(shown, ());
  TEST_EQUAL(shown->m_color, color, (*shown));
  TEST_EQUAL(shown->m_width, w, (*shown));
  TEST_EQUAL(shown->m_height, h, (*shown));
}

UNIT_TEST(OffscreenFrames_KeepShownFrameAcrossResize)
{
  FrameExchange frames(64, 48);
  TEST(!frames.Draw(), ("Nothing is shown before the first frame"));
  frames.Resize(64, 48);
  TEST(!frames.Draw(), ());

  frames.Present(kRed);
  auto const red = frames.Draw();
  TestShown(red, kRed, 64, 48);

  // GTK renders after a resize before the engine presents a frame of the new size.
  frames.Resize(96, 80);
  TEST_EQUAL(frames.Draw(), red, ());
  frames.Resize(40, 120);
  frames.Resize(100, 50);
  TEST_EQUAL(frames.Draw(), red, ());
  TEST_EQUAL(frames.Draw(), red, ());

  frames.Present(kGreen);
  auto const green = frames.Draw();
  TestShown(green, kGreen, 100, 50);
  TEST_NOT_EQUAL(green->m_texture, red->m_texture, ());
  TEST_EQUAL(frames.Draw(), green, ());
  TEST_EQUAL(frames.Published(), 2, ());
}

UNIT_TEST(OffscreenFrames_PublishedFrameSurvivesResize)
{
  FrameExchange frames(64, 48);
  frames.Present(kRed);
  TestShown(frames.Draw(), kRed, 64, 48);

  // Published before the resize but not shown yet: still newer than the shown frame.
  frames.Present(kBlue);
  frames.Resize(100, 50);
  auto const blue = frames.Draw();
  TestShown(blue, kBlue, 64, 48);
  TEST_EQUAL(frames.Draw(), blue, ());

  frames.Present(kGreen);
  TestShown(frames.Draw(), kGreen, 100, 50);
}

UNIT_TEST(OffscreenFrames_ReusedFramesGetNewSize)
{
  FrameExchange frames(64, 48);
  std::array<Color, 4> constexpr kColors = {kRed, kGreen, kBlue, kWhite};
  std::set<GLuint> oldTextures;
  for (size_t i = 0; i < 6; ++i)
  {
    frames.Present(kColors[i % kColors.size()]);
    auto const shown = frames.Draw();
    TestShown(shown, kColors[i % kColors.size()], 64, 48);
    oldTextures.insert(shown->m_texture);
  }
  TEST_EQUAL(oldTextures.size(), 3, ());

  // Every frame, including the shown one, comes back to the producer through the front slot.
  frames.Resize(100, 50);
  std::set<GLuint> newTextures;
  for (size_t i = 0; i < 6; ++i)
  {
    frames.Present(kColors[i % kColors.size()]);
    // A frame that is never shown is recycled as well.
    if (i == 3)
      frames.Present(kRed);
    auto const shown = frames.Draw();
    TestShown(shown, i == 3 ? kRed : kColors[i % kColors.size()], 100, 50);
    newTextures.insert(shown->m_texture);
  }
  TEST_EQUAL(newTextures, oldTextures, ());
  TEST_EQUAL(frames.Published(), 13, ());
}

UNIT_TEST(OffscreenFrames_ShareDepthStencil)
{
  FrameExchange frames(64, 48);
  std::set<GLint> fbos;
  std::set<GLint> depthBuffers;
  for (auto const & [w, h] : {std::pair{64, 48}, std::pair{100, 50}})
  {
    frames.Resize(w, h);
    for (Color const & color : {kRed, kGreen, kBlue, kWhite})
    {
      auto const target = frames.BackTarget();
      fbos.insert(target.m_fbo);
      depthBuffers.insert(target.m_depth);
      frames.Present(color);
      TestShown(frames.Draw(), color, w, h);
    }
  }
  TEST_EQUAL(fbos.size(), 3, ());
  TEST_EQUAL(depthBuffers.size(), 1, ());
}
}  // namespace gl_factory_tests
