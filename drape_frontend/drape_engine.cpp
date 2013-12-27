#include "drape_engine.hpp"

#include "message_subclasses.hpp"

namespace df
{

DrapeEngine::DrapeEngine(RefPointer<OGLContextFactory> contextfactory, double vs, int w, int h)
{
  m_threadCommutator = MasterPointer<ThreadsCommutator>(new ThreadsCommutator());

  m_frontend = MasterPointer<FrontendRenderer>(
                 new FrontendRenderer(m_threadCommutator.GetRefPointer(), contextfactory, w, h));

  m_backend =  MasterPointer<BackendRenderer>(
                 new BackendRenderer(m_threadCommutator.GetRefPointer(), contextfactory, vs, w, h));
}

void DrapeEngine::OnSizeChanged(int x0, int y0, int w, int h)
{
  m_backend->Resize(x0, y0, w, h);
  m_threadCommutator->PostMessage(ThreadsCommutator::RenderThread
                                  ,MovePointer<Message>(new ResizeMessage(x0, y0, w, h)));
}

void DrapeEngine::SetAngle(float radians)
{
  m_threadCommutator->PostMessage(ThreadsCommutator::RenderThread
                                  , MovePointer<Message>(new RotateMessage(radians)));
}

}
