#include "app/organicmaps/Framework.hpp"

#include "app/organicmaps/core/jni_helper.hpp"
#include "app/organicmaps/UserMarkHelper.hpp"
#include "app/organicmaps/opengl/androidoglcontextfactory.hpp"
#include "app/organicmaps/platform/AndroidPlatform.hpp"
#include "app/organicmaps/util/Distance.hpp"
#include "app/organicmaps/util/FeatureIdBuilder.hpp"
#include "app/organicmaps/util/NetworkPolicy.hpp"
#include "app/organicmaps/vulkan/android_vulkan_context_factory.hpp"

#include "map/bookmark_helpers.hpp"
#include "map/chart_generator.hpp"
#include "map/everywhere_search_params.hpp"
#include "map/user_mark.hpp"

#include "storage/storage_defines.hpp"
#include "storage/storage_helpers.hpp"

#include "drape_frontend/user_event_stream.hpp"
#include "drape_frontend/visual_params.hpp"

#include "drape/pointers.hpp"
#include "drape/support_manager.hpp"
#include "drape/visual_scale.hpp"

#include "coding/files_container.hpp"

#include "geometry/angles.hpp"
#include "geometry/mercator.hpp"
#include "geometry/point_with_altitude.hpp"

#include "indexer/feature_altitude.hpp"
#include "indexer/kayak.hpp"
#include "indexer/validate_and_format_contacts.hpp"

#include "routing/following_info.hpp"
#include "routing/speed_camera_manager.hpp"

#include "platform/country_file.hpp"
#include "platform/local_country_file.hpp"
#include "platform/local_country_file_utils.hpp"
#include "platform/locale.hpp"
#include "platform/location.hpp"
#include "platform/localization.hpp"
#include "platform/measurement_utils.hpp"
#include "platform/network_policy.hpp"
#include "platform/platform.hpp"
#include "platform/preferred_languages.hpp"
#include "platform/settings.hpp"
#include "platform/utm_mgrs_utils.hpp"

#include "base/assert.hpp"
#include "base/file_name_utils.hpp"
#include "base/logging.hpp"
#include "base/math.hpp"
#include "base/sunrise_sunset.hpp"

#include "ge0/url_generator.hpp"

#include "3party/open-location-code/openlocationcode.h"

#include <memory>
#include <string>
#include <vector>

#include <android/api-level.h>

using namespace std;
using namespace std::placeholders;

unique_ptr<android::Framework> g_framework;

namespace platform
{
NetworkPolicy ToNativeNetworkPolicy(JNIEnv * env, jobject obj)
{
  return NetworkPolicy(network_policy::GetNetworkPolicyStatus(env, obj));
}
}  // namespace platform

using namespace storage;
using platform::CountryFile;
using platform::LocalCountryFile;
using platform::ToNativeNetworkPolicy;

static_assert(sizeof(int) >= 4, "Size of jint in less than 4 bytes.");

::Framework * frm() { return g_framework->NativeFramework(); }

namespace
{
jobject g_placePageActivationListener = nullptr;

android::AndroidVulkanContextFactory * CastFactory(drape_ptr<dp::GraphicsContextFactory> const & f)
{
  return dynamic_cast<android::AndroidVulkanContextFactory *>(f.get());
}
}  // namespace

namespace android
{

enum MultiTouchAction
{
  MULTITOUCH_UP    =   0x00000001,
  MULTITOUCH_DOWN  =   0x00000002,
  MULTITOUCH_MOVE  =   0x00000003,
  MULTITOUCH_CANCEL =  0x00000004
};

Framework::Framework(std::function<void()> && afterMapsLoaded)
: m_work({} /* params */, false /* loadMaps */)
{
  m_work.LoadMapsAsync(std::move(afterMapsLoaded));

  m_work.GetTrafficManager().SetStateListener(bind(&Framework::TrafficStateChanged, this, _1));
  m_work.GetTransitManager().SetStateListener(bind(&Framework::TransitSchemeStateChanged, this, _1));
  m_work.GetIsolinesManager().SetStateListener(bind(&Framework::IsolinesSchemeStateChanged, this, _1));
  m_work.GetPowerManager().Subscribe(this);
}

void Framework::OnLocationError(int errorCode)
{
  m_work.OnLocationError(static_cast<location::TLocationError>(errorCode));
}

void Framework::OnLocationUpdated(location::GpsInfo const & info)
{
  m_work.OnLocationUpdate(info);
}

void Framework::OnCompassUpdated(location::CompassInfo const & info, bool forceRedraw)
{
  static double const COMPASS_THRESHOLD = base::DegToRad(1.0);

  /// @todo Do not emit compass bearing too often.
  /// Need to make more experiments in future.
  if (forceRedraw || fabs(ang::GetShortestDistance(m_lastCompass, info.m_bearing)) >= COMPASS_THRESHOLD)
  {
    m_lastCompass = info.m_bearing;
    m_work.OnCompassUpdate(info);
  }
}

void Framework::MyPositionModeChanged(location::EMyPositionMode mode, bool routingActive)
{
  if (m_myPositionModeSignal)
    m_myPositionModeSignal(mode, routingActive);
}

void Framework::TrafficStateChanged(TrafficManager::TrafficState state)
{
  if (m_onTrafficStateChangedFn)
    m_onTrafficStateChangedFn(state);
}

void Framework::TransitSchemeStateChanged(TransitReadManager::TransitSchemeState state)
{
  if (m_onTransitStateChangedFn)
    m_onTransitStateChangedFn(state);
}

void Framework::IsolinesSchemeStateChanged(IsolinesManager::IsolinesState state)
{
  if (m_onIsolinesStateChangedFn)
    m_onIsolinesStateChangedFn(state);
}

bool Framework::DestroySurfaceOnDetach()
{
  if (m_vulkanContextFactory)
    return false;
  return true;
}

bool Framework::CreateDrapeEngine(JNIEnv * env, jobject jSurface, int densityDpi, bool firstLaunch,
                                  bool launchByDeepLink, uint32_t appVersionCode, bool isCustomROM)
{
  // Vulkan is supported only since Android 8.0, because some Android devices with Android 7.x
  // have fatal driver issue, which can lead to process termination and whole OS destabilization.
  int constexpr kMinSdkVersionForVulkan = 26;
  int const sdkVersion = android_get_device_api_level();
  LOG(LINFO, ("Android SDK version in the Drape Engine:", sdkVersion));
  auto const vulkanForbidden = sdkVersion < kMinSdkVersionForVulkan ||
                               dp::SupportManager::Instance().IsVulkanForbidden();
  if (vulkanForbidden)
    LOG(LWARNING, ("Vulkan API is forbidden on this device."));

  m_vulkanContextFactory.reset();
  m_oglContextFactory.reset();
  ::Framework::DrapeCreationParams p;

  if (m_work.LoadPreferredGraphicsAPI() == dp::ApiVersion::Vulkan && !vulkanForbidden)
  {
    auto vkFactory = make_unique_dp<AndroidVulkanContextFactory>(appVersionCode, sdkVersion, isCustomROM);
    if (!vkFactory->IsVulkanSupported())
    {
      LOG(LWARNING, ("Vulkan API is not supported."));
    }
    else
    {
      vkFactory->SetSurface(env, jSurface);
      if (!vkFactory->IsValid())
      {
        LOG(LWARNING, ("Invalid Vulkan API context."));
      }
      else
      {
        p.m_apiVersion = dp::ApiVersion::Vulkan;
        p.m_surfaceWidth = vkFactory->GetWidth();
        p.m_surfaceHeight = vkFactory->GetHeight();

        m_vulkanContextFactory = std::move(vkFactory);
      }
    }
  }

  if (!m_vulkanContextFactory)
  {
    auto oglFactory = make_unique_dp<AndroidOGLContextFactory>(env, jSurface);
    if (!oglFactory->IsValid())
    {
      LOG(LWARNING, ("Invalid GL context."));
      return false;
    }
    p.m_apiVersion = oglFactory->IsSupportedOpenGLES3() ? dp::ApiVersion::OpenGLES3 :
                                                          dp::ApiVersion::OpenGLES2;
    p.m_surfaceWidth = oglFactory->GetWidth();
    p.m_surfaceHeight = oglFactory->GetHeight();

    m_oglContextFactory = make_unique_dp<dp::ThreadSafeFactory>(oglFactory.release());
  }

  p.m_visualScale = static_cast<float>(dp::VisualScale(densityDpi));
  // Drape doesn't care about Editor vs Api mode differences.
  p.m_isChoosePositionMode = m_isChoosePositionMode != ChoosePositionMode::None;
  p.m_hints.m_isFirstLaunch = firstLaunch;
  p.m_hints.m_isLaunchByDeepLink = launchByDeepLink;
  ASSERT(!m_guiPositions.empty(), ("GUI elements must be set-up before engine is created"));
  p.m_widgetsInitInfo = m_guiPositions;

  m_work.SetMyPositionModeListener(bind(&Framework::MyPositionModeChanged, this, _1, _2));

  if (m_vulkanContextFactory)
    m_work.CreateDrapeEngine(make_ref(m_vulkanContextFactory), std::move(p));
  else
    m_work.CreateDrapeEngine(make_ref(m_oglContextFactory), std::move(p));
  m_work.EnterForeground();

  return true;
}

bool Framework::IsDrapeEngineCreated() const
{
  return m_work.IsDrapeEngineCreated();
}

void Framework::UpdateDpi(int dpi)
{
  ASSERT_GREATER(dpi, 0, ());
  m_work.UpdateVisualScale(dp::VisualScale(dpi));
}

void Framework::Resize(JNIEnv * env, jobject jSurface, int w, int h)
{
  if (m_vulkanContextFactory)
  {
    auto vulkanContextFactory = CastFactory(m_vulkanContextFactory);
    if (vulkanContextFactory->GetWidth() != w || vulkanContextFactory->GetHeight() != h)
    {
      m_vulkanContextFactory->SetPresentAvailable(false);
      m_work.SetRenderingDisabled(false /* destroySurface */);

      vulkanContextFactory->ChangeSurface(env, jSurface, w, h);

      vulkanContextFactory->SetPresentAvailable(true);
      m_work.SetRenderingEnabled();
    }
  }
  else
  {
    m_oglContextFactory->CastFactory<AndroidOGLContextFactory>()->UpdateSurfaceSize(w, h);
  }
  m_work.OnSize(w, h);
}

void Framework::DetachSurface(bool destroySurface)
{
  LOG(LINFO, ("Detach surface started. destroySurface =", destroySurface));
  if (m_vulkanContextFactory)
  {
    m_vulkanContextFactory->SetPresentAvailable(false);
  }
  else
  {
    ASSERT(m_oglContextFactory != nullptr, ());
    m_oglContextFactory->SetPresentAvailable(false);
  }

  if (destroySurface)
  {
    LOG(LINFO, ("Destroy surface."));
    m_isSurfaceDestroyed = true;
    m_work.OnDestroySurface();
  }

  if (m_vulkanContextFactory)
  {
    // With Vulkan we don't need to recreate all graphics resources,
    // we have to destroy only resources bound with surface (swapchains,
    // image views, framebuffers and command buffers). All these resources will be
    // destroyed in ResetSurface().
    m_work.SetRenderingDisabled(false /* destroySurface */);

    // Allow pipeline dump only on enter background.
    CastFactory(m_vulkanContextFactory)->ResetSurface(destroySurface /* allowPipelineDump */);
  }
  else
  {
    m_work.SetRenderingDisabled(destroySurface);
    auto factory = m_oglContextFactory->CastFactory<AndroidOGLContextFactory>();
    factory->ResetSurface();
  }
  LOG(LINFO, ("Detach surface finished."));
}

bool Framework::AttachSurface(JNIEnv * env, jobject jSurface)
{
  LOG(LINFO, ("Attach surface started."));

  int w = 0, h = 0;
  if (m_vulkanContextFactory)
  {
    auto factory = CastFactory(m_vulkanContextFactory);
    factory->SetSurface(env, jSurface);
    if (!factory->IsValid())
    {
      LOG(LWARNING, ("Invalid Vulkan API context."));
      return false;
    }
    w = factory->GetWidth();
    h = factory->GetHeight();
  }
  else
  {
    ASSERT(m_oglContextFactory != nullptr, ());
    auto factory = m_oglContextFactory->CastFactory<AndroidOGLContextFactory>();
    factory->SetSurface(env, jSurface);
    if (!factory->IsValid())
    {
      LOG(LWARNING, ("Invalid GL context."));
      return false;
    }
    w = factory->GetWidth();
    h = factory->GetHeight();
  }

  ASSERT(!m_guiPositions.empty(), ("GUI elements must be set-up before engine is created"));

  if (m_vulkanContextFactory)
  {
    m_vulkanContextFactory->SetPresentAvailable(true);
    m_work.SetRenderingEnabled();
  }
  else
  {
    m_oglContextFactory->SetPresentAvailable(true);
    m_work.SetRenderingEnabled(make_ref(m_oglContextFactory));
  }

  if (m_isSurfaceDestroyed)
  {
    LOG(LINFO, ("Recover surface, viewport size:", w, h));
    bool const recreateContextDependentResources = (m_vulkanContextFactory == nullptr);
    m_work.OnRecoverSurface(w, h, recreateContextDependentResources);
    m_isSurfaceDestroyed = false;
  }

  LOG(LINFO, ("Attach surface finished."));

  return true;
}

void Framework::PauseSurfaceRendering()
{
  if (m_vulkanContextFactory)
    m_vulkanContextFactory->SetPresentAvailable(false);
  if (m_oglContextFactory)
    m_oglContextFactory->SetPresentAvailable(false);

  LOG(LINFO, ("Pause surface rendering."));
}

void Framework::ResumeSurfaceRendering()
{
  if (m_vulkanContextFactory)
  {
    if (CastFactory(m_vulkanContextFactory)->IsValid())
      m_vulkanContextFactory->SetPresentAvailable(true);
  }
  if (m_oglContextFactory)
  {
    AndroidOGLContextFactory * factory = m_oglContextFactory->CastFactory<AndroidOGLContextFactory>();
    if (factory->IsValid())
      factory->SetPresentAvailable(true);
  }
  LOG(LINFO, ("Resume surface rendering."));
}

void Framework::SetMapStyle(MapStyle mapStyle)
{
  m_work.SetMapStyle(mapStyle);
}

void Framework::MarkMapStyle(MapStyle mapStyle)
{
  // In case of Vulkan rendering we don't recreate geometry and textures data, so
  // we need use SetMapStyle instead of MarkMapStyle in all cases.
  if (m_vulkanContextFactory)
    m_work.SetMapStyle(mapStyle);
  else
    m_work.MarkMapStyle(mapStyle);
}

MapStyle Framework::GetMapStyle() const
{
  return m_work.GetMapStyle();
}

void Framework::Save3dMode(bool allow3d, bool allow3dBuildings)
{
  m_work.Save3dMode(allow3d, allow3dBuildings);
}

void Framework::Set3dMode(bool allow3d, bool allow3dBuildings)
{
  m_work.Allow3dMode(allow3d, allow3dBuildings);
}

void Framework::Get3dMode(bool & allow3d, bool & allow3dBuildings)
{
  m_work.Load3dMode(allow3d, allow3dBuildings);
}

void Framework::SetMapLanguageCode(std::string const & languageCode)
{
  m_work.SetMapLanguageCode(languageCode);
}

std::string Framework::GetMapLanguageCode()
{
  return m_work.GetMapLanguageCode();
}

void Framework::SetChoosePositionMode(ChoosePositionMode mode, bool isBusiness, m2::PointD const * optionalPosition)
{
  m_isChoosePositionMode = mode;
  m_work.BlockTapEvents(mode != ChoosePositionMode::None);
  m_work.EnableChoosePositionMode(mode != ChoosePositionMode::None, isBusiness, optionalPosition);
}

ChoosePositionMode Framework::GetChoosePositionMode()
{
  return m_isChoosePositionMode;
}

Storage & Framework::GetStorage()
{
  return m_work.GetStorage();
}

DataSource const & Framework::GetDataSource() { return m_work.GetDataSource(); }

void Framework::ShowNode(CountryId const & idx, bool zoomToDownloadButton)
{
  if (zoomToDownloadButton)
  {
    m2::RectD const rect = CalcLimitRect(idx, m_work.GetStorage(), m_work.GetCountryInfoGetter());
    m_work.SetViewportCenter(rect.Center(), 10);
  }
  else
  {
    m_work.ShowNode(idx);
  }
}

void Framework::Scale(double factor, m2::PointD const & pxPoint, bool isAnim)
{
  m_work.Scale(factor, pxPoint, isAnim);
}

void Framework::Scroll(double distanceX, double distanceY)
{
  m_work.Scroll(distanceX, distanceY);
}

void Framework::Touch(int action, Finger const & f1, Finger const & f2, uint8_t maskedPointer)
{
  MultiTouchAction eventType = static_cast<MultiTouchAction>(action);
  df::TouchEvent event;

  switch(eventType)
  {
  case MULTITOUCH_DOWN:
    event.SetTouchType(df::TouchEvent::TOUCH_DOWN);
    break;
  case MULTITOUCH_MOVE:
    event.SetTouchType(df::TouchEvent::TOUCH_MOVE);
    break;
  case MULTITOUCH_UP:
    event.SetTouchType(df::TouchEvent::TOUCH_UP);
    break;
  case MULTITOUCH_CANCEL:
    event.SetTouchType(df::TouchEvent::TOUCH_CANCEL);
    break;
  default:
    return;
  }

  df::Touch touch;
  touch.m_location = m2::PointD(f1.m_x, f1.m_y);
  touch.m_id = f1.m_id;
  event.SetFirstTouch(touch);

  touch.m_location = m2::PointD(f2.m_x, f2.m_y);
  touch.m_id = f2.m_id;
  event.SetSecondTouch(touch);

  event.SetFirstMaskedPointer(maskedPointer);
  m_work.TouchEvent(event);
}

m2::PointD Framework::GetViewportCenter() const
{
  return m_work.GetViewportCenter();
}

void Framework::AddString(string const & name, string const & value)
{
  m_work.AddString(name, value);
}

void Framework::Scale(::Framework::EScaleMode mode)
{
  m_work.Scale(mode, true);
}

void Framework::Scale(m2::PointD const & centerPt, int targetZoom, bool animate)
{
  ref_ptr<df::DrapeEngine> engine = m_work.GetDrapeEngine();
  if (engine)
    engine->SetModelViewCenter(centerPt, targetZoom, animate, false);
}

::Framework * Framework::NativeFramework()
{
  return &m_work;
}

bool Framework::Search(search::EverywhereSearchParams const & params)
{
  m_searchQuery = params.m_query;
  return m_work.GetSearchAPI().SearchEverywhere(params);
}

void Framework::AddLocalMaps()
{
  m_work.RegisterAllMaps();
}

void Framework::RemoveLocalMaps()
{
  m_work.DeregisterAllMaps();
}

void Framework::ReloadWorldMaps()
{
  /// @todo Can invent more optimal routine to remove/add World files only.
  RemoveLocalMaps();
  AddLocalMaps();
}

void Framework::ReplaceBookmark(kml::MarkId markId, kml::BookmarkData & bm)
{
  m_work.GetBookmarkManager().GetEditSession().UpdateBookmark(markId, bm);
}

void Framework::ReplaceTrack(kml::TrackId trackId, kml::TrackData & trackData)
{
  m_work.GetBookmarkManager().GetEditSession().UpdateTrack(trackId, trackData);
}

void Framework::ChangeTrackColor(kml::TrackId trackId, dp::Color color)
{
  m_work.GetBookmarkManager().GetEditSession().ChangeTrackColor(trackId, color);
}

void Framework::MoveBookmark(kml::MarkId markId, kml::MarkGroupId curCat, kml::MarkGroupId newCat)
{
  m_work.GetBookmarkManager().GetEditSession().MoveBookmark(markId, curCat, newCat);
}

void Framework::MoveTrack(kml::TrackId trackId, kml::MarkGroupId curCat, kml::MarkGroupId newCat)
{
  m_work.GetBookmarkManager().GetEditSession().MoveTrack(trackId, curCat, newCat);
}

void Framework::ExecuteMapApiRequest()
{
  return m_work.ExecuteMapApiRequest();
}

void Framework::DeactivatePopup()
{
  m_work.DeactivateMapSelection();
}

void Framework::DeactivateMapSelectionCircle()
{
  m_work.DeactivateMapSelectionCircle();
}

/*
string Framework::GetOutdatedCountriesString()
{
  vector<Country const *> countries;
  class Storage const & storage = GetStorage();
  storage.GetOutdatedCountries(countries);

  string res;
  NodeAttrs attrs;

  for (size_t i = 0; i < countries.size(); ++i)
  {
    storage.GetNodeAttrs(countries[i]->Name(), attrs);

    if (i > 0)
      res += ", ";

    res += attrs.m_nodeLocalName;
  }

  return res;
}
*/

void Framework::SetTrafficStateListener(TrafficManager::TrafficStateChangedFn const & fn)
{
  m_onTrafficStateChangedFn = fn;
}

void Framework::SetTransitSchemeListener(TransitReadManager::TransitStateChangedFn const & function)
{
  m_onTransitStateChangedFn = function;
}

void Framework::SetIsolinesListener(IsolinesManager::IsolinesStateChangedFn const & function)
{
  m_onIsolinesStateChangedFn = function;
}

bool Framework::IsTrafficEnabled()
{
  return m_work.GetTrafficManager().IsEnabled();
}

void Framework::EnableTraffic()
{
  m_work.GetTrafficManager().SetEnabled(true);
  NativeFramework()->SaveTrafficEnabled(true);
}

void Framework::DisableTraffic()
{
  m_work.GetTrafficManager().SetEnabled(false);
  NativeFramework()->SaveTrafficEnabled(false);
}

void Framework::SetMyPositionModeListener(location::TMyPositionModeChanged const & fn)
{
  m_myPositionModeSignal = fn;
}

location::EMyPositionMode Framework::GetMyPositionMode() const
{
  // No need in assertion here, return location::PendingPosition if no engine created.
  //ASSERT(IsDrapeEngineCreated(), ());

  return m_work.GetMyPositionMode();
}

void Framework::SwitchMyPositionNextMode()
{
  ASSERT(IsDrapeEngineCreated(), ());
  m_work.SwitchMyPositionNextMode();
}

void Framework::SetupWidget(gui::EWidget widget, float x, float y, dp::Anchor anchor)
{
  m_guiPositions[widget] = gui::Position(m2::PointF(x, y), anchor);
}

void Framework::UpdateMyPositionRoutingOffset(int offsetY)
{
  NativeFramework()->UpdateMyPositionRoutingOffset(false, offsetY);
}

void Framework::ApplyWidgets()
{
  gui::TWidgetsLayoutInfo layout;
  for (auto const & widget : m_guiPositions)
    layout[widget.first] = widget.second.m_pixelPivot;

  m_work.SetWidgetLayout(std::move(layout));
}

void Framework::CleanWidgets()
{
  m_guiPositions.clear();
}

void Framework::SetupMeasurementSystem()
{
  m_work.SetupMeasurementSystem();
}

place_page::Info & Framework::GetPlacePageInfo()
{
  return m_work.GetCurrentPlacePageInfo();
}

bool Framework::IsAutoRetryDownloadFailed()
{
  return m_work.GetDownloadingPolicy().IsAutoRetryDownloadFailed();
}

bool Framework::IsDownloadOn3gEnabled()
{
  return m_work.GetDownloadingPolicy().IsCellularDownloadEnabled();
}

void Framework::EnableDownloadOn3g()
{
  m_work.GetDownloadingPolicy().EnableCellularDownload(true);
}

/*
int Framework::ToDoAfterUpdate() const
{
  return (int) m_work.ToDoAfterUpdate();
}
*/

void Framework::OnPowerFacilityChanged(power_management::Facility const facility, bool enabled)
{
  // Dummy
  // TODO: provide information for UI Properties.
}

void Framework::OnPowerSchemeChanged(power_management::Scheme const actualScheme)
{
  // Dummy
  // TODO: provide information for UI Properties.
}

FeatureID Framework::BuildFeatureId(JNIEnv * env, jobject featureId)
{
  static FeatureIdBuilder const builder(env);

  return builder.Build(env, featureId);
}
}  // namespace android

//============ GLUE CODE for app.organicmaps.Framework class =============//
/*            ____
 *          _ |||| _
 *          \\    //
 *           \\  //
 *            \\//
 *             \/
 */

extern "C"
{
void CallRoutingListener(shared_ptr<jobject> listener, int errorCode,
                         storage::CountriesSet const & absentMaps)
{
  JNIEnv * env = jni::GetEnv();
  jmethodID const method = jni::GetMethodID(env, *listener, "onRoutingEvent", "(I[Ljava/lang/String;)V");
  ASSERT(method, ());

  env->CallVoidMethod(*listener, method, errorCode, jni::TScopedLocalObjectArrayRef(env, jni::ToJavaStringArray(env, absentMaps)).get());
}

void CallRouteProgressListener(shared_ptr<jobject> listener, float progress)
{
  JNIEnv * env = jni::GetEnv();
  jmethodID const methodId = jni::GetMethodID(env, *listener, "onRouteBuildingProgress", "(F)V");
  env->CallVoidMethod(*listener, methodId, progress);
}

void CallRouteRecommendationListener(shared_ptr<jobject> listener,
                                     RoutingManager::Recommendation recommendation)
{
  JNIEnv * env = jni::GetEnv();
  jmethodID const methodId = jni::GetMethodID(env, *listener, "onRecommend", "(I)V");
  env->CallVoidMethod(*listener, methodId, static_cast<int>(recommendation));
}

void CallSetRoutingLoadPointsListener(shared_ptr<jobject> listener, bool success)
{
  JNIEnv * env = jni::GetEnv();
  jmethodID const methodId = jni::GetMethodID(env, *listener, "onRoutePointsLoaded", "(Z)V");
  env->CallVoidMethod(*listener, methodId, static_cast<jboolean>(success));
}

RoutingManager::LoadRouteHandler g_loadRouteHandler;

/// @name JNI EXPORTS
JNIEXPORT jstring JNICALL
Java_app_organicmaps_Framework_nativeGetAddress(JNIEnv * env, jclass clazz, jdouble lat, jdouble lon)
{
  auto const info = frm()->GetAddressAtPoint(mercator::FromLatLon(lat, lon));
  return jni::ToJavaString(env, info.FormatAddress());
}

JNIEXPORT void JNICALL
Java_app_organicmaps_Framework_nativeClearApiPoints(JNIEnv * env, jclass clazz)
{
  frm()->GetBookmarkManager().GetEditSession().ClearGroup(UserMark::Type::API);
}

JNIEXPORT jint JNICALL
Java_app_organicmaps_Framework_nativeParseAndSetApiUrl(JNIEnv * env, jclass clazz, jstring url)
{
  return static_cast<jint>(frm()->ParseAndSetApiURL(jni::ToNativeString(env, url)));
}

JNIEXPORT jobject JNICALL
Java_app_organicmaps_Framework_nativeGetParsedRoutingData(JNIEnv * env, jclass clazz)
{
  using namespace url_scheme;
  static jclass const pointClazz = jni::GetGlobalClassRef(env, "app/organicmaps/api/RoutePoint");
  // Java signature : RoutePoint(double lat, double lon, String name)
  static jmethodID const pointConstructor = jni::GetConstructorID(env, pointClazz, "(DDLjava/lang/String;)V");

  static jclass const routeDataClazz = jni::GetGlobalClassRef(env, "app/organicmaps/api/ParsedRoutingData");
  // Java signature : ParsedRoutingData(RoutePoint[] points, int routerType) {
  static jmethodID const routeDataConstructor = jni::GetConstructorID(env, routeDataClazz, "([Lapp/organicmaps/api/RoutePoint;I)V");

  auto const & routingData = frm()->GetParsedRoutingData();
  jobjectArray points = jni::ToJavaArray(env, pointClazz, routingData.m_points,
                                         [](JNIEnv * env, RoutePoint const & point)
                                         {
                                           jni::TScopedLocalRef const name(env, jni::ToJavaString(env, point.m_name));
                                           return env->NewObject(pointClazz, pointConstructor,
                                                                 mercator::YToLat(point.m_org.y),
                                                                 mercator::XToLon(point.m_org.x), name.get());
                                         });

  return env->NewObject(routeDataClazz, routeDataConstructor, points, routingData.m_type);
}

JNIEXPORT jobject JNICALL
Java_app_organicmaps_Framework_nativeGetParsedSearchRequest(JNIEnv * env, jclass clazz)
{
  using namespace url_scheme;
  static jclass const cl = jni::GetGlobalClassRef(env, "app/organicmaps/api/ParsedSearchRequest");
  // Java signature : ParsedSearchRequest(String query, String locale, double lat, double lon, boolean isSearchOnMap)
  static jmethodID const ctor = jni::GetConstructorID(env, cl, "(Ljava/lang/String;Ljava/lang/String;DDZ)V");
  auto const & r = frm()->GetParsedSearchRequest();
  ms::LatLon const center = frm()->GetParsedCenterLatLon();
  return env->NewObject(cl, ctor, jni::ToJavaString(env, r.m_query), jni::ToJavaString(env, r.m_locale), center.m_lat, center.m_lon, r.m_isSearchOnMap);
}

JNIEXPORT jstring JNICALL
Java_app_organicmaps_Framework_nativeGetParsedAppName(JNIEnv * env, jclass)
{
  std::string const & appName = frm()->GetParsedAppName();
  return (appName.empty()) ? nullptr : jni::ToJavaString(env, appName);
}

JNIEXPORT jstring JNICALL
Java_app_organicmaps_Framework_nativeGetParsedOAuth2Code(JNIEnv * env, jclass)
{
  std::string const & code = frm()->GetParsedOAuth2Code();
  return jni::ToJavaString(env, code);
}

JNIEXPORT jstring JNICALL
Java_app_organicmaps_Framework_nativeGetParsedBackUrl(JNIEnv * env, jclass)
{
  std::string const & backUrl = frm()->GetParsedBackUrl();
  return (backUrl.empty()) ? nullptr : jni::ToJavaString(env, backUrl);
}

JNIEXPORT jdoubleArray JNICALL
Java_app_organicmaps_Framework_nativeGetParsedCenterLatLon(JNIEnv * env, jclass)
{
  ms::LatLon const center = frm()->GetParsedCenterLatLon();
  if (!center.IsValid())
    return nullptr;

  double latlon[] = {center.m_lat, center.m_lon};
  jdoubleArray jLatLon = env->NewDoubleArray(2);
  env->SetDoubleArrayRegion(jLatLon, 0, 2, latlon);

  return jLatLon;
}

JNIEXPORT void JNICALL
Java_app_organicmaps_Framework_nativePlacePageActivationListener(JNIEnv *env, jclass, jobject jListener)
{
  LOG(LINFO, ("Set global map object listener"));
  g_placePageActivationListener = env->NewGlobalRef(jListener);
  // void onPlacePageActivated(MapObject object);
  jmethodID const activatedId = jni::GetMethodID(env, g_placePageActivationListener,
                                                 "onPlacePageActivated",
                                                 "(Lapp/organicmaps/widget/placepage/PlacePageData;)V");
  // void onPlacePageDeactivated();
  jmethodID const deactivateId = jni::GetMethodID(env, g_placePageActivationListener,
                                                  "onPlacePageDeactivated", "()V");
  // void onPlacePageDeactivated();
  jmethodID const switchFullscreenId = jni::GetMethodID(env, g_placePageActivationListener,
                                                        "onSwitchFullScreenMode", "()V");
  auto const fillPlacePage = [activatedId]()
  {
    JNIEnv * env = jni::GetEnv();
    auto const & info = frm()->GetCurrentPlacePageInfo();
    jni::TScopedLocalRef placePageDataRef(env, nullptr);
    if (info.IsTrack())
    {
      // todo: (KK) implement elevation info handling for the proper track selection
      auto const & track = frm()->GetBookmarkManager().GetTrack(info.GetTrackId());
      auto const & elevationInfo = track->GetElevationInfo();
      if (elevationInfo.has_value())
        placePageDataRef.reset(usermark_helper::CreateElevationInfo(env, elevationInfo.value()));
    }
    if (!placePageDataRef)
      placePageDataRef.reset(usermark_helper::CreateMapObject(env, info));

    env->CallVoidMethod(g_placePageActivationListener, activatedId, placePageDataRef.get());
  };
  auto const closePlacePage = [deactivateId]()
  {
    JNIEnv * env = jni::GetEnv();
    env->CallVoidMethod(g_placePageActivationListener, deactivateId);
  };
  auto const switchFullscreen = [switchFullscreenId]()
  {
    JNIEnv * env = jni::GetEnv();
    env->CallVoidMethod(g_placePageActivationListener, switchFullscreenId);
  };

  frm()->SetPlacePageListeners(fillPlacePage, closePlacePage, fillPlacePage, switchFullscreen);
}

JNIEXPORT void JNICALL
Java_app_organicmaps_Framework_nativeRemovePlacePageActivationListener(JNIEnv *env, jclass, jobject jListener)
{
  if (g_placePageActivationListener == nullptr)
    return;

  if (!env->IsSameObject(g_placePageActivationListener, jListener))
    return;

  frm()->SetPlacePageListeners({} /* onOpen */, {} /* onClose */, {} /* onUpdate */, {} /* onSwitchFullScreen */);
  LOG(LINFO, ("Remove global map object listener"));
  env->DeleteGlobalRef(g_placePageActivationListener);
  g_placePageActivationListener = nullptr;
}

JNIEXPORT jstring JNICALL
Java_app_organicmaps_Framework_nativeGetGe0Url(JNIEnv * env, jclass, jdouble lat, jdouble lon, jdouble zoomLevel, jstring name)
{
  ::Framework * fr = frm();
  double const scale = (zoomLevel > 0 ? zoomLevel : fr->GetDrawScale());
  string const url = fr->CodeGe0url(lat, lon, scale, jni::ToNativeString(env, name));
  return jni::ToJavaString(env, url);
}

JNIEXPORT jstring JNICALL
Java_app_organicmaps_Framework_nativeGetGeoUri(JNIEnv * env, jclass, jdouble lat, jdouble lon, jdouble zoomLevel, jstring name)
{
  ::Framework * fr = frm();
  double const scale = (zoomLevel > 0 ? zoomLevel : fr->GetDrawScale());
  string const url = ge0::GenerateGeoUri(lat, lon, scale, jni::ToNativeString(env, name));
  return jni::ToJavaString(env, url);
}

JNIEXPORT jobject JNICALL
Java_app_organicmaps_Framework_nativeGetDistanceAndAzimuth(
    JNIEnv * env, jclass, jdouble merX, jdouble merY, jdouble cLat, jdouble cLon, jdouble north)
{
  platform::Distance distance;
  double azimut = -1.0;
  frm()->GetDistanceAndAzimut(m2::PointD(merX, merY), cLat, cLon, north, distance, azimut);

  static jclass const daClazz = jni::GetGlobalClassRef(env, "app/organicmaps/bookmarks/data/DistanceAndAzimut");
  // Java signature : DistanceAndAzimut(Distance distance, double azimuth)
  static jmethodID const methodID = jni::GetConstructorID(env, daClazz, "(Lapp/organicmaps/util/Distance;D)V");

  return env->NewObject(daClazz, methodID,
                        ToJavaDistance(env, distance),
                        static_cast<jdouble>(azimut));
}

JNIEXPORT jobject JNICALL
Java_app_organicmaps_Framework_nativeGetDistanceAndAzimuthFromLatLon(
    JNIEnv * env, jclass clazz, jdouble lat, jdouble lon, jdouble cLat, jdouble cLon, jdouble north)
{
  double const merY = mercator::LatToY(lat);
  double const merX = mercator::LonToX(lon);
  return Java_app_organicmaps_Framework_nativeGetDistanceAndAzimuth(env, clazz, merX, merY, cLat, cLon, north);
}

JNIEXPORT jstring JNICALL
Java_app_organicmaps_Framework_nativeFormatLatLon(JNIEnv * env, jclass, jdouble lat, jdouble lon, int coordsFormat)
{
  switch (static_cast<android::CoordinatesFormat>(coordsFormat))
  {
    default:
    case android::CoordinatesFormat::LatLonDMS: // DMS, comma separated
      return jni::ToJavaString(env, measurement_utils::FormatLatLonAsDMS(lat, lon, true /*withComma*/, 2));
    case android::CoordinatesFormat::LatLonDecimal: // Decimal, comma separated
      return jni::ToJavaString(env, measurement_utils::FormatLatLon(lat, lon, true /* withComma */, 6));
    case android::CoordinatesFormat::OLCFull: // Open location code, long format
      return jni::ToJavaString(env, openlocationcode::Encode({lat, lon}));
    case android::CoordinatesFormat::OSMLink: // Link to osm.org
      return jni::ToJavaString(env, measurement_utils::FormatOsmLink(lat, lon, 14));
    case android::CoordinatesFormat::UTM:  // Universal Transverse Mercator
    {
      string utmFormat = utm_mgrs_utils::FormatUTM(lat, lon);
      if (!utmFormat.empty())
        return jni::ToJavaString(env, utmFormat);
      else
        return nullptr;
    }
    case android::CoordinatesFormat::MGRS: // Military Grid Reference System
    {
      string mgrsFormat = utm_mgrs_utils::FormatMGRS(lat, lon, 5);
      if (!mgrsFormat.empty())
         return jni::ToJavaString(env, mgrsFormat);
      else
         return nullptr;
    }
  }
}

JNIEXPORT jstring JNICALL
Java_app_organicmaps_Framework_nativeFormatAltitude(JNIEnv * env, jclass, jdouble alt)
{
  return jni::ToJavaString(env, platform::Distance::FormatAltitude(alt));
}

JNIEXPORT jstring JNICALL
Java_app_organicmaps_Framework_nativeFormatSpeed(JNIEnv * env, jclass, jdouble speed)
{
  auto const units = measurement_utils::GetMeasurementUnits();
  return jni::ToJavaString(env, measurement_utils::FormatSpeedNumeric(speed, units) + " " +
                                platform::GetLocalizedSpeedUnits(units));
}

/*
JNIEXPORT jobject JNICALL
Java_app_organicmaps_Framework_nativeGetOutdatedCountriesString(JNIEnv * env, jclass)
{
  return jni::ToJavaString(env, g_framework->GetOutdatedCountriesString());
}

JNIEXPORT jobjectArray JNICALL
Java_app_organicmaps_Framework_nativeGetOutdatedCountries(JNIEnv * env, jclass)
{
  vector<Country const *> countries;
  Storage const & storage = g_framework->GetStorage();
  storage.GetOutdatedCountries(countries);

  vector<string> ids;
  for (auto country : countries)
    ids.push_back(country->Name());

  return jni::ToJavaStringArray(env, ids);
}

JNIEXPORT jint JNICALL
Java_app_organicmaps_Framework_nativeToDoAfterUpdate(JNIEnv * env, jclass)
{
  return g_framework->ToDoAfterUpdate();
}

JNIEXPORT jboolean JNICALL
Java_app_organicmaps_Framework_nativeIsDataVersionChanged(JNIEnv * env, jclass)
{
  return frm()->IsDataVersionUpdated() ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT void JNICALL
Java_app_organicmaps_Framework_nativeUpdateSavedDataVersion(JNIEnv * env, jclass)
{
  frm()->UpdateSavedDataVersion();
}
*/

JNIEXPORT jlong JNICALL
Java_app_organicmaps_Framework_nativeGetDataVersion(JNIEnv * env, jclass)
{
  return frm()->GetCurrentDataVersion();
}

JNIEXPORT jint JNICALL
Java_app_organicmaps_Framework_nativeGetDrawScale(JNIEnv * env, jclass)
{
  return static_cast<jint>(frm()->GetDrawScale());
}

JNIEXPORT void JNICALL
Java_app_organicmaps_Framework_nativePokeSearchInViewport(JNIEnv * env, jclass)
{
  frm()->GetSearchAPI().PokeSearchInViewport();
}

JNIEXPORT jdoubleArray JNICALL
Java_app_organicmaps_Framework_nativeGetScreenRectCenter(JNIEnv * env, jclass)
{
  m2::PointD const center = frm()->GetViewportCenter();

  double latlon[] = {mercator::YToLat(center.y), mercator::XToLon(center.x)};
  jdoubleArray jLatLon = env->NewDoubleArray(2);
  env->SetDoubleArrayRegion(jLatLon, 0, 2, latlon);

  return jLatLon;
}

JNIEXPORT void JNICALL
Java_app_organicmaps_Framework_nativeShowTrackRect(JNIEnv * env, jclass, jlong track)
{
  frm()->ShowTrack(static_cast<kml::TrackId>(track));
}

JNIEXPORT jstring JNICALL
Java_app_organicmaps_Framework_nativeGetBookmarkDir(JNIEnv * env, jclass)
{
  return jni::ToJavaString(env, GetPlatform().SettingsDir().c_str());
}

JNIEXPORT jstring JNICALL
Java_app_organicmaps_Framework_nativeGetWritableDir(JNIEnv * env, jclass)
{
  return jni::ToJavaString(env, GetPlatform().WritableDir().c_str());
}

JNIEXPORT jstring JNICALL
Java_app_organicmaps_Framework_nativeGetSettingsDir(JNIEnv * env, jclass)
{
  return jni::ToJavaString(env, GetPlatform().SettingsDir().c_str());
}

JNIEXPORT jstring JNICALL
Java_app_organicmaps_Framework_nativeGetDataFileExt(JNIEnv * env, jclass)
{
  return jni::ToJavaString(env, DATA_FILE_EXTENSION);
}

JNIEXPORT jobjectArray JNICALL
Java_app_organicmaps_Framework_nativeGetMovableFilesExts(JNIEnv * env, jclass)
{
  vector<string> exts = { DATA_FILE_EXTENSION, FONT_FILE_EXTENSION };
  platform::CountryIndexes::GetIndexesExts(exts);
  return jni::ToJavaStringArray(env, exts);
}

JNIEXPORT jobjectArray JNICALL
Java_app_organicmaps_Framework_nativeGetBookmarksFilesExts(JNIEnv * env, jclass)
{
  static std::array<std::string, 4> const kBookmarkExtensions = {
    std::string{kKmzExtension},
    std::string{kKmlExtension},
    std::string{kKmbExtension},
    std::string{kGpxExtension}
  };

  return jni::ToJavaStringArray(env, kBookmarkExtensions);
}

JNIEXPORT void JNICALL
Java_app_organicmaps_Framework_nativeChangeWritableDir(JNIEnv * env, jclass, jstring jNewPath)
{
  string newPath = jni::ToNativeString(env, jNewPath);
  g_framework->RemoveLocalMaps();
  android::Platform::Instance().SetWritableDir(newPath);
  g_framework->AddLocalMaps();
}

JNIEXPORT jboolean JNICALL
Java_app_organicmaps_Framework_nativeIsRoutingActive(JNIEnv * env, jclass)
{
  return frm()->GetRoutingManager().IsRoutingActive();
}

JNIEXPORT jboolean JNICALL
Java_app_organicmaps_Framework_nativeIsRouteBuilding(JNIEnv * env, jclass)
{
  return frm()->GetRoutingManager().IsRouteBuilding();
}

JNIEXPORT jboolean JNICALL
Java_app_organicmaps_Framework_nativeIsRouteBuilt(JNIEnv * env, jclass)
{
  return frm()->GetRoutingManager().IsRouteBuilt();
}

JNIEXPORT void JNICALL
Java_app_organicmaps_Framework_nativeCloseRouting(JNIEnv * env, jclass)
{
  frm()->GetRoutingManager().CloseRouting(true /* remove route points */);
}

JNIEXPORT void JNICALL
Java_app_organicmaps_Framework_nativeBuildRoute(JNIEnv * env, jclass)
{
  frm()->GetRoutingManager().BuildRoute();
}

JNIEXPORT void JNICALL
Java_app_organicmaps_Framework_nativeRemoveRoute(JNIEnv * env, jclass)
{
  frm()->GetRoutingManager().RemoveRoute(false /* deactivateFollowing */);
}

JNIEXPORT void JNICALL
Java_app_organicmaps_Framework_nativeFollowRoute(JNIEnv * env, jclass)
{
  frm()->GetRoutingManager().FollowRoute();
}

JNIEXPORT void JNICALL
Java_app_organicmaps_Framework_nativeDisableFollowing(JNIEnv * env, jclass)
{
  frm()->GetRoutingManager().DisableFollowMode();
}

JNIEXPORT jobjectArray JNICALL
Java_app_organicmaps_Framework_nativeGenerateNotifications(JNIEnv * env, jclass, bool announceStreets)
{
  ::Framework * fr = frm();
  if (!fr->GetRoutingManager().IsRoutingActive())
    return nullptr;

  vector<string> notifications;
  fr->GetRoutingManager().GenerateNotifications(notifications, announceStreets);
  if (notifications.empty())
    return nullptr;

  return jni::ToJavaStringArray(env, notifications);
}

JNIEXPORT void JNICALL
Java_app_organicmaps_Framework_nativeSetSpeedCamManagerMode(JNIEnv * env, jclass, jint mode)
{
  frm()->GetRoutingManager().GetSpeedCamManager().SetMode(
    static_cast<routing::SpeedCameraManagerMode>(mode));
}

JNIEXPORT jint JNICALL
Java_app_organicmaps_Framework_nativeGetSpeedCamManagerMode(JNIEnv * env, jclass)
{
  return static_cast<jint>(frm()->GetRoutingManager().GetSpeedCamManager().GetMode());
}

JNIEXPORT jobject JNICALL
Java_app_organicmaps_Framework_nativeGetRouteFollowingInfo(JNIEnv * env, jclass)
{
  ::Framework * fr = frm();
  if (!fr->GetRoutingManager().IsRoutingActive())
    return nullptr;

  routing::FollowingInfo info;
  fr->GetRoutingManager().GetRouteFollowingInfo(info);
  if (!info.IsValid())
    return nullptr;

  static jclass const klass = jni::GetGlobalClassRef(env, "app/organicmaps/routing/RoutingInfo");
  // Java signature : RoutingInfo(Distance distToTarget, Distance distToTurn,
  //                              String currentStreet, String nextStreet, String nextNextStreet,
  //                              double completionPercent, int vehicleTurnOrdinal, int
  //                              vehicleNextTurnOrdinal, int pedestrianTurnOrdinal, int exitNum,
  //                              int totalTime, SingleLaneInfo[] lanes)
  static jmethodID const ctorRouteInfoID =
      jni::GetConstructorID(env, klass,
                            "(Lapp/organicmaps/util/Distance;Lapp/organicmaps/util/Distance;"
                            "Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;DIIIII"
                            "[Lapp/organicmaps/routing/SingleLaneInfo;DZZ)V");

  vector<routing::FollowingInfo::SingleLaneInfoClient> const & lanes = info.m_lanes;
  jobjectArray jLanes = nullptr;
  if (!lanes.empty())
  {
    static jclass const laneClass = jni::GetGlobalClassRef(env, "app/organicmaps/routing/SingleLaneInfo");
    auto const lanesSize = static_cast<jsize>(lanes.size());
    jLanes = env->NewObjectArray(lanesSize, laneClass, nullptr);
    ASSERT(jLanes, (jni::DescribeException()));
    static jmethodID const ctorSingleLaneInfoID = jni::GetConstructorID(env, laneClass, "([BZ)V");

    for (jsize j = 0; j < lanesSize; ++j)
    {
      auto const laneSize = static_cast<jsize>(lanes[j].m_lane.size());
      jni::TScopedLocalByteArrayRef singleLane(env, env->NewByteArray(laneSize));
      ASSERT(singleLane.get(), (jni::DescribeException()));
      env->SetByteArrayRegion(singleLane.get(), 0, laneSize, lanes[j].m_lane.data());

      jni::TScopedLocalRef singleLaneInfo(
          env, env->NewObject(laneClass, ctorSingleLaneInfoID, singleLane.get(),
                              lanes[j].m_isRecommended));
      ASSERT(singleLaneInfo.get(), (jni::DescribeException()));
      env->SetObjectArrayElement(jLanes, j, singleLaneInfo.get());
    }
  }

  auto const & rm = frm()->GetRoutingManager();
  auto const isSpeedCamLimitExceeded = rm.IsRoutingActive() ? rm.IsSpeedCamLimitExceeded() : false;
  auto const shouldPlaySignal = frm()->GetRoutingManager().GetSpeedCamManager().ShouldPlayBeepSignal();
  jobject const result = env->NewObject(
      klass, ctorRouteInfoID, ToJavaDistance(env, info.m_distToTarget),
      ToJavaDistance(env, info.m_distToTurn), jni::ToJavaString(env, info.m_currentStreetName),
      jni::ToJavaString(env, info.m_nextStreetName), jni::ToJavaString(env, info.m_nextNextStreetName),
      info.m_completionPercent, info.m_turn, info.m_nextTurn, info.m_pedestrianTurn, info.m_exitNum,
      info.m_time, jLanes, info.m_speedLimitMps, static_cast<jboolean>(isSpeedCamLimitExceeded),
      static_cast<jboolean>(shouldPlaySignal));
  ASSERT(result, (jni::DescribeException()));
  return result;
}

JNIEXPORT jobjectArray JNICALL
Java_app_organicmaps_Framework_nativeGetRouteJunctionPoints(JNIEnv * env, jclass)
{
  vector<m2::PointD> junctionPoints;
  if (!frm()->GetRoutingManager().RoutingSession().GetRouteJunctionPoints(junctionPoints))
  {
    LOG(LWARNING, ("Can't get the route junction points"));
    return nullptr;
  }

  static jclass const junctionClazz = jni::GetGlobalClassRef(env, "app/organicmaps/routing/JunctionInfo");
  // Java signature : JunctionInfo(double lat, double lon)
  static jmethodID const junctionConstructor = jni::GetConstructorID(env, junctionClazz, "(DD)V");

  return jni::ToJavaArray(env, junctionClazz, junctionPoints,
    [](JNIEnv * env, m2::PointD const & point)
    {
      return env->NewObject(junctionClazz, junctionConstructor,
                            mercator::YToLat(point.y),
                            mercator::XToLon(point.x));
    });
}

JNIEXPORT jintArray JNICALL
Java_app_organicmaps_Framework_nativeGenerateRouteAltitudeChartBits(JNIEnv * env, jclass, jint width, jint height, jobject routeAltitudeLimits)
{
  RoutingManager::DistanceAltitude altitudes;
  if (!frm()->GetRoutingManager().GetRouteAltitudesAndDistancesM(altitudes))
  {
    LOG(LWARNING, ("Can't get distance to route points and altitude."));
    return nullptr;
  }

  altitudes.Simplify();

  vector<uint8_t> imageRGBAData;
  if (!altitudes.GenerateRouteAltitudeChart(width, height, imageRGBAData))
  {
    LOG(LWARNING, ("Can't generate route altitude image."));
    return nullptr;
  }

  uint32_t totalAscent, totalDescent;
  altitudes.CalculateAscentDescent(totalAscent, totalDescent);

  // Android platform code has specific result string formatting, so make conversion here.
  using namespace measurement_utils;
  auto units = Units::Metric;
  if (settings::Get(settings::kMeasurementUnits, units) && units == Units::Imperial)
  {
    totalAscent = measurement_utils::MetersToFeet(totalAscent);
    totalDescent = measurement_utils::MetersToFeet(totalDescent);
  }

  jni::TScopedLocalRef const totalAscentString(env, jni::ToJavaString(env, ToStringPrecision(totalAscent, 0)));

  jni::TScopedLocalRef const totalDescentString(env, jni::ToJavaString(env, ToStringPrecision(totalDescent, 0)));

  // Passing route limits.
  // Do not use jni::GetGlobalClassRef, because this class is used only to init static fieldId vars.
  static jclass const routeAltitudeLimitsClass = env->GetObjectClass(routeAltitudeLimits);
  ASSERT(routeAltitudeLimitsClass, ());

  static jfieldID const totalAscentField = env->GetFieldID(routeAltitudeLimitsClass, "totalAscent", "I");
  ASSERT(totalAscentField, ());
  env->SetIntField(routeAltitudeLimits, totalAscentField, static_cast<jint>(totalAscent));

  static jfieldID const totalDescentField = env->GetFieldID(routeAltitudeLimitsClass, "totalDescent", "I");
  ASSERT(totalDescentField, ());
  env->SetIntField(routeAltitudeLimits, totalDescentField, static_cast<jint>(totalDescent));

  static jfieldID const totalAscentStringField = env->GetFieldID(routeAltitudeLimitsClass, "totalAscentString", "Ljava/lang/String;");
  ASSERT(totalAscentStringField, ());
  env->SetObjectField(routeAltitudeLimits, totalAscentStringField, totalAscentString.get());

  static jfieldID const totalDescentStringField = env->GetFieldID(routeAltitudeLimitsClass, "totalDescentString", "Ljava/lang/String;");
  ASSERT(totalDescentStringField, ());
  env->SetObjectField(routeAltitudeLimits, totalDescentStringField, totalDescentString.get());

  static jfieldID const isMetricUnitsField = env->GetFieldID(routeAltitudeLimitsClass, "isMetricUnits", "Z");
  ASSERT(isMetricUnitsField, ());
  env->SetBooleanField(routeAltitudeLimits, isMetricUnitsField, units == Units::Metric);

  size_t const imageRGBADataSize = imageRGBAData.size();
  ASSERT_NOT_EQUAL(imageRGBADataSize, 0, ("GenerateRouteAltitudeChart returns true but the vector with altitude image bits is empty."));

  size_t const pxlCount = width * height;
  if (maps::kAltitudeChartBPP * pxlCount != imageRGBADataSize)
  {
    LOG(LWARNING, ("Wrong size of vector with altitude image bits. Expected size:", pxlCount, ". Real size:", imageRGBADataSize));
    return nullptr;
  }

  jintArray imageRGBADataArray = env->NewIntArray(static_cast<jsize>(pxlCount));
  ASSERT(imageRGBADataArray, ());
  jint * arrayElements = env->GetIntArrayElements(imageRGBADataArray, 0);
  ASSERT(arrayElements, ());

  for (size_t i = 0; i < pxlCount; ++i)
  {
    size_t const shiftInBytes = i * maps::kAltitudeChartBPP;
    // Type of |imageRGBAData| elements is uint8_t. But uint8_t is promoted to unsinged int in code below before shifting.
    // So there's no data lost in code below.
    arrayElements[i] = (imageRGBAData[shiftInBytes + 3] << 24) /* alpha */
        | (imageRGBAData[shiftInBytes] << 16) /* red */
        | (imageRGBAData[shiftInBytes + 1] << 8) /* green */
        | (imageRGBAData[shiftInBytes + 2]); /* blue */
  }
  env->ReleaseIntArrayElements(imageRGBADataArray, arrayElements, 0);

  return imageRGBADataArray;
}

JNIEXPORT void JNICALL
Java_app_organicmaps_Framework_nativeShowCountry(JNIEnv * env, jclass, jstring countryId, jboolean zoomToDownloadButton)
{
  g_framework->ShowNode(jni::ToNativeString(env, countryId), (bool) zoomToDownloadButton);
}

JNIEXPORT void JNICALL
Java_app_organicmaps_Framework_nativeSetRoutingListener(JNIEnv * env, jclass, jobject listener)
{
  CHECK(g_framework, ("Framework isn't created yet!"));
  frm()->GetRoutingManager().SetRouteBuildingListener(
      [rf = jni::make_global_ref(listener)](routing::RouterResultCode e, storage::CountriesSet const & countries) {
        CallRoutingListener(rf, static_cast<int>(e), countries);
      });
}

JNIEXPORT void JNICALL
Java_app_organicmaps_Framework_nativeSetRouteProgressListener(JNIEnv * env, jclass, jobject listener)
{
  CHECK(g_framework, ("Framework isn't created yet!"));
  frm()->GetRoutingManager().SetRouteProgressListener(
      bind(&CallRouteProgressListener, jni::make_global_ref(listener), _1));
}

JNIEXPORT void JNICALL
Java_app_organicmaps_Framework_nativeSetRoutingRecommendationListener(JNIEnv * env, jclass,
                                                                          jobject listener)
{
  CHECK(g_framework, ("Framework isn't created yet!"));
  frm()->GetRoutingManager().SetRouteRecommendationListener(
      bind(&CallRouteRecommendationListener, jni::make_global_ref(listener), _1));
}

JNIEXPORT void JNICALL
Java_app_organicmaps_Framework_nativeSetRoutingLoadPointsListener(
        JNIEnv *, jclass, jobject listener)
{
  CHECK(g_framework, ("Framework isn't created yet!"));
  if (listener != nullptr)
    g_loadRouteHandler = bind(&CallSetRoutingLoadPointsListener, jni::make_global_ref(listener), _1);
  else
    g_loadRouteHandler = nullptr;
}

JNIEXPORT void JNICALL
Java_app_organicmaps_Framework_nativeDeactivatePopup(JNIEnv * env, jclass)
{
  return g_framework->DeactivatePopup();
}

JNIEXPORT void JNICALL
Java_app_organicmaps_Framework_nativeDeactivateMapSelectionCircle(JNIEnv * env, jclass)
{
  return g_framework->DeactivateMapSelectionCircle();
}

JNIEXPORT void JNICALL
Java_app_organicmaps_Framework_nativeSetMapStyle(JNIEnv * env, jclass, jint mapStyle)
{
  MapStyle const val = static_cast<MapStyle>(mapStyle);
  if (val != g_framework->GetMapStyle())
    g_framework->SetMapStyle(val);
}

JNIEXPORT jint JNICALL
Java_app_organicmaps_Framework_nativeGetMapStyle(JNIEnv * env, jclass)
{
  return g_framework->GetMapStyle();
}

JNIEXPORT void JNICALL
Java_app_organicmaps_Framework_nativeMarkMapStyle(JNIEnv * env, jclass, jint mapStyle)
{
  MapStyle const val = static_cast<MapStyle>(mapStyle);
  if (val != g_framework->GetMapStyle())
    g_framework->MarkMapStyle(val);
}

JNIEXPORT void JNICALL
Java_app_organicmaps_Framework_nativeSetRouter(JNIEnv * env, jclass, jint routerType)
{
  using Type = routing::RouterType;
  Type type = Type::Vehicle;
  switch (routerType)
  {
    case 0: break;
    case 1: type = Type::Pedestrian; break;
    case 2: type = Type::Bicycle; break;
    case 3: type = Type::Transit; break;
    case 4: type = Type::Ruler; break;
    default: assert(false); break;
  }
  g_framework->GetRoutingManager().SetRouter(type);
}

JNIEXPORT jint JNICALL
Java_app_organicmaps_Framework_nativeGetRouter(JNIEnv * env, jclass)
{
  return static_cast<jint>(g_framework->GetRoutingManager().GetRouter());
}

JNIEXPORT jint JNICALL
Java_app_organicmaps_Framework_nativeGetLastUsedRouter(JNIEnv * env, jclass)
{
  return static_cast<jint>(g_framework->GetRoutingManager().GetLastUsedRouter());
}

JNIEXPORT jint JNICALL
Java_app_organicmaps_Framework_nativeGetBestRouter(JNIEnv * env, jclass,
                                                       jdouble srcLat, jdouble srcLon,
                                                       jdouble dstLat, jdouble dstLon)
{
  return static_cast<jint>(frm()->GetRoutingManager().GetBestRouter(
      mercator::FromLatLon(srcLat, srcLon), mercator::FromLatLon(dstLat, dstLon)));
}

JNIEXPORT void JNICALL
Java_app_organicmaps_Framework_nativeAddRoutePoint(JNIEnv * env, jclass, jstring title,
                                                       jstring subtitle, jint markType,
                                                       jint intermediateIndex,
                                                       jboolean isMyPosition,
                                                       jdouble lat, jdouble lon)
{
  RouteMarkData data;
  data.m_title = jni::ToNativeString(env, title);
  data.m_subTitle = jni::ToNativeString(env, subtitle);
  data.m_pointType = static_cast<RouteMarkType>(markType);
  data.m_intermediateIndex = static_cast<size_t>(intermediateIndex);
  data.m_isMyPosition = static_cast<bool>(isMyPosition);
  data.m_position = m2::PointD(mercator::FromLatLon(lat, lon));

  frm()->GetRoutingManager().AddRoutePoint(std::move(data));
}

JNIEXPORT void JNICALL
Java_app_organicmaps_Framework_nativeRemoveRoutePoint(JNIEnv * env, jclass,
                                                          jint markType, jint intermediateIndex)
{
  frm()->GetRoutingManager().RemoveRoutePoint(static_cast<RouteMarkType>(markType),
                                              static_cast<size_t>(intermediateIndex));
}

JNIEXPORT void JNICALL
Java_app_organicmaps_Framework_nativeRemoveIntermediateRoutePoints(JNIEnv * env, jclass)
{
  frm()->GetRoutingManager().RemoveIntermediateRoutePoints();
}

JNIEXPORT jboolean JNICALL
Java_app_organicmaps_Framework_nativeCouldAddIntermediatePoint(JNIEnv * env, jclass)
{
  return frm()->GetRoutingManager().CouldAddIntermediatePoint();
}

JNIEXPORT jobjectArray JNICALL
Java_app_organicmaps_Framework_nativeGetRoutePoints(JNIEnv * env, jclass)
{
  auto const points = frm()->GetRoutingManager().GetRoutePoints();

  static jclass const pointClazz = jni::GetGlobalClassRef(env,
                                   "app/organicmaps/routing/RouteMarkData");
  // Java signature : RouteMarkData(String title, String subtitle,
  //                                @RoutePointInfo.RouteMarkType int pointType,
  //                                int intermediateIndex, boolean isVisible, boolean isMyPosition,
  //                                boolean isPassed, double lat, double lon)
  static jmethodID const pointConstructor = jni::GetConstructorID(env, pointClazz,
                                            "(Ljava/lang/String;Ljava/lang/String;IIZZZDD)V");
  return jni::ToJavaArray(env, pointClazz, points, [&](JNIEnv * jEnv, RouteMarkData const & data)
  {
    jni::TScopedLocalRef const title(env, jni::ToJavaString(env, data.m_title));
    jni::TScopedLocalRef const subtitle(env, jni::ToJavaString(env, data.m_subTitle));
    return env->NewObject(pointClazz, pointConstructor,
                          title.get(), subtitle.get(),
                          static_cast<jint>(data.m_pointType),
                          static_cast<jint>(data.m_intermediateIndex),
                          static_cast<jboolean>(data.m_isVisible),
                          static_cast<jboolean>(data.m_isMyPosition),
                          static_cast<jboolean>(data.m_isPassed),
                          mercator::YToLat(data.m_position.y),
                          mercator::XToLon(data.m_position.x));
  });
}

JNIEXPORT jobject JNICALL
Java_app_organicmaps_Framework_nativeGetTransitRouteInfo(JNIEnv * env, jclass)
{
  auto const routeInfo = frm()->GetRoutingManager().GetTransitRouteInfo();

  static jclass const transitStepClass = jni::GetGlobalClassRef(env,
                                         "app/organicmaps/routing/TransitStepInfo");
  // Java signature : TransitStepInfo(@TransitType int type, @Nullable String distance, @Nullable String distanceUnits,
  //                                  int timeInSec, @Nullable String number, int color, int intermediateIndex)
  static jmethodID const transitStepConstructor = jni::GetConstructorID(env, transitStepClass,
                                                  "(ILjava/lang/String;Ljava/lang/String;ILjava/lang/String;II)V");

  jni::TScopedLocalRef const steps(env, jni::ToJavaArray(env, transitStepClass,
                                                         routeInfo.m_steps,
                                                         [&](JNIEnv * jEnv, TransitStepInfo const & stepInfo)
  {
      jni::TScopedLocalRef const distance(env, jni::ToJavaString(env, stepInfo.m_distanceStr));
      jni::TScopedLocalRef const distanceUnits(env, jni::ToJavaString(env, stepInfo.m_distanceUnitsSuffix));
      jni::TScopedLocalRef const number(env, jni::ToJavaString(env, stepInfo.m_number));
      return env->NewObject(transitStepClass, transitStepConstructor,
                            static_cast<jint>(stepInfo.m_type),
                            distance.get(),
                            distanceUnits.get(),
                            static_cast<jint>(stepInfo.m_timeInSec),
                            number.get(),
                            static_cast<jint>(stepInfo.m_colorARGB),
                            static_cast<jint>(stepInfo.m_intermediateIndex));
  }));

  static jclass const transitRouteInfoClass = jni::GetGlobalClassRef(env,
                                                                     "app/organicmaps/routing/TransitRouteInfo");
  // Java signature : TransitRouteInfo(@NonNull String totalDistance, @NonNull String totalDistanceUnits, int totalTimeInSec,
  //                                   @NonNull String totalPedestrianDistance, @NonNull String totalPedestrianDistanceUnits,
  //                                   int totalPedestrianTimeInSec, @NonNull TransitStepInfo[] steps)
  static jmethodID const transitRouteInfoConstructor = jni::GetConstructorID(env, transitRouteInfoClass,
                                                                             "(Ljava/lang/String;Ljava/lang/String;I"
                                                                             "Ljava/lang/String;Ljava/lang/String;I"
                                                                             "[Lapp/organicmaps/routing/TransitStepInfo;)V");
  jni::TScopedLocalRef const distance(env, jni::ToJavaString(env, routeInfo.m_totalDistanceStr));
  jni::TScopedLocalRef const distanceUnits(env, jni::ToJavaString(env, routeInfo.m_totalDistanceUnitsSuffix));
  jni::TScopedLocalRef const distancePedestrian(env, jni::ToJavaString(env, routeInfo.m_totalPedestrianDistanceStr));
  jni::TScopedLocalRef const distancePedestrianUnits(env, jni::ToJavaString(env, routeInfo.m_totalPedestrianUnitsSuffix));
  return env->NewObject(transitRouteInfoClass, transitRouteInfoConstructor,
                        distance.get(), distanceUnits.get(), static_cast<jint>(routeInfo.m_totalTimeInSec),
                        distancePedestrian.get(), distancePedestrianUnits.get(), static_cast<jint>(routeInfo.m_totalPedestrianTimeInSec),
                        steps.get());
}

JNIEXPORT void JNICALL
Java_app_organicmaps_Framework_nativeReloadWorldMaps(JNIEnv * env, jclass)
{
  g_framework->ReloadWorldMaps();
}

JNIEXPORT jboolean JNICALL
Java_app_organicmaps_Framework_nativeIsDayTime(JNIEnv * env, jclass, jlong utcTimeSeconds, jdouble lat, jdouble lon)
{
  DayTimeType const dt = GetDayTime(static_cast<time_t>(utcTimeSeconds), lat, lon);
  return (dt == DayTimeType::Day || dt == DayTimeType::PolarDay);
}

JNIEXPORT void JNICALL
Java_app_organicmaps_Framework_nativeSet3dMode(JNIEnv * env, jclass, jboolean allow, jboolean allowBuildings)
{
  bool const allow3d = static_cast<bool>(allow);
  bool const allow3dBuildings = static_cast<bool>(allowBuildings);

  g_framework->Save3dMode(allow3d, allow3dBuildings);
  g_framework->Set3dMode(allow3d, allow3dBuildings);
}

JNIEXPORT void JNICALL
Java_app_organicmaps_Framework_nativeGet3dMode(JNIEnv * env, jclass, jobject result)
{
  bool enabled;
  bool buildings;
  g_framework->Get3dMode(enabled, buildings);

  static jclass const resultClass = env->GetObjectClass(result);

  static jfieldID const enabledField = env->GetFieldID(resultClass, "enabled", "Z");
  env->SetBooleanField(result, enabledField, enabled);

  static jfieldID const buildingsField = env->GetFieldID(resultClass, "buildings", "Z");
  env->SetBooleanField(result, buildingsField, buildings);
}

JNIEXPORT void JNICALL
Java_app_organicmaps_Framework_nativeSetAutoZoomEnabled(JNIEnv * env, jclass, jboolean enabled)
{
  bool const autoZoomEnabled = static_cast<bool>(enabled);
  frm()->SaveAutoZoom(autoZoomEnabled);
  frm()->AllowAutoZoom(autoZoomEnabled);
}

JNIEXPORT void JNICALL
Java_app_organicmaps_Framework_nativeSetTransitSchemeEnabled(JNIEnv * env, jclass, jboolean enabled)
{
  frm()->GetTransitManager().EnableTransitSchemeMode(static_cast<bool>(enabled));
}

JNIEXPORT jboolean JNICALL
Java_app_organicmaps_Framework_nativeIsTransitSchemeEnabled(JNIEnv * env, jclass)
{
  return static_cast<jboolean>(frm()->LoadTransitSchemeEnabled());
}

JNIEXPORT void JNICALL
Java_app_organicmaps_Framework_nativeSetIsolinesLayerEnabled(JNIEnv * env, jclass, jboolean enabled)
{
  auto const isolinesEnabled = static_cast<bool>(enabled);
  frm()->GetIsolinesManager().SetEnabled(isolinesEnabled);
  frm()->SaveIsolinesEnabled(isolinesEnabled);
}

JNIEXPORT jboolean JNICALL
Java_app_organicmaps_Framework_nativeIsIsolinesLayerEnabled(JNIEnv * env, jclass)
{
  return static_cast<jboolean>(frm()->LoadIsolinesEnabled());
}

JNIEXPORT void JNICALL
Java_app_organicmaps_Framework_nativeSetOutdoorsLayerEnabled(JNIEnv * env, jclass, jboolean enabled)
{
  frm()->SaveOutdoorsEnabled(enabled);
}

JNIEXPORT jboolean JNICALL
Java_app_organicmaps_Framework_nativeIsOutdoorsLayerEnabled(JNIEnv * env, jclass)
{
  return static_cast<jboolean>(frm()->LoadOutdoorsEnabled());
}

JNIEXPORT void JNICALL
Java_app_organicmaps_Framework_nativeSaveSettingSchemeEnabled(JNIEnv * env, jclass, jboolean enabled)
{
  frm()->SaveTransitSchemeEnabled(static_cast<bool>(enabled));
}

JNIEXPORT jboolean JNICALL
Java_app_organicmaps_Framework_nativeGetAutoZoomEnabled(JNIEnv *, jclass)
{
  return frm()->LoadAutoZoom();
}

// static void nativeZoomToPoint(double lat, double lon, int zoom, boolean animate);
JNIEXPORT void JNICALL
Java_app_organicmaps_Framework_nativeZoomToPoint(JNIEnv * env, jclass, jdouble lat, jdouble lon, jint zoom, jboolean animate)
{
  g_framework->Scale(m2::PointD(mercator::FromLatLon(lat, lon)), zoom, animate);
}

JNIEXPORT jobject JNICALL
Java_app_organicmaps_Framework_nativeDeleteBookmarkFromMapObject(JNIEnv * env, jclass)
{
  if (!frm()->HasPlacePageInfo())
    return nullptr;

  place_page::Info const & info = g_framework->GetPlacePageInfo();
  auto const bookmarkId = info.GetBookmarkId();
  frm()->GetBookmarkManager().GetEditSession().DeleteBookmark(bookmarkId);

  auto buildInfo = info.GetBuildInfo();
  buildInfo.m_match = place_page::BuildInfo::Match::FeatureOnly;
  buildInfo.m_userMarkId = kml::kInvalidMarkId;
  buildInfo.m_source = place_page::BuildInfo::Source::Other;
  frm()->UpdatePlacePageInfoForCurrentSelection(buildInfo);

  return usermark_helper::CreateMapObject(env, g_framework->GetPlacePageInfo());
}

JNIEXPORT jstring JNICALL
Java_app_organicmaps_Framework_nativeGetPoiContactUrl(JNIEnv *env, jclass, jint id)
{
  auto const metaID = static_cast<osm::MapObject::MetadataID>(id);
  string_view const value = g_framework->GetPlacePageInfo().GetMetadata(metaID);
  if (osm::isSocialContactTag(metaID))
    return jni::ToJavaString(env, osm::socialContactToURL(metaID, value));
  return jni::ToJavaString(env, value);
}

JNIEXPORT void JNICALL
Java_app_organicmaps_Framework_nativeSetChoosePositionMode(JNIEnv *, jclass, jint mode, jboolean isBusiness,
                                                           jboolean applyPosition)
{
  // TODO(AB): Move this code into the Framework to share with iOS and other platforms.
  auto const f = frm();
  if (applyPosition && f->HasPlacePageInfo())
    g_framework->SetChoosePositionMode(static_cast<android::ChoosePositionMode>(mode), isBusiness,
                                       &f->GetCurrentPlacePageInfo().GetMercator());
  else
    g_framework->SetChoosePositionMode(static_cast<android::ChoosePositionMode>(mode), isBusiness, nullptr);
}

JNIEXPORT jint JNICALL
Java_app_organicmaps_Framework_nativeGetChoosePositionMode(JNIEnv *, jclass)
{
  return static_cast<jint>(g_framework->GetChoosePositionMode());
}

JNIEXPORT jboolean JNICALL
Java_app_organicmaps_Framework_nativeIsDownloadedMapAtScreenCenter(JNIEnv *, jclass)
{
  ::Framework * fr = frm();
  return storage::IsPointCoveredByDownloadedMaps(fr->GetViewportCenter(), fr->GetStorage(), fr->GetCountryInfoGetter());
}

JNIEXPORT jstring JNICALL
Java_app_organicmaps_Framework_nativeGetActiveObjectFormattedCuisine(JNIEnv * env, jclass)
{
  ::Framework * frm = g_framework->NativeFramework();
  if (!frm->HasPlacePageInfo())
    return {};

  return jni::ToJavaString(env, g_framework->GetPlacePageInfo().FormatCuisines());
}

JNIEXPORT void JNICALL
Java_app_organicmaps_Framework_nativeSetVisibleRect(JNIEnv * env, jclass, jint left, jint top, jint right, jint bottom)
{
  frm()->SetVisibleViewport(m2::RectD(left, top, right, bottom));
}

JNIEXPORT jboolean JNICALL
Java_app_organicmaps_Framework_nativeIsRouteFinished(JNIEnv * env, jclass)
{
  return frm()->GetRoutingManager().IsRouteFinished();
}

JNIEXPORT void JNICALL
Java_app_organicmaps_Framework_nativeRunFirstLaunchAnimation(JNIEnv * env, jclass)
{
  frm()->RunFirstLaunchAnimation();
}

JNIEXPORT jint JNICALL
Java_app_organicmaps_Framework_nativeOpenRoutePointsTransaction(JNIEnv * env, jclass)
{
  return frm()->GetRoutingManager().OpenRoutePointsTransaction();
}

JNIEXPORT void JNICALL
Java_app_organicmaps_Framework_nativeApplyRoutePointsTransaction(JNIEnv * env, jclass,
                                                                     jint transactionId)
{
  frm()->GetRoutingManager().ApplyRoutePointsTransaction(transactionId);
}

JNIEXPORT void JNICALL
Java_app_organicmaps_Framework_nativeCancelRoutePointsTransaction(JNIEnv * env, jclass,
                                                                      jint transactionId)
{
  frm()->GetRoutingManager().CancelRoutePointsTransaction(transactionId);
}

JNIEXPORT jint JNICALL
Java_app_organicmaps_Framework_nativeInvalidRoutePointsTransactionId(JNIEnv * env, jclass)
{
  return frm()->GetRoutingManager().InvalidRoutePointsTransactionId();
}

JNIEXPORT jboolean JNICALL
Java_app_organicmaps_Framework_nativeHasSavedRoutePoints(JNIEnv *, jclass)
{
  return frm()->GetRoutingManager().HasSavedRoutePoints();
}

JNIEXPORT void JNICALL
Java_app_organicmaps_Framework_nativeLoadRoutePoints(JNIEnv *, jclass)
{
  frm()->GetRoutingManager().LoadRoutePoints(g_loadRouteHandler);
}

JNIEXPORT void JNICALL
Java_app_organicmaps_Framework_nativeSaveRoutePoints(JNIEnv *, jclass)
{
  frm()->GetRoutingManager().SaveRoutePoints();
}

JNIEXPORT void JNICALL
Java_app_organicmaps_Framework_nativeDeleteSavedRoutePoints(JNIEnv *, jclass)
{
  frm()->GetRoutingManager().DeleteSavedRoutePoints();
}

JNIEXPORT void JNICALL
Java_app_organicmaps_Framework_nativeShowFeature(JNIEnv * env, jclass, jobject featureId)
{
  auto const f = g_framework->BuildFeatureId(env, featureId);

  if (f.IsValid())
    frm()->ShowFeature(f);
}

JNIEXPORT void JNICALL
Java_app_organicmaps_Framework_nativeMakeCrash(JNIEnv *env, jclass type)
{
  CHECK(false, ("Diagnostic native crash!"));
}

JNIEXPORT void JNICALL
Java_app_organicmaps_Framework_nativeSetPowerManagerFacility(JNIEnv *, jclass,
                                                                 jint facilityType, jboolean state)
{
  frm()->GetPowerManager().SetFacility(static_cast<power_management::Facility>(facilityType),
                                       static_cast<bool>(state));
}

JNIEXPORT jint JNICALL
Java_app_organicmaps_Framework_nativeGetPowerManagerScheme(JNIEnv *, jclass)
{
  return static_cast<jint>(frm()->GetPowerManager().GetScheme());
}

JNIEXPORT void JNICALL
Java_app_organicmaps_Framework_nativeSetPowerManagerScheme(JNIEnv *, jclass, jint schemeType)
{
  frm()->GetPowerManager().SetScheme(static_cast<power_management::Scheme>(schemeType));
}

JNIEXPORT void JNICALL
Java_app_organicmaps_Framework_nativeSetViewportCenter(JNIEnv *, jclass, jdouble lat, jdouble lon, jint zoom)
{
  // isAnim = true because of previous nativeSetChoosePositionMode animations.
  frm()->SetViewportCenter(mercator::FromLatLon(lat, lon), static_cast<int>(zoom), true /* isAnim */);
}

JNIEXPORT void JNICALL
Java_app_organicmaps_Framework_nativeStopLocationFollow(JNIEnv *, jclass)
{
  frm()->StopLocationFollow();
}

JNIEXPORT void JNICALL
Java_app_organicmaps_Framework_nativeSetSearchViewport(JNIEnv *, jclass, jdouble lat,
                                                           jdouble lon, jint zoom)
{
  auto const center = mercator::FromLatLon(static_cast<double>(lat),
                                           static_cast<double>(lon));
  auto const rect = df::GetRectForDrawScale(static_cast<int>(zoom), center);
  frm()->GetSearchAPI().OnViewportChanged(rect);
}

JNIEXPORT jboolean JNICALL
Java_app_organicmaps_Framework_nativeHasPlacePageInfo(JNIEnv *, jclass)
{
  return static_cast<jboolean>(frm()->HasPlacePageInfo());
}

JNIEXPORT void JNICALL
Java_app_organicmaps_Framework_nativeMemoryWarning(JNIEnv *, jclass)
{
  return frm()->MemoryWarning();
}

JNIEXPORT jstring JNICALL
Java_app_organicmaps_Framework_nativeGetKayakHotelLink(JNIEnv * env, jclass, jstring countryIsoCode, jstring uri,
                                                        jlong firstDaySec, jlong lastDaySec)
{
  string const url = osm::GetKayakHotelURLFromURI(jni::ToNativeString(env, countryIsoCode),
                                                  jni::ToNativeString(env, uri),
                                                  static_cast<time_t>(firstDaySec),
                                                  static_cast<time_t>(lastDaySec));
  return url.empty() ? nullptr : jni::ToJavaString(env, url);
}

JNIEXPORT jboolean JNICALL
Java_app_organicmaps_Framework_nativeShouldShowProducts(JNIEnv * env, jclass)
{
  return frm()->ShouldShowProducts();
}

JNIEXPORT jobject JNICALL
Java_app_organicmaps_Framework_nativeGetProductsConfiguration(JNIEnv * env, jclass)
{
  auto config = frm()->GetProductsConfiguration();
  if (!config) return nullptr;

  static jclass const productClass = jni::GetGlobalClassRef(
    env,
    "app/organicmaps/products/Product"
  );
  static jmethodID const productConstructor = jni::GetConstructorID(
    env,
    productClass,
    "(Ljava/lang/String;Ljava/lang/String;)V"
  );

  jobjectArray products = jni::ToJavaArray(
    env,
    productClass,
    config->GetProducts(),
    [](JNIEnv * env, products::ProductsConfig::Product const & product)
    {
      jni::TScopedLocalRef const title(env, jni::ToJavaString(env, product.GetTitle()));
      jni::TScopedLocalRef const link(env, jni::ToJavaString(env, product.GetLink()));

      return env->NewObject(
        productClass,
        productConstructor,
        title.get(),
        link.get()
      );
    });

  static jclass const productsConfigClass = jni::GetGlobalClassRef(
    env,
    "app/organicmaps/products/ProductsConfig"
  );
  static jmethodID const productsConfigConstructor = jni::GetConstructorID(
    env,
    productsConfigClass,
    "(Ljava/lang/String;[Lapp/organicmaps/products/Product;)V"
  );

  jni::TScopedLocalRef const placePagePrompt(env, jni::ToJavaString(env, config->GetPlacePagePrompt()));
  return env->NewObject(productsConfigClass, productsConfigConstructor, placePagePrompt.get(), products);
}

JNIEXPORT void JNICALL
Java_app_organicmaps_Framework_nativeDidCloseProductsPopup(JNIEnv * env, jclass, jstring reason)
{
  frm()->DidCloseProductsPopup(frm()->FromString(jni::ToNativeString(env, reason)));
}

JNIEXPORT void JNICALL
Java_app_organicmaps_Framework_nativeDidSelectProduct(JNIEnv * env, jclass, jstring title, jstring link)
{
  products::ProductsConfig::Product product(
    jni::ToNativeString(env, title),
    jni::ToNativeString(env, link)
  );

  frm()->DidSelectProduct(product);
}

}  // extern "C"
