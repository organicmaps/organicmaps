#include "../../base/SRC_FIRST.hpp"
#include "../../yg/screen.hpp"
#include "../../yg/utils.hpp"
#include "../../yg/texture.hpp"
#include "../../std/shared_ptr.hpp"
#include "../../qt_tstfrm/macros.hpp"
#include "../../base/thread.hpp"

namespace
{
  struct RenderRoutine : public threads::IRoutine
  {
    shared_ptr<yg::gl::Screen> m_pScreen;
    shared_ptr<qt::gl::RenderContext> m_renderContext;
    int & m_globalCounter;

    RenderRoutine(shared_ptr<yg::gl::Screen> pScreen,
                  shared_ptr<qt::gl::RenderContext> renderContext,
                  int & globalCounter)
                    : m_pScreen(pScreen),
                    m_renderContext(renderContext),
                    m_globalCounter(globalCounter)
    {}

    void Do()
    {
      m_renderContext->makeCurrent();
      for (size_t i = 0; i < 30; ++i)
      {
        m_pScreen->beginFrame();
        m_pScreen->clear(yg::gl::Screen::s_bgColor);
        m_pScreen->immDrawRect(
            m2::RectF(i * 15 + 20, 10, i * 15 + 30, 20),
            m2::RectF(),
            shared_ptr<yg::gl::RGBA8Texture>(),
            false,
            yg::Color(0, 0, 255, (m_globalCounter++) * (255 / 60)),
            true);
        m_pScreen->endFrame();
      }
    }
  };

  struct TestThreadedRendering
  {
    shared_ptr<qt::gl::RenderContext> m_renderContext;
    int globalCounter;
    void Init(shared_ptr<qt::gl::RenderContext> renderContext)
    {
      m_renderContext = renderContext;
      globalCounter = 0;
    }

    void DoDraw(shared_ptr<yg::gl::Screen> p)
    {
      globalCounter = 0;
      threads::Thread thread;
      thread.Create(new RenderRoutine(p, m_renderContext, globalCounter));
      for (size_t i = 0; i < 30; ++i)
      {
        p->beginFrame();
        p->clear(yg::gl::Screen::s_bgColor);
        p->immDrawSolidRect(
            m2::RectF(i * 15 + 20, 30, i * 15 + 30, 40),
            yg::Color(0, 0, 255, (globalCounter++) * (255 / 60) ));

        p->endFrame();
      }
      thread.Join();
    }
  };

//  UNIT_TEST_GL(TestThreadedRendering);
}
