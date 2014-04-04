#include "drape_engine.hpp"

#include "message_subclasses.hpp"
#include "visual_params.hpp"

#include "../drape/texture_manager.hpp"

#include "../std/bind.hpp"

namespace df
{
  void Dummy() {}
  DrapeEngine::DrapeEngine(RefPointer<OGLContextFactory> contextfactory, double vs, Viewport const & viewport)
    : m_viewport(viewport)
    , m_navigator(m_scales, bind(&Dummy))
  {
    GLFunctions::Init();
    VisualParams::Init(vs, df::CalculateTileSize(m_viewport.GetWidth(), m_viewport.GetHeight()));
    m_navigator.LoadState();
    m_navigator.OnSize(0, 0, m_viewport.GetWidth(), m_viewport.GetHeight());

    m_textures.Reset(new TextureManager());
    RefPointer<TextureSetHolder>     textureHolder = m_textures.GetRefPointer();
    TextureSetBinder * binder = new TextureSetBinder(m_textures.GetRefPointer());

    m_threadCommutator = MasterPointer<ThreadsCommutator>(new ThreadsCommutator());
    RefPointer<ThreadsCommutator> commutatorRef = m_threadCommutator.GetRefPointer();

    m_frontend = MasterPointer<FrontendRenderer>(new FrontendRenderer(commutatorRef,
                                                                      contextfactory,
                                                                      MovePointer<TextureSetController>(binder),
                                                                      m_viewport));
    m_backend =  MasterPointer<BackendRenderer>(new BackendRenderer(commutatorRef,
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
                                    MovePointer<Message>(new ResizeMessage(m_viewport)));
    UpdateCoverage();
  }

  void DrapeEngine::DragStarted(const m2::PointF & p)
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

  void DrapeEngine::Scale(const m2::PointF & p, double factor)
  {
    m_navigator.ScaleToPoint(p, factor, 0.0);
    UpdateCoverage();
  }

  void DrapeEngine::UpdateCoverage()
  {
    m_threadCommutator->PostMessage(ThreadsCommutator::RenderThread,
                                    MovePointer<Message>(new UpdateCoverageMessage(m_navigator.Screen())));
  }
}
