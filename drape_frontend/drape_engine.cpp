#include "drape_frontend/drape_engine.hpp"
#include "drape_frontend/message_subclasses.hpp"
#include "drape_frontend/visual_params.hpp"

#include "drape_gui/country_status_helper.hpp"
#include "drape_gui/drape_gui.hpp"

#include "drape/texture_manager.hpp"

#include "platform/platform.hpp"
#include "platform/settings.hpp"

#include "std/bind.hpp"

namespace df
{

namespace
{

void ConnectDownloadFn(gui::CountryStatusHelper::EButtonType buttonType, MapDataProvider::TDownloadFn downloadFn)
{
  gui::DrapeGui & guiSubsystem = gui::DrapeGui::Instance();
  guiSubsystem.ConnectOnButtonPressedHandler(buttonType, [downloadFn, &guiSubsystem]()
  {
    storage::TIndex countryIndex = guiSubsystem.GetCountryStatusHelper().GetCountryIndex();
    ASSERT(countryIndex != storage::TIndex::INVALID, ());
    if (downloadFn != nullptr)
      downloadFn(countryIndex);
  });
}

string const LocationStateMode = "LastLocationStateMode";

}

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
  guiSubsystem.SetLocalizator(bind(&StringsBundle::GetString, params.m_stringsBundle.get(), _1));

  ConnectDownloadFn(gui::CountryStatusHelper::BUTTON_TYPE_MAP, params.m_model.GetDownloadMapHandler());
  ConnectDownloadFn(gui::CountryStatusHelper::BUTTON_TYPE_MAP_ROUTING, params.m_model.GetDownloadMapRoutingHandler());
  ConnectDownloadFn(gui::CountryStatusHelper::BUTTON_TRY_AGAIN, params.m_model.GetDownloadRetryHandler());

  m_textureManager = make_unique_dp<dp::TextureManager>();
  m_threadCommutator = make_unique_dp<ThreadsCommutator>();

  int modeValue = 0;
  if (!Settings::Get(LocationStateMode, modeValue))
    modeValue = location::MODE_FOLLOW;

  FrontendRenderer::Params frParams(make_ref(m_threadCommutator), params.m_factory,
                                    make_ref(m_textureManager), m_viewport,
                                    bind(&DrapeEngine::ModelViewChanged, this, _1),
                                    params.m_model.GetIsCountryLoadedFn(),
                                    bind(&DrapeEngine::MyPositionModeChanged, this, _1),
                                    static_cast<location::EMyPositionMode>(modeValue));

  m_frontend = make_unique_dp<FrontendRenderer>(frParams);

  BackendRenderer::Params brParams(frParams.m_commutator, frParams.m_oglContextFactory,
                                   frParams.m_texMng, params.m_model);
  m_backend = make_unique_dp<BackendRenderer>(brParams);
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
  m_viewport.SetViewport(0, 0, w, h);
  AddUserEvent(ResizeEvent(w, h));
}

void DrapeEngine::AddTouchEvent(TouchEvent const & event)
{
  AddUserEvent(event);
}

void DrapeEngine::Scale(double factor, m2::PointD const & pxPoint, bool isAnim)
{
  AddUserEvent(ScaleEvent(factor, pxPoint, isAnim));
}

void DrapeEngine::SetModelViewCenter(m2::PointD const & centerPt, int zoom, bool isAnim)
{
  AddUserEvent(SetCenterEvent(centerPt, zoom, isAnim));
}

void DrapeEngine::SetModelViewRect(m2::RectD const & rect, bool applyRotation, int zoom, bool isAnim)
{
  AddUserEvent(SetRectEvent(rect, applyRotation, zoom, isAnim));
}

void DrapeEngine::SetModelViewAnyRect(m2::AnyRectD const & rect, bool isAnim)
{
  AddUserEvent(SetAnyRectEvent(rect, isAnim));
}

int DrapeEngine::AddModelViewListener(TModelViewListenerFn const & listener)
{
  static int currentSlotID = 0;
  VERIFY(m_listeners.insert(make_pair(++currentSlotID, listener)).second, ());
  return currentSlotID;
}

void DrapeEngine::RemoveModeViewListener(int slotID)
{
  m_listeners.erase(slotID);
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

void DrapeEngine::AddUserEvent(UserEvent const & e)
{
  m_frontend->AddUserEvent(e);
}

void DrapeEngine::ModelViewChanged(ScreenBase const & screen)
{
  Platform & pl = GetPlatform();
  pl.RunOnGuiThread(bind(&DrapeEngine::ModelViewChangedGuiThread, this, screen));
}

void DrapeEngine::ModelViewChangedGuiThread(ScreenBase const & screen)
{
  for (pair<int, TModelViewListenerFn> const & p : m_listeners)
    p.second(screen);
}

void DrapeEngine::MyPositionModeChanged(location::EMyPositionMode mode)
{
  GetPlatform().RunOnGuiThread([this, mode]()
  {
    Settings::Set(LocationStateMode, static_cast<int>(mode));
    if (m_myPositionModeChanged != nullptr)
      m_myPositionModeChanged(mode);
  });
}

void DrapeEngine::SetCountryInfo(gui::CountryInfo const & info, bool isCurrentCountry, bool isCountryLoaded)
{
  m_threadCommutator->PostMessage(ThreadsCommutator::ResourceUploadThread,
                                  make_unique_dp<CountryInfoUpdateMessage>(info, isCurrentCountry, isCountryLoaded),
                                  MessagePriority::Normal);
}

void DrapeEngine::SetCompassInfo(location::CompassInfo const & info)
{
  m_threadCommutator->PostMessage(ThreadsCommutator::RenderThread,
                                  make_unique_dp<CompassInfoMessage>(info),
                                  MessagePriority::High);
}

void DrapeEngine::SetGpsInfo(location::GpsInfo const & info, bool isNavigable, const location::RouteMatchingInfo & routeInfo)
{
  m_threadCommutator->PostMessage(ThreadsCommutator::RenderThread,
                                  make_unique_dp<GpsInfoMessage>(info, isNavigable, routeInfo),
                                  MessagePriority::High);
}

void DrapeEngine::MyPositionNextMode()
{
  m_threadCommutator->PostMessage(ThreadsCommutator::RenderThread,
                                  make_unique_dp<ChangeMyPositionModeMessage>(ChangeMyPositionModeMessage::TYPE_NEXT),
                                  MessagePriority::High);
}

void DrapeEngine::CancelMyPosition()
{
  m_threadCommutator->PostMessage(ThreadsCommutator::RenderThread,
                                  make_unique_dp<ChangeMyPositionModeMessage>(ChangeMyPositionModeMessage::TYPE_CANCEL),
                                  MessagePriority::High);
}

void DrapeEngine::StopLocationFollow()
{
  m_threadCommutator->PostMessage(ThreadsCommutator::RenderThread,
                                  make_unique_dp<ChangeMyPositionModeMessage>(ChangeMyPositionModeMessage::TYPE_STOP_FOLLOW),
                                  MessagePriority::High);
}

void DrapeEngine::InvalidateMyPosition()
{
  m_threadCommutator->PostMessage(ThreadsCommutator::RenderThread,
                                  make_unique_dp<ChangeMyPositionModeMessage>(ChangeMyPositionModeMessage::TYPE_INVALIDATE),
                                  MessagePriority::High);
}

void DrapeEngine::SetMyPositionModeListener(location::TMyPositionModeChanged const & fn)
{
  m_myPositionModeChanged = fn;
}

} // namespace df
