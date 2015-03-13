#include "drape_frontend/drape_engine.hpp"

#include "drape_frontend/message_subclasses.hpp"
#include "drape_frontend/visual_params.hpp"

#include "drape_gui/drape_gui.hpp"

#include "drape/texture_manager.hpp"

#include "std/bind.hpp"
#include "std/condition_variable.hpp"
#include "std/mutex.hpp"

namespace df
{

DrapeEngine::DrapeEngine(dp::RefPointer<dp::OGLContextFactory> contextfactory,
                         Viewport const & viewport,
                         MapDataProvider const & model,
                         double vs)
  : m_viewport(viewport)
{
  VisualParams::Init(vs, df::CalculateTileSize(m_viewport.GetWidth(), m_viewport.GetHeight()));

  gui::DrapeGui::TScaleFactorFn scaleFn = []
  {
    return VisualParams::Instance().GetVisualScale();
  };
  gui::DrapeGui::TGeneralizationLevelFn gnLvlFn = [](ScreenBase const & screen)
  {
    return GetDrawTileScale(screen);
  };

  gui::DrapeGui::Instance().Init(scaleFn, gnLvlFn);

  m_textureManager = dp::MasterPointer<dp::TextureManager>(new dp::TextureManager());
  m_threadCommutator = dp::MasterPointer<ThreadsCommutator>(new ThreadsCommutator());
  dp::RefPointer<ThreadsCommutator> commutatorRef = m_threadCommutator.GetRefPointer();

  m_frontend = dp::MasterPointer<FrontendRenderer>(new FrontendRenderer(commutatorRef,
                                                                        contextfactory,
                                                                        m_textureManager.GetRefPointer(),
                                                                        m_viewport));
  m_backend =  dp::MasterPointer<BackendRenderer>(new BackendRenderer(commutatorRef,
                                                                      contextfactory,
                                                                      m_textureManager.GetRefPointer(),
                                                                      model));
}

DrapeEngine::~DrapeEngine()
{
  m_frontend.Destroy();
  m_backend.Destroy();
  m_threadCommutator.Destroy();
  m_textureManager.Destroy();
}

void DrapeEngine::Resize(int w, int h)
{
  if (m_viewport.GetWidth() == w && m_viewport.GetHeight() == h)
    return;

  m_viewport.SetViewport(0, 0, w, h);
  m_threadCommutator->PostMessage(ThreadsCommutator::RenderThread,
                                  dp::MovePointer<Message>(new ResizeMessage(m_viewport)),
                                  MessagePriority::High);
}

void DrapeEngine::UpdateCoverage(ScreenBase const & screen)
{
  m_threadCommutator->PostMessage(ThreadsCommutator::RenderThread,
                                  dp::MovePointer<Message>(new UpdateModelViewMessage(screen)),
                                  MessagePriority::Normal);
}

void DrapeEngine::ClearUserMarksLayer(df::TileKey const & tileKey)
{
  m_threadCommutator->PostMessage(ThreadsCommutator::RenderThread,
                                  dp::MovePointer<Message>(new ClearUserMarkLayerMessage(tileKey)),
                                  MessagePriority::Normal);
}

void DrapeEngine::ChangeVisibilityUserMarksLayer(TileKey const & tileKey, bool isVisible)
{
  m_threadCommutator->PostMessage(ThreadsCommutator::RenderThread,
                                  dp::MovePointer<Message>(new ChangeUserMarkLayerVisibilityMessage(tileKey, isVisible)),
                                  MessagePriority::Normal);
}

void DrapeEngine::UpdateUserMarksLayer(TileKey const & tileKey, UserMarksProvider * provider)
{
  m_threadCommutator->PostMessage(ThreadsCommutator::ResourceUploadThread,
                                  dp::MovePointer<Message>(new UpdateUserMarkLayerMessage(tileKey, provider)),
                                  MessagePriority::Normal);
}

void DrapeEngine::SetRenderingEnabled(bool const isEnabled)
{
  SetRenderingEnabled(m_frontend.GetRefPointer(), isEnabled);
  SetRenderingEnabled(m_backend.GetRefPointer(), isEnabled);

  LOG(LDEBUG, (isEnabled ? "Rendering enabled" : "Rendering disabled"));
}

void DrapeEngine::SetRenderingEnabled(dp::RefPointer<BaseRenderer> renderer, bool const isEnabled)
{
  // here we have to wait for finishing of message processing
  mutex completionMutex;
  condition_variable completionCondition;
  bool notified = false;
  auto completionHandler = [&]()
  {
    unique_lock<mutex> lock(completionMutex);
    notified = true;
    completionCondition.notify_one();
  };

  renderer->SetRenderingEnabled(isEnabled, completionHandler);

  unique_lock<mutex> lock(completionMutex);
  while (!notified) // loop to avoid spurious wakeups
  {
    completionCondition.wait(lock);
  }
}

} // namespace df
