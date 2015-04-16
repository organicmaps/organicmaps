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

DrapeEngine::DrapeEngine(Params const & params)
  : m_viewport(params.m_viewport)
{
  VisualParams::Init(params.m_vs, df::CalculateTileSize(m_viewport.GetWidth(), m_viewport.GetHeight()));

  gui::DrapeGui::TScaleFactorFn scaleFn = []
  {
    return VisualParams::Instance().GetVisualScale();
  };
  gui::DrapeGui::TGeneralizationLevelFn gnLvlFn = [](ScreenBase const & screen)
  {
    return GetDrawTileScale(screen);
  };

  gui::DrapeGui & guiSubsystem = gui::DrapeGui::Instance();
  guiSubsystem.Init(scaleFn, gnLvlFn);
  guiSubsystem.SetLocalizator(bind(&StringsBundle::GetString, static_cast<StringsBundle*>(params.m_stringsBundle), _1));
  guiSubsystem.SetStorageAccessor(params.m_storageAccessor);

  m_textureManager = move(make_unique_dp<dp::TextureManager>());
  m_threadCommutator = move(make_unique_dp<ThreadsCommutator>());

  m_frontend = move(make_unique_dp<FrontendRenderer>(make_ref<ThreadsCommutator>(m_threadCommutator), params.m_factory,
                                                     make_ref<dp::TextureManager>(m_textureManager), m_viewport));

  m_backend = move(make_unique_dp<BackendRenderer>(make_ref<ThreadsCommutator>(m_threadCommutator), params.m_factory,
                                                   make_ref<dp::TextureManager>(m_textureManager), params.m_model));
}

DrapeEngine::~DrapeEngine()
{
  // reset pointers explicitly! We must wait for threads completion
  m_frontend.reset();
  m_backend.reset();
  m_threadCommutator.reset();

  gui::DrapeGui::Instance().Destroy();
  m_textureManager->Release();
}

void DrapeEngine::Resize(int w, int h)
{
  if (m_viewport.GetWidth() == w && m_viewport.GetHeight() == h)
    return;

  m_viewport.SetViewport(0, 0, w, h);
  m_threadCommutator->PostMessage(ThreadsCommutator::RenderThread,
                                  make_unique_dp<ResizeMessage>(m_viewport),
                                  MessagePriority::High);
}

void DrapeEngine::UpdateCoverage(ScreenBase const & screen)
{
  m_frontend->SetModelView(screen);
}

void DrapeEngine::ClearUserMarksLayer(df::TileKey const & tileKey)
{
  m_threadCommutator->PostMessage(ThreadsCommutator::RenderThread,
                                  make_unique_dp<ClearUserMarkLayerMessage>(tileKey),
                                  MessagePriority::Normal);
}

void DrapeEngine::ChangeVisibilityUserMarksLayer(TileKey const & tileKey, bool isVisible)
{
  m_threadCommutator->PostMessage(ThreadsCommutator::RenderThread,
                                  make_unique_dp<ChangeUserMarkLayerVisibilityMessage>(tileKey, isVisible),
                                  MessagePriority::Normal);
}

void DrapeEngine::UpdateUserMarksLayer(TileKey const & tileKey, UserMarksProvider * provider)
{
  m_threadCommutator->PostMessage(ThreadsCommutator::ResourceUploadThread,
                                  make_unique_dp<UpdateUserMarkLayerMessage>(tileKey, provider),
                                  MessagePriority::Normal);
}

void DrapeEngine::SetRenderingEnabled(bool const isEnabled)
{
  m_frontend->SetRenderingEnabled(isEnabled);
  m_backend->SetRenderingEnabled(isEnabled);

  LOG(LDEBUG, (isEnabled ? "Rendering enabled" : "Rendering disabled"));
}

} // namespace df
