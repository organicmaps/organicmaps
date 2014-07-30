#include "drape_engine.hpp"

#include "message_subclasses.hpp"
#include "visual_params.hpp"

#include "../drape/texture_manager.hpp"

#include "../std/bind.hpp"

namespace df
{

DrapeEngine::DrapeEngine(dp::RefPointer<dp::OGLContextFactory> contextfactory, double vs, Viewport const & viewport)
  : m_viewport(viewport)
  , m_navigator(m_scales)
{
  GLFunctions::Init();
  VisualParams::Init(vs, df::CalculateTileSize(m_viewport.GetWidth(), m_viewport.GetHeight()));
  m_navigator.LoadState();
  m_navigator.OnSize(0, 0, m_viewport.GetWidth(), m_viewport.GetHeight());

  m_textures.Reset(new dp::TextureManager());
  dp::RefPointer<dp::TextureSetHolder> textureHolder = m_textures.GetRefPointer();
  dp::TextureSetBinder * binder = new dp::TextureSetBinder(m_textures.GetRefPointer());

  m_threadCommutator = dp::MasterPointer<ThreadsCommutator>(new ThreadsCommutator());
  dp::RefPointer<ThreadsCommutator> commutatorRef = m_threadCommutator.GetRefPointer();

  m_frontend = dp::MasterPointer<FrontendRenderer>(new FrontendRenderer(commutatorRef,
                                                                        contextfactory,
                                                                        dp::MovePointer<dp::TextureSetController>(binder),
                                                                        m_viewport));
  m_backend =  dp::MasterPointer<BackendRenderer>(new BackendRenderer(commutatorRef,
                                                                      contextfactory,
                                                                      textureHolder));

  UpdateCoverage();
}

DrapeEngine::~DrapeEngine()
{
  m_navigator.SaveState();
  m_frontend.Destroy();
  m_backend.Destroy();
  m_textures.Destroy();
  m_threadCommutator.Destroy();
}

void DrapeEngine::Resize(int w, int h)
{
  if (m_viewport.GetWidth() == w && m_viewport.GetHeight() == h)
    return;

  m_viewport.SetViewport(0, 0, w, h);
  m_navigator.OnSize(0, 0, w, h);
  m_threadCommutator->PostMessage(ThreadsCommutator::RenderThread,
                                  dp::MovePointer<Message>(new ResizeMessage(m_viewport)));
}

void DrapeEngine::DragStarted(m2::PointF const & p)
{
  m_navigator.StartDrag(p, 0.0);
  UpdateCoverage();
}

void DrapeEngine::Drag(m2::PointF const & p)
{
  m_navigator.DoDrag(p, 0.0);
  UpdateCoverage();
}

void DrapeEngine::DragEnded(m2::PointF const & p)
{
  m_navigator.StopDrag(p, 0.0, false);
  UpdateCoverage();
}

void DrapeEngine::Scale(m2::PointF const & p, double factor)
{
  m_navigator.ScaleToPoint(p, factor, 0.0);
  UpdateCoverage();
}

void DrapeEngine::UpdateCoverage()
{
  m_threadCommutator->PostMessage(ThreadsCommutator::RenderThread,
                                  dp::MovePointer<Message>(new UpdateModelViewMessage(m_navigator.Screen())));
}

} // namespace df
