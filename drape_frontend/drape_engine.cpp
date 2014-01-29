#include "drape_engine.hpp"

#include "message_subclasses.hpp"
#include "vizualization_params.hpp"

namespace df
{
  DrapeEngine::DrapeEngine(RefPointer<OGLContextFactory> contextfactory, double vs, Viewport const & viewport)
  {
    GLFunctions::Init();
    VizualizationParams::SetVisualScale(vs);

    m_threadCommutator = MasterPointer<ThreadsCommutator>(new ThreadsCommutator());
    RefPointer<ThreadsCommutator> commutatorRef = m_threadCommutator.GetRefPointer();

    m_frontend = MasterPointer<FrontendRenderer>(new FrontendRenderer(commutatorRef, contextfactory, viewport));
    m_backend =  MasterPointer<BackendRenderer>(new BackendRenderer(commutatorRef, contextfactory, viewport));
  }

  DrapeEngine::~DrapeEngine()
  {
    m_backend.Destroy();
    m_frontend.Destroy();
    m_threadCommutator.Destroy();
  }

  void DrapeEngine::OnSizeChanged(int x0, int y0, int w, int h)
  {
    m_threadCommutator->PostMessage(ThreadsCommutator::RenderThread,
                                    MovePointer<Message>(new ResizeMessage(x0, y0, w, h)));
  }

  void DrapeEngine::SetAngle(float radians)
  {
    m_threadCommutator->PostMessage(ThreadsCommutator::RenderThread,
                                    MovePointer<Message>(new RotateMessage(radians)));
  }
}
