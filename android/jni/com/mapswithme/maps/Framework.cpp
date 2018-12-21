#include "com/mapswithme/maps/Framework.hpp"
#include "com/mapswithme/core/jni_helper.hpp"
#include "com/mapswithme/maps/UserMarkHelper.hpp"
#include "com/mapswithme/opengl/androidoglcontextfactory.hpp"
#include "com/mapswithme/platform/Platform.hpp"
#include "com/mapswithme/util/NetworkPolicy.hpp"

#include "map/chart_generator.hpp"
#include "map/everywhere_search_params.hpp"
#include "map/user_mark.hpp"

#include "partners_api/ads_engine.hpp"
#include "partners_api/banner.hpp"
#include "partners_api/booking_block_params.hpp"
#include "partners_api/mopub_ads.hpp"
#include "partners_api/megafon_countries.hpp"

#include "storage/storage_helpers.hpp"

#include "drape/pointers.hpp"
#include "drape/visual_scale.hpp"
#include "drape_frontend/user_event_stream.hpp"
#include "drape_frontend/visual_params.hpp"

#include "coding/file_container.hpp"
#include "coding/file_name_utils.hpp"

#include "geometry/angles.hpp"

#include "indexer/feature_altitude.hpp"

#include "routing/speed_camera_manager.hpp"

#include "platform/country_file.hpp"
#include "platform/local_country_file.hpp"
#include "platform/local_country_file_utils.hpp"
#include "platform/location.hpp"
#include "platform/measurement_utils.hpp"
#include "platform/network_policy.hpp"
#include "platform/platform.hpp"
#include "platform/preferred_languages.hpp"
#include "platform/settings.hpp"

#include "base/assert.hpp"
#include "base/logging.hpp"
#include "base/math.hpp"
#include "base/sunrise_sunset.hpp"

#include <cstdint>
#include <memory>
#include <string>
#include <utility>
#include <vector>

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

namespace
{
::Framework * frm()
{
  return g_framework->NativeFramework();
}

jobject g_mapObjectListener = nullptr;
int const kUndefinedTip = -1;
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

Framework::Framework()
  : m_lastCompass(0.0)
  , m_isContextDestroyed(false)
  , m_currentMode(location::PendingPosition)
  , m_isCurrentModeInitialized(false)
  , m_isChoosePositionMode(false)
{
  m_work.GetTrafficManager().SetStateListener(bind(&Framework::TrafficStateChanged, this, _1));
  m_work.GetTransitManager().SetStateListener(bind(&Framework::TransitSchemeStateChanged, this, _1));
  m_work.GetPowerManager().Subscribe(this);
}

void Framework::OnLocationError(int errorCode)
{
  m_work.OnLocationError(static_cast<location::TLocationError>(errorCode));
}

void Framework::OnLocationUpdated(location::GpsInfo const & info)
{
  ASSERT(IsDrapeEngineCreated(), ());
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

void Framework::UpdateCompassSensor(int ind, float * arr)
{
  m_sensors[ind].Next(arr);
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

bool Framework::CreateDrapeEngine(JNIEnv * env, jobject jSurface, int densityDpi, bool firstLaunch,
                                  bool launchByDeepLink)
{
  m_contextFactory = make_unique_dp<dp::ThreadSafeFactory>(new AndroidOGLContextFactory(env, jSurface));
  AndroidOGLContextFactory const * factory = m_contextFactory->CastFactory<AndroidOGLContextFactory>();
  if (!factory->IsValid())
  {
    LOG(LWARNING, ("Invalid GL context."));
    return false;
  }

  ::Framework::DrapeCreationParams p;
  p.m_apiVersion = factory->IsSupportedOpenGLES3() ? dp::ApiVersion::OpenGLES3 :
                                                     dp::ApiVersion::OpenGLES2;
  p.m_surfaceWidth = factory->GetWidth();
  p.m_surfaceHeight = factory->GetHeight();
  p.m_visualScale = dp::VisualScale(densityDpi);
  p.m_hasMyPositionState = m_isCurrentModeInitialized;
  p.m_initialMyPositionState = m_currentMode;
  p.m_isChoosePositionMode = m_isChoosePositionMode;
  p.m_hints.m_isFirstLaunch = firstLaunch;
  p.m_hints.m_isLaunchByDeepLink = launchByDeepLink;
  ASSERT(!m_guiPositions.empty(), ("GUI elements must be set-up before engine is created"));
  p.m_widgetsInitInfo = m_guiPositions;

  m_work.SetMyPositionModeListener(bind(&Framework::MyPositionModeChanged, this, _1, _2));

  m_work.CreateDrapeEngine(make_ref(m_contextFactory), move(p));
  m_work.EnterForeground();

  return true;
}

bool Framework::IsDrapeEngineCreated()
{
  return m_work.IsDrapeEngineCreated();
}

void Framework::Resize(int w, int h)
{
  m_contextFactory->CastFactory<AndroidOGLContextFactory>()->UpdateSurfaceSize(w, h);
  m_work.OnSize(w, h);

  //TODO: remove after correct visible rect calculation.
  frm()->SetVisibleViewport(m2::RectD(0, 0, w, h));
}

void Framework::DetachSurface(bool destroyContext)
{
  LOG(LINFO, ("Detach surface started. destroyContext =", destroyContext));
  ASSERT(m_contextFactory != nullptr, ());
  m_contextFactory->SetPresentAvailable(false);

  if (destroyContext)
  {
    LOG(LINFO, ("Destroy context."));
    m_isContextDestroyed = true;
    m_work.EnterBackground();
    m_work.OnDestroyGLContext();
  }
  m_work.SetRenderingDisabled(destroyContext);

  AndroidOGLContextFactory * factory = m_contextFactory->CastFactory<AndroidOGLContextFactory>();
  factory->ResetSurface();
  LOG(LINFO, ("Detach surface finished."));
}

bool Framework::AttachSurface(JNIEnv * env, jobject jSurface)
{
  LOG(LINFO, ("Attach surface started."));

  ASSERT(m_contextFactory != nullptr, ());
  AndroidOGLContextFactory * factory = m_contextFactory->CastFactory<AndroidOGLContextFactory>();
  factory->SetSurface(env, jSurface);

  if (!factory->IsValid())
  {
    LOG(LWARNING, ("Invalid GL context."));
    return false;
  }

  ASSERT(!m_guiPositions.empty(), ("GUI elements must be set-up before engine is created"));

  m_contextFactory->SetPresentAvailable(true);
  m_work.SetRenderingEnabled(factory);

  if (m_isContextDestroyed)
  {
    LOG(LINFO, ("Recover GL resources, viewport size:", factory->GetWidth(), factory->GetHeight()));
    m_work.OnRecoverGLContext(factory->GetWidth(), factory->GetHeight());
    m_isContextDestroyed = false;

    m_work.EnterForeground();
  }

  LOG(LINFO, ("Attach surface finished."));

  return true;
}

void Framework::PauseSurfaceRendering()
{
  if (m_contextFactory == nullptr)
    return;
  LOG(LINFO, ("Pause surface rendering."));
  m_contextFactory->SetPresentAvailable(false);
}

void Framework::ResumeSurfaceRendering()
{
  if (m_contextFactory == nullptr)
    return;
  LOG(LINFO, ("Resume surface rendering."));
  AndroidOGLContextFactory * factory = m_contextFactory->CastFactory<AndroidOGLContextFactory>();
  if (factory->IsValid())
    m_contextFactory->SetPresentAvailable(true);
}

void Framework::SetMapStyle(MapStyle mapStyle)
{
  m_work.SetMapStyle(mapStyle);
}

void Framework::MarkMapStyle(MapStyle mapStyle)
{
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

void Framework::SetChoosePositionMode(bool isChoosePositionMode, bool isBusiness,
                                      bool hasPosition, m2::PointD const & position)
{
  m_isChoosePositionMode = isChoosePositionMode;
  m_work.BlockTapEvents(isChoosePositionMode);
  m_work.EnableChoosePositionMode(isChoosePositionMode, isBusiness, hasPosition, position);
}

bool Framework::GetChoosePositionMode()
{
  return m_isChoosePositionMode;
}

Storage & Framework::GetStorage()
{
  return m_work.GetStorage();
}

DataSource const & Framework::GetDataSource() { return m_work.GetDataSource(); }

void Framework::ShowNode(TCountryId const & idx, bool zoomToDownloadButton)
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
  return m_work.SearchEverywhere(params);
}

void Framework::AddLocalMaps()
{
  m_work.RegisterAllMaps();
}

void Framework::RemoveLocalMaps()
{
  m_work.DeregisterAllMaps();
}

void Framework::ReplaceBookmark(kml::MarkId markId, kml::BookmarkData & bm)
{
  m_work.GetBookmarkManager().GetEditSession().UpdateBookmark(markId, bm);
}

void Framework::MoveBookmark(kml::MarkId markId, kml::MarkGroupId curCat, kml::MarkGroupId newCat)
{
  m_work.GetBookmarkManager().GetEditSession().MoveBookmark(markId, curCat, newCat);
}

bool Framework::ShowMapForURL(string const & url)
{
  return m_work.ShowMapForURL(url);
}

void Framework::DeactivatePopup()
{
  m_work.DeactivateMapSelection(false);
}

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

void Framework::SetTrafficStateListener(TrafficManager::TrafficStateChangedFn const & fn)
{
  m_onTrafficStateChangedFn = fn;
}

void Framework::SetTransitSchemeListener(TransitReadManager::TransitStateChangedFn const & function)
{
  m_onTransitStateChangedFn = function;
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

location::EMyPositionMode Framework::GetMyPositionMode()
{
  if (!m_isCurrentModeInitialized)
  {
    if (!settings::Get(settings::kLocationStateMode, m_currentMode))
      m_currentMode = location::NotFollowNoPosition;

    m_isCurrentModeInitialized = true;
  }

  return m_currentMode;
}

void Framework::OnMyPositionModeChanged(location::EMyPositionMode mode)
{
  m_currentMode = mode;
  m_isCurrentModeInitialized = true;
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

void Framework::ApplyWidgets()
{
  gui::TWidgetsLayoutInfo layout;
  for (auto const & widget : m_guiPositions)
    layout[widget.first] = widget.second.m_pixelPivot;

  m_work.SetWidgetLayout(move(layout));
}

void Framework::CleanWidgets()
{
  m_guiPositions.clear();
}

void Framework::SetupMeasurementSystem()
{
  m_work.SetupMeasurementSystem();
}

void Framework::SetPlacePageInfo(place_page::Info const & info)
{
  m_info = info;
}

place_page::Info & Framework::GetPlacePageInfo()
{
  return m_info;
}

void Framework::RequestBookingMinPrice(JNIEnv * env, jobject policy,
                                       booking::BlockParams && params,
                                       booking::BlockAvailabilityCallback const & callback)
{
  auto const bookingApi = m_work.GetBookingApi(ToNativeNetworkPolicy(env, policy));
  if (bookingApi)
    bookingApi->GetBlockAvailability(move(params), callback);
}

void Framework::RequestBookingInfo(JNIEnv * env, jobject policy,
                                   string const & hotelId, string const & lang,
                                   booking::GetHotelInfoCallback const & callback)
{
  auto const bookingApi = m_work.GetBookingApi(ToNativeNetworkPolicy(env, policy));
  if (bookingApi)
    bookingApi->GetHotelInfo(hotelId, lang, callback);
}

bool Framework::HasSpaceForMigration()
{
  return m_work.IsEnoughSpaceForMigrate();
}

void Framework::Migrate(bool keepOldMaps)
{
  m_work.Migrate(keepOldMaps);
}

storage::TCountryId Framework::PreMigrate(ms::LatLon const & position, Storage::TChangeCountryFunction const & statusChangeListener,
                                                                       Storage::TProgressFunction const & progressListener)
{
  return m_work.PreMigrate(position, statusChangeListener, progressListener);
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

void Framework::DisableAdProvider(ads::Banner::Type const type, ads::Banner::Place const place)
{
  m_work.DisableAdProvider(type, place);
}

uint64_t Framework::RequestTaxiProducts(JNIEnv * env, jobject policy, ms::LatLon const & from,
                                        ms::LatLon const & to,
                                        taxi::SuccessCallback const & onSuccess,
                                        taxi::ErrorCallback const & onError)
{
  auto const taxiEngine = m_work.GetTaxiEngine(ToNativeNetworkPolicy(env, policy));
  if (!taxiEngine)
    return 0;

  return taxiEngine->GetAvailableProducts(from, to, onSuccess, onError);
}

taxi::RideRequestLinks Framework::GetTaxiLinks(JNIEnv * env, jobject policy, taxi::Provider::Type type,
                                               string const & productId, ms::LatLon const & from,
                                               ms::LatLon const & to)
{
  auto const taxiEngine = m_work.GetTaxiEngine(ToNativeNetworkPolicy(env, policy));
  if (!taxiEngine)
    return {};

  return taxiEngine->GetRideRequestLinks(type, productId, from, to);
}

void Framework::RequestViatorProducts(JNIEnv * env, jobject policy, std::string const & destId,
                                      std::string const & currency,
                                      viator::GetTop5ProductsCallback const & callback)
{
  auto const viatorApi = m_work.GetViatorApi(ToNativeNetworkPolicy(env, policy));
  if (!viatorApi)
    return;

  viatorApi->GetTop5Products(destId, currency, callback);
}

void Framework::RequestUGC(FeatureID const & fid, ugc::Api::UGCCallback const & ugcCallback)
{
  m_work.GetUGC(fid, ugcCallback);
}

void Framework::SetUGCUpdate(FeatureID const & fid, ugc::UGCUpdate const & ugc)
{
  m_work.GetUGCApi()->SetUGCUpdate(fid, ugc);
}

void Framework::UploadUGC()
{
  m_work.UploadUGC(nullptr /* onCompleteUploading */);
}

int Framework::ToDoAfterUpdate() const
{
  return (int) m_work.ToDoAfterUpdate();
}

uint64_t Framework::GetLocals(JNIEnv * env, jobject policy, double lat, double lon,
                              locals::LocalsSuccessCallback const & successFn,
                              locals::LocalsErrorCallback const & errorFn)
{
  auto api = NativeFramework()->GetLocalsApi(ToNativeNetworkPolicy(env, policy));
  if (api == nullptr)
    return 0;

  std::string const langStr = languages::GetCurrentNorm();
  size_t constexpr kResultsOnPage = 5;
  size_t constexpr kPageNumber = 1;
  return api->GetLocals(lat, lon, langStr, kResultsOnPage, kPageNumber, successFn, errorFn);
}

void Framework::LogLocalAdsEvent(local_ads::EventType type, double lat, double lon, uint16_t accuracy)
{
  auto const & info = g_framework->GetPlacePageInfo();
  auto const & featureID = info.GetID();
  auto const & mwmInfo = featureID.m_mwmId.GetInfo();
  if (!mwmInfo)
    return;

  local_ads::Event event(type, mwmInfo->GetVersion(), mwmInfo->GetCountryName(), featureID.m_index,
                         m_work.GetDrawScale(), local_ads::Clock::now(), lat, lon, accuracy);
  m_work.GetLocalAdsManager().GetStatistics().RegisterEvent(std::move(event));
}

void Framework::OnPowerFacilityChanged(PowerManager::Facility const facility, bool enabled)
{
  // Dummy
  // TODO: provide information for UI Properties.
}

void Framework::OnPowerSchemeChanged(PowerManager::Scheme const actualScheme)
{
  // Dummy
  // TODO: provide information for UI Properties.
}
}  // namespace android

//============ GLUE CODE for com.mapswithme.maps.Framework class =============//
/*            ____
 *          _ |||| _
 *          \\    //
 *           \\  //
 *            \\//
 *             \/
 */

extern "C"
{
void CallRoutingListener(shared_ptr<jobject> listener, int errorCode, vector<storage::TCountryId> const & absentMaps)
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

void CallPurchaseValidationListener(shared_ptr<jobject> listener, Purchase::ValidationCode code,
                                    Purchase::ValidationInfo const & validationInfo)
{
  JNIEnv * env = jni::GetEnv();
  jmethodID const methodId = jni::GetMethodID(env, *listener, "onValidatePurchase",
    "(ILjava/lang/String;Ljava/lang/String;Ljava/lang/String;)V");

  jni::TScopedLocalRef const serverId(env, jni::ToJavaString(env, validationInfo.m_serverId));
  jni::TScopedLocalRef const vendorId(env, jni::ToJavaString(env, validationInfo.m_vendorId));
  jni::TScopedLocalRef const receiptData(env, jni::ToJavaString(env, validationInfo.m_receiptData));

  env->CallVoidMethod(*listener, methodId, static_cast<jint>(code), serverId.get(), vendorId.get(),
                      receiptData.get());
}

void CallStartPurchaseTransactionListener(shared_ptr<jobject> listener, bool success,
                                          std::string const & serverId,
                                          std::string const & vendorId)
{
  JNIEnv * env = jni::GetEnv();
  jmethodID const methodId = jni::GetMethodID(env, *listener, "onStartTransaction",
                                              "(ZLjava/lang/String;Ljava/lang/String;)V");

  jni::TScopedLocalRef const serverIdStr(env, jni::ToJavaString(env, serverId));
  jni::TScopedLocalRef const vendorIdStr(env, jni::ToJavaString(env, vendorId));

  env->CallVoidMethod(*listener, methodId, static_cast<jboolean>(success),
                      serverIdStr.get(), vendorIdStr.get());
}

/// @name JNI EXPORTS
//@{
JNIEXPORT jstring JNICALL
Java_com_mapswithme_maps_Framework_nativeGetNameAndAddress(JNIEnv * env, jclass clazz, jdouble lat, jdouble lon)
{
  search::AddressInfo const info = frm()->GetAddressInfoAtPoint(MercatorBounds::FromLatLon(lat, lon));
  return jni::ToJavaString(env, info.FormatNameAndAddress());
}

JNIEXPORT void JNICALL
Java_com_mapswithme_maps_Framework_nativeClearApiPoints(JNIEnv * env, jclass clazz)
{
  frm()->GetBookmarkManager().GetEditSession().ClearGroup(UserMark::Type::API);
}

JNIEXPORT jint JNICALL
Java_com_mapswithme_maps_Framework_nativeParseAndSetApiUrl(JNIEnv * env, jclass clazz, jstring url)
{
  return static_cast<jint>(frm()->ParseAndSetApiURL(jni::ToNativeString(env, url)));
}

JNIEXPORT jobject JNICALL
Java_com_mapswithme_maps_Framework_nativeGetParsedRoutingData(JNIEnv * env, jclass clazz)
{
  using namespace url_scheme;
  static jclass const pointClazz = jni::GetGlobalClassRef(env, "com/mapswithme/maps/api/RoutePoint");
  // Java signature : RoutePoint(double lat, double lon, String name)
  static jmethodID const pointConstructor = jni::GetConstructorID(env, pointClazz, "(DDLjava/lang/String;)V");

  static jclass const routeDataClazz = jni::GetGlobalClassRef(env, "com/mapswithme/maps/api/ParsedRoutingData");
  // Java signature : ParsedRoutingData(RoutePoint[] points, int routerType) {
  static jmethodID const routeDataConstructor = jni::GetConstructorID(env, routeDataClazz, "([Lcom/mapswithme/maps/api/RoutePoint;I)V");

  auto const & routingData = frm()->GetParsedRoutingData();
  jobjectArray points = jni::ToJavaArray(env, pointClazz, routingData.m_points,
                                         [](JNIEnv * env, RoutePoint const & point)
                                         {
                                           jni::TScopedLocalRef const name(env, jni::ToJavaString(env, point.m_name));
                                           return env->NewObject(pointClazz, pointConstructor,
                                                                 MercatorBounds::YToLat(point.m_org.y),
                                                                 MercatorBounds::XToLon(point.m_org.x), name.get());
                                         });

  return env->NewObject(routeDataClazz, routeDataConstructor, points, routingData.m_type);
}

JNIEXPORT jobject JNICALL
Java_com_mapswithme_maps_Framework_nativeGetParsedSearchRequest(JNIEnv * env, jclass clazz)
{
  using namespace url_scheme;
  static jclass const cl = jni::GetGlobalClassRef(env, "com/mapswithme/maps/api/ParsedSearchRequest");
  // Java signature : ParsedSearchRequest(String query, String locale, double lat, double lon, boolean isSearchOnMap)
  static jmethodID const ctor = jni::GetConstructorID(env, cl, "(Ljava/lang/String;Ljava/lang/String;DDZ)V");
  auto const & r = frm()->GetParsedSearchRequest();
  return env->NewObject(cl, ctor, jni::ToJavaString(env, r.m_query), jni::ToJavaString(env, r.m_locale), r.m_centerLat, r.m_centerLon, r.m_isSearchOnMap);
}

JNIEXPORT void JNICALL
Java_com_mapswithme_maps_Framework_nativeSetMapObjectListener(JNIEnv * env, jclass clazz, jobject jListener)
{
  LOG(LINFO, ("Set global map object listener"));
  g_mapObjectListener = env->NewGlobalRef(jListener);
  // void onMapObjectActivated(MapObject object);
  jmethodID const activatedId = jni::GetMethodID(env, g_mapObjectListener, "onMapObjectActivated",
                                                 "(Lcom/mapswithme/maps/bookmarks/data/MapObject;)V");
  // void onDismiss(boolean switchFullScreenMode);
  jmethodID const dismissId = jni::GetMethodID(env, g_mapObjectListener, "onDismiss", "(Z)V");
  frm()->SetMapSelectionListeners([activatedId](place_page::Info const & info)
  {
    JNIEnv * env = jni::GetEnv();
    g_framework->SetPlacePageInfo(info);
    jni::TScopedLocalRef mapObject(env, usermark_helper::CreateMapObject(env, info));
    env->CallVoidMethod(g_mapObjectListener, activatedId, mapObject.get());
  }, [dismissId](bool switchFullScreenMode)
  {
    JNIEnv * env = jni::GetEnv();
    g_framework->SetPlacePageInfo({});
    env->CallVoidMethod(g_mapObjectListener, dismissId, switchFullScreenMode);
  });
}

JNIEXPORT void JNICALL
Java_com_mapswithme_maps_Framework_nativeRemoveMapObjectListener(JNIEnv * env, jclass)
{
  if (g_mapObjectListener == nullptr)
    return;

  frm()->SetMapSelectionListeners({}, {});
  LOG(LINFO, ("Remove global map object listener"));
  env->DeleteGlobalRef(g_mapObjectListener);
  g_mapObjectListener = nullptr;
}

JNIEXPORT jstring JNICALL
Java_com_mapswithme_maps_Framework_nativeGetGe0Url(JNIEnv * env, jclass, jdouble lat, jdouble lon, jdouble zoomLevel, jstring name)
{
  ::Framework * fr = frm();
  double const scale = (zoomLevel > 0 ? zoomLevel : fr->GetDrawScale());
  string const url = fr->CodeGe0url(lat, lon, scale, jni::ToNativeString(env, name));
  return jni::ToJavaString(env, url);
}

JNIEXPORT jobject JNICALL
Java_com_mapswithme_maps_Framework_nativeGetDistanceAndAzimuth(
    JNIEnv * env, jclass, jdouble merX, jdouble merY, jdouble cLat, jdouble cLon, jdouble north)
{
  string distance;
  double azimut = -1.0;
  frm()->GetDistanceAndAzimut(m2::PointD(merX, merY), cLat, cLon, north, distance, azimut);

  static jclass const daClazz = jni::GetGlobalClassRef(env, "com/mapswithme/maps/bookmarks/data/DistanceAndAzimut");
  // Java signature : DistanceAndAzimut(String distance, double azimuth)
  static jmethodID const methodID = jni::GetConstructorID(env, daClazz, "(Ljava/lang/String;D)V");

  return env->NewObject(daClazz, methodID,
                        jni::ToJavaString(env, distance.c_str()),
                        static_cast<jdouble>(azimut));
}

JNIEXPORT jobject JNICALL
Java_com_mapswithme_maps_Framework_nativeGetDistanceAndAzimuthFromLatLon(
    JNIEnv * env, jclass clazz, jdouble lat, jdouble lon, jdouble cLat, jdouble cLon, jdouble north)
{
  double const merY = MercatorBounds::LatToY(lat);
  double const merX = MercatorBounds::LonToX(lon);
  return Java_com_mapswithme_maps_Framework_nativeGetDistanceAndAzimuth(env, clazz, merX, merY, cLat, cLon, north);
}

JNIEXPORT jobject JNICALL
Java_com_mapswithme_maps_Framework_nativeFormatLatLon(JNIEnv * env, jclass, jdouble lat, jdouble lon, jboolean useDMSFormat)
{
  return jni::ToJavaString(
      env, (useDMSFormat ? measurement_utils::FormatLatLonAsDMS(lat, lon, 2)
                         : measurement_utils::FormatLatLon(lat, lon, true /* withSemicolon */, 6)));
}

JNIEXPORT jobjectArray JNICALL
Java_com_mapswithme_maps_Framework_nativeFormatLatLonToArr(JNIEnv * env, jclass, jdouble lat, jdouble lon, jboolean useDMSFormat)
{
  string slat, slon;
  if (useDMSFormat)
    measurement_utils::FormatLatLonAsDMS(lat, lon, slat, slon, 2);
  else
    measurement_utils::FormatLatLon(lat, lon, slat, slon, 6);

  static jclass const klass = jni::GetGlobalClassRef(env, "java/lang/String");
  jobjectArray arr = env->NewObjectArray(2, klass, 0);

  env->SetObjectArrayElement(arr, 0, jni::ToJavaString(env, slat));
  env->SetObjectArrayElement(arr, 1, jni::ToJavaString(env, slon));

  return arr;
}

JNIEXPORT jobject JNICALL
Java_com_mapswithme_maps_Framework_nativeFormatAltitude(JNIEnv * env, jclass, jdouble alt)
{
  return jni::ToJavaString(env, measurement_utils::FormatAltitude(alt));
}

JNIEXPORT jobject JNICALL
Java_com_mapswithme_maps_Framework_nativeFormatSpeed(JNIEnv * env, jclass, jdouble speed)
{
  return jni::ToJavaString(env, measurement_utils::FormatSpeedWithDeviceUnits(speed));
}

JNIEXPORT jobject JNICALL
Java_com_mapswithme_maps_Framework_nativeGetOutdatedCountriesString(JNIEnv * env, jclass)
{
  return jni::ToJavaString(env, g_framework->GetOutdatedCountriesString());
}

JNIEXPORT jobjectArray JNICALL
Java_com_mapswithme_maps_Framework_nativeGetOutdatedCountries(JNIEnv * env, jclass)
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
Java_com_mapswithme_maps_Framework_nativeToDoAfterUpdate(JNIEnv * env, jclass)
{
  return g_framework->ToDoAfterUpdate();
}

JNIEXPORT jboolean JNICALL
Java_com_mapswithme_maps_Framework_nativeIsDataVersionChanged(JNIEnv * env, jclass)
{
  return frm()->IsDataVersionUpdated() ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT void JNICALL
Java_com_mapswithme_maps_Framework_nativeUpdateSavedDataVersion(JNIEnv * env, jclass)
{
  frm()->UpdateSavedDataVersion();
}

JNIEXPORT jlong JNICALL
Java_com_mapswithme_maps_Framework_nativeGetDataVersion(JNIEnv * env, jclass)
{
  return frm()->GetCurrentDataVersion();
}

JNIEXPORT jint JNICALL
Java_com_mapswithme_maps_Framework_nativeGetDrawScale(JNIEnv * env, jclass)
{
  return static_cast<jint>(frm()->GetDrawScale());
}

JNIEXPORT void JNICALL
Java_com_mapswithme_maps_Framework_nativePokeSearchInViewport(JNIEnv * env, jclass)
{
  frm()->PokeSearchInViewport();
}

JNIEXPORT jdoubleArray JNICALL
Java_com_mapswithme_maps_Framework_nativeGetScreenRectCenter(JNIEnv * env, jclass)
{
  m2::PointD const center = frm()->GetViewportCenter();

  double latlon[] = {MercatorBounds::YToLat(center.y), MercatorBounds::XToLon(center.x)};
  jdoubleArray jLatLon = env->NewDoubleArray(2);
  env->SetDoubleArrayRegion(jLatLon, 0, 2, latlon);

  return jLatLon;
}

JNIEXPORT void JNICALL
Java_com_mapswithme_maps_Framework_nativeShowTrackRect(JNIEnv * env, jclass, jlong track)
{
  frm()->ShowTrack(static_cast<kml::TrackId>(track));
}

JNIEXPORT jstring JNICALL
Java_com_mapswithme_maps_Framework_nativeGetBookmarkDir(JNIEnv * env, jclass)
{
  return jni::ToJavaString(env, GetPlatform().SettingsDir().c_str());
}

JNIEXPORT jstring JNICALL
Java_com_mapswithme_maps_Framework_nativeGetWritableDir(JNIEnv * env, jclass)
{
  return jni::ToJavaString(env, GetPlatform().WritableDir().c_str());
}

JNIEXPORT jstring JNICALL
Java_com_mapswithme_maps_Framework_nativeGetSettingsDir(JNIEnv * env, jclass)
{
  return jni::ToJavaString(env, GetPlatform().SettingsDir().c_str());
}

JNIEXPORT jobjectArray JNICALL
Java_com_mapswithme_maps_Framework_nativeGetMovableFilesExts(JNIEnv * env, jclass)
{
  vector<string> exts = { DATA_FILE_EXTENSION, FONT_FILE_EXTENSION, ROUTING_FILE_EXTENSION };
  platform::CountryIndexes::GetIndexesExts(exts);
  return jni::ToJavaStringArray(env, exts);
}

JNIEXPORT jstring JNICALL
Java_com_mapswithme_maps_Framework_nativeGetBookmarksExt(JNIEnv * env, jclass)
{
  return jni::ToJavaString(env, BOOKMARKS_FILE_EXTENSION);
}

JNIEXPORT void JNICALL
Java_com_mapswithme_maps_Framework_nativeSetWritableDir(JNIEnv * env, jclass, jstring jNewPath)
{
  string newPath = jni::ToNativeString(env, jNewPath);
  g_framework->RemoveLocalMaps();
  android::Platform::Instance().SetWritableDir(newPath);
  g_framework->AddLocalMaps();
}

JNIEXPORT jboolean JNICALL
Java_com_mapswithme_maps_Framework_nativeIsRoutingActive(JNIEnv * env, jclass)
{
  return frm()->GetRoutingManager().IsRoutingActive();
}

JNIEXPORT jboolean JNICALL
Java_com_mapswithme_maps_Framework_nativeIsRouteBuilding(JNIEnv * env, jclass)
{
  return frm()->GetRoutingManager().IsRouteBuilding();
}

JNIEXPORT jboolean JNICALL
Java_com_mapswithme_maps_Framework_nativeIsRouteBuilt(JNIEnv * env, jclass)
{
  return frm()->GetRoutingManager().IsRouteBuilt();
}

JNIEXPORT void JNICALL
Java_com_mapswithme_maps_Framework_nativeCloseRouting(JNIEnv * env, jclass)
{
  frm()->GetRoutingManager().CloseRouting(true /* remove route points */);
}

JNIEXPORT void JNICALL
Java_com_mapswithme_maps_Framework_nativeBuildRoute(JNIEnv * env, jclass)
{
  frm()->GetRoutingManager().BuildRoute(0 /* timeoutSec */);
}

JNIEXPORT void JNICALL
Java_com_mapswithme_maps_Framework_nativeRemoveRoute(JNIEnv * env, jclass)
{
  frm()->GetRoutingManager().RemoveRoute(false /* deactivateFollowing */);
}

JNIEXPORT void JNICALL
Java_com_mapswithme_maps_Framework_nativeFollowRoute(JNIEnv * env, jclass)
{
  frm()->GetRoutingManager().FollowRoute();
}

JNIEXPORT void JNICALL
Java_com_mapswithme_maps_Framework_nativeDisableFollowing(JNIEnv * env, jclass)
{
  frm()->GetRoutingManager().DisableFollowMode();
}

JNIEXPORT jstring JNICALL
Java_com_mapswithme_maps_Framework_nativeGetUserAgent(JNIEnv * env, jclass)
{
  return jni::ToJavaString(env, GetPlatform().GetAppUserAgent());
}

JNIEXPORT jobjectArray JNICALL
Java_com_mapswithme_maps_Framework_nativeGenerateNotifications(JNIEnv * env, jclass)
{
  ::Framework * fr = frm();
  if (!fr->GetRoutingManager().IsRoutingActive())
    return nullptr;

  vector<string> notifications;
  fr->GetRoutingManager().GenerateNotifications(notifications);
  if (notifications.empty())
    return nullptr;

  return jni::ToJavaStringArray(env, notifications);
}

JNIEXPORT void JNICALL
Java_com_mapswithme_maps_Framework_nativeSetSpeedCamManagerMode(JNIEnv * env, jclass, jint mode)
{
  frm()->GetRoutingManager().GetSpeedCamManager().SetMode(
    static_cast<routing::SpeedCameraManagerMode>(mode));
}

JNIEXPORT jint JNICALL
Java_com_mapswithme_maps_Framework_nativeGetSpeedCamManagerMode(JNIEnv * env, jclass)
{
  return static_cast<jint>(frm()->GetRoutingManager().GetSpeedCamManager().GetMode());
}

JNIEXPORT jobject JNICALL
Java_com_mapswithme_maps_Framework_nativeGetRouteFollowingInfo(JNIEnv * env, jclass)
{
  ::Framework * fr = frm();
  if (!fr->GetRoutingManager().IsRoutingActive())
    return nullptr;

  location::FollowingInfo info;
  fr->GetRoutingManager().GetRouteFollowingInfo(info);
  if (!info.IsValid())
    return nullptr;

  static jclass const klass = jni::GetGlobalClassRef(env, "com/mapswithme/maps/routing/RoutingInfo");
  // Java signature : RoutingInfo(String distToTarget, String units, String distTurn, String turnSuffix, String currentStreet, String nextStreet,
  //                              double completionPercent, int vehicleTurnOrdinal, int vehicleNextTurnOrdinal, int pedestrianTurnOrdinal,
  //                              double pedestrianDirectionLat, double pedestrianDirectionLon, int exitNum, int totalTime, SingleLaneInfo[] lanes)
  static jmethodID const ctorRouteInfoID = jni::GetConstructorID(env, klass,
                                               "(Ljava/lang/String;Ljava/lang/String;"
                                               "Ljava/lang/String;Ljava/lang/String;"
                                               "Ljava/lang/String;Ljava/lang/String;DIIIDDII"
                                               "[Lcom/mapswithme/maps/routing/SingleLaneInfo;ZZ)V");

  vector<location::FollowingInfo::SingleLaneInfoClient> const & lanes = info.m_lanes;
  jobjectArray jLanes = nullptr;
  if (!lanes.empty())
  {
    static jclass const laneClass = jni::GetGlobalClassRef(env, "com/mapswithme/maps/routing/SingleLaneInfo");
    size_t const lanesSize = lanes.size();
    jLanes = env->NewObjectArray(lanesSize, laneClass, nullptr);
    ASSERT(jLanes, (jni::DescribeException()));
    static jmethodID const ctorSingleLaneInfoID = jni::GetConstructorID(env, laneClass, "([BZ)V");

    for (size_t j = 0; j < lanesSize; ++j)
    {
      size_t const laneSize = lanes[j].m_lane.size();
      jni::TScopedLocalByteArrayRef singleLane(env, env->NewByteArray(laneSize));
      ASSERT(singleLane.get(), (jni::DescribeException()));
      env->SetByteArrayRegion(singleLane.get(), 0, laneSize, lanes[j].m_lane.data());

      jni::TScopedLocalRef singleLaneInfo(env, env->NewObject(laneClass, ctorSingleLaneInfoID, singleLane.get(), lanes[j].m_isRecommended));
      ASSERT(singleLaneInfo.get(), (jni::DescribeException()));
      env->SetObjectArrayElement(jLanes, j, singleLaneInfo.get());
    }
  }

  auto const & rm = frm()->GetRoutingManager();
  auto const isSpeedLimitExceeded = rm.IsRoutingActive() ? rm.IsSpeedLimitExceeded() : false;
  auto const shouldPlaySignal = frm()->GetRoutingManager().GetSpeedCamManager().ShouldPlayBeepSignal();
  jobject const result = env->NewObject(
      klass, ctorRouteInfoID, jni::ToJavaString(env, info.m_distToTarget),
      jni::ToJavaString(env, info.m_targetUnitsSuffix), jni::ToJavaString(env, info.m_distToTurn),
      jni::ToJavaString(env, info.m_turnUnitsSuffix), jni::ToJavaString(env, info.m_sourceName),
      jni::ToJavaString(env, info.m_displayedStreetName), info.m_completionPercent, info.m_turn, info.m_nextTurn, info.m_pedestrianTurn,
      info.m_pedestrianDirectionPos.lat, info.m_pedestrianDirectionPos.lon, info.m_exitNum, info.m_time, jLanes,
      static_cast<jboolean>(isSpeedLimitExceeded), static_cast<jboolean>(shouldPlaySignal));
  ASSERT(result, (jni::DescribeException()));
  return result;
}

JNIEXPORT void JNICALL
Java_com_mapswithme_maps_Framework_nativeDisableAdProvider(JNIEnv * env, jclass, jint type,
                                                           jint place)
{
  auto const & bannerType = static_cast<ads::Banner::Type>(type);
  auto const & bannerPlace = static_cast<ads::Banner::Place>(place);
  g_framework->DisableAdProvider(bannerType, bannerPlace);
}

JNIEXPORT jintArray JNICALL
Java_com_mapswithme_maps_Framework_nativeGenerateRouteAltitudeChartBits(JNIEnv * env, jclass, jint width, jint height, jobject routeAltitudeLimits)
{
  ::Framework * fr = frm();
  ASSERT(fr, ());

  feature::TAltitudes altitudes;
  vector<double> routePointDistanceM;
  if (!fr->GetRoutingManager().GetRouteAltitudesAndDistancesM(routePointDistanceM, altitudes))
  {
    LOG(LWARNING, ("Can't get distance to route points and altitude."));
    return nullptr;
  }

  vector<uint8_t> imageRGBAData;
  int32_t minRouteAltitude = 0;
  int32_t maxRouteAltitude = 0;
  measurement_utils::Units units = measurement_utils::Units::Metric;
  if (!fr->GetRoutingManager().GenerateRouteAltitudeChart(
        width, height, altitudes, routePointDistanceM, imageRGBAData,
        minRouteAltitude, maxRouteAltitude, units))
  {
    LOG(LWARNING, ("Can't generate route altitude image."));
    return nullptr;
  }

  // Passing route limits.
  jclass const routeAltitudeLimitsClass = env->GetObjectClass(routeAltitudeLimits);
  ASSERT(routeAltitudeLimitsClass, ());

  static jfieldID const minRouteAltitudeField = env->GetFieldID(routeAltitudeLimitsClass, "minRouteAltitude", "I");
  ASSERT(minRouteAltitudeField, ());
  env->SetIntField(routeAltitudeLimits, minRouteAltitudeField, minRouteAltitude);

  static jfieldID const maxRouteAltitudeField = env->GetFieldID(routeAltitudeLimitsClass, "maxRouteAltitude", "I");
  ASSERT(maxRouteAltitudeField, ());
  env->SetIntField(routeAltitudeLimits, maxRouteAltitudeField, maxRouteAltitude);

  static jfieldID const isMetricUnitsField = env->GetFieldID(routeAltitudeLimitsClass, "isMetricUnits", "Z");
  ASSERT(isMetricUnitsField, ());
  env->SetBooleanField(routeAltitudeLimits, isMetricUnitsField, units == measurement_utils::Units::Metric);

  size_t const imageRGBADataSize = imageRGBAData.size();
  ASSERT_NOT_EQUAL(imageRGBADataSize, 0, ("GenerateRouteAltitudeChart returns true but the vector with altitude image bits is empty."));

  size_t const pxlCount = width * height;
  if (maps::kAltitudeChartBPP * pxlCount != imageRGBADataSize)
  {
    LOG(LWARNING, ("Wrong size of vector with altitude image bits. Expected size:", pxlCount, ". Real size:", imageRGBADataSize));
    return nullptr;
  }

  jintArray imageRGBADataArray = env->NewIntArray(pxlCount);
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
Java_com_mapswithme_maps_Framework_nativeShowCountry(JNIEnv * env, jclass, jstring countryId, jboolean zoomToDownloadButton)
{
  g_framework->ShowNode(jni::ToNativeString(env, countryId), (bool) zoomToDownloadButton);
}

JNIEXPORT void JNICALL
Java_com_mapswithme_maps_Framework_nativeSetRoutingListener(JNIEnv * env, jclass, jobject listener)
{
  CHECK(g_framework, ("Framework isn't created yet!"));
  auto rf = jni::make_global_ref(listener);
  frm()->GetRoutingManager().SetRouteBuildingListener(
      [rf](routing::RouterResultCode e, storage::TCountriesVec const & v) {
        CallRoutingListener(rf, static_cast<int>(e), v);
      });
}

JNIEXPORT void JNICALL
Java_com_mapswithme_maps_Framework_nativeSetRouteProgressListener(JNIEnv * env, jclass, jobject listener)
{
  CHECK(g_framework, ("Framework isn't created yet!"));
  frm()->GetRoutingManager().SetRouteProgressListener(
      bind(&CallRouteProgressListener, jni::make_global_ref(listener), _1));
}

JNIEXPORT void JNICALL
Java_com_mapswithme_maps_Framework_nativeSetRoutingRecommendationListener(JNIEnv * env, jclass,
                                                                          jobject listener)
{
  CHECK(g_framework, ("Framework isn't created yet!"));
  frm()->GetRoutingManager().SetRouteRecommendationListener(
      bind(&CallRouteRecommendationListener, jni::make_global_ref(listener), _1));
}

JNIEXPORT void JNICALL
Java_com_mapswithme_maps_Framework_nativeSetRoutingLoadPointsListener(
        JNIEnv *, jclass, jobject listener)
{
  CHECK(g_framework, ("Framework isn't created yet!"));
  if (listener != nullptr)
    g_loadRouteHandler = bind(&CallSetRoutingLoadPointsListener, jni::make_global_ref(listener), _1);
  else
    g_loadRouteHandler = nullptr;
}

JNIEXPORT void JNICALL
Java_com_mapswithme_maps_Framework_nativeDeactivatePopup(JNIEnv * env, jclass)
{
  return g_framework->DeactivatePopup();
}

JNIEXPORT void JNICALL
Java_com_mapswithme_maps_Framework_nativeSetMapStyle(JNIEnv * env, jclass, jint mapStyle)
{
  MapStyle const val = static_cast<MapStyle>(mapStyle);
  if (val != g_framework->GetMapStyle())
    g_framework->SetMapStyle(val);
}

JNIEXPORT jint JNICALL
Java_com_mapswithme_maps_Framework_nativeGetMapStyle(JNIEnv * env, jclass, jint mapStyle)
{
  return g_framework->GetMapStyle();
}

JNIEXPORT void JNICALL
Java_com_mapswithme_maps_Framework_nativeMarkMapStyle(JNIEnv * env, jclass, jint mapStyle)
{
  MapStyle const val = static_cast<MapStyle>(mapStyle);
  if (val != g_framework->GetMapStyle())
    g_framework->MarkMapStyle(val);
}

JNIEXPORT void JNICALL
Java_com_mapswithme_maps_Framework_nativeSetRouter(JNIEnv * env, jclass, jint routerType)
{
  g_framework->GetRoutingManager().SetRouter(static_cast<routing::RouterType>(routerType));
}

JNIEXPORT jint JNICALL
Java_com_mapswithme_maps_Framework_nativeGetRouter(JNIEnv * env, jclass)
{
  return static_cast<jint>(g_framework->GetRoutingManager().GetRouter());
}

JNIEXPORT jint JNICALL
Java_com_mapswithme_maps_Framework_nativeGetLastUsedRouter(JNIEnv * env, jclass)
{
  return static_cast<jint>(g_framework->GetRoutingManager().GetLastUsedRouter());
}

JNIEXPORT jint JNICALL
Java_com_mapswithme_maps_Framework_nativeGetBestRouter(JNIEnv * env, jclass,
                                                       jdouble srcLat, jdouble srcLon,
                                                       jdouble dstLat, jdouble dstLon)
{
  return static_cast<jint>(frm()->GetRoutingManager().GetBestRouter(
      MercatorBounds::FromLatLon(srcLat, srcLon), MercatorBounds::FromLatLon(dstLat, dstLon)));
}

JNIEXPORT void JNICALL
Java_com_mapswithme_maps_Framework_nativeAddRoutePoint(JNIEnv * env, jclass, jstring title,
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
  data.m_position = m2::PointD(MercatorBounds::FromLatLon(lat, lon));

  frm()->GetRoutingManager().AddRoutePoint(std::move(data));
}

JNIEXPORT void JNICALL
Java_com_mapswithme_maps_Framework_nativeRemoveRoutePoint(JNIEnv * env, jclass,
                                                          jint markType, jint intermediateIndex)
{
  frm()->GetRoutingManager().RemoveRoutePoint(static_cast<RouteMarkType>(markType),
                                              static_cast<size_t>(intermediateIndex));
}

JNIEXPORT void JNICALL
Java_com_mapswithme_maps_Framework_nativeRemoveIntermediateRoutePoints(JNIEnv * env, jclass)
{
  frm()->GetRoutingManager().RemoveIntermediateRoutePoints();
}

JNIEXPORT jboolean JNICALL
Java_com_mapswithme_maps_Framework_nativeCouldAddIntermediatePoint(JNIEnv * env, jclass)
{
  return frm()->GetRoutingManager().CouldAddIntermediatePoint();
}

JNIEXPORT jobjectArray JNICALL
Java_com_mapswithme_maps_Framework_nativeGetRoutePoints(JNIEnv * env, jclass)
{
  auto const points = frm()->GetRoutingManager().GetRoutePoints();

  static jclass const pointClazz = jni::GetGlobalClassRef(env,
                                   "com/mapswithme/maps/routing/RouteMarkData");
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
                          MercatorBounds::YToLat(data.m_position.y),
                          MercatorBounds::XToLon(data.m_position.x));
  });
}

JNIEXPORT jobject JNICALL
Java_com_mapswithme_maps_Framework_nativeGetTransitRouteInfo(JNIEnv * env, jclass)
{
  auto const routeInfo = frm()->GetRoutingManager().GetTransitRouteInfo();

  static jclass const transitStepClass = jni::GetGlobalClassRef(env,
                                         "com/mapswithme/maps/routing/TransitStepInfo");
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
                                                                     "com/mapswithme/maps/routing/TransitRouteInfo");
  // Java signature : TransitRouteInfo(@NonNull String totalDistance, @NonNull String totalDistanceUnits, int totalTimeInSec,
  //                                   @NonNull String totalPedestrianDistance, @NonNull String totalPedestrianDistanceUnits,
  //                                   int totalPedestrianTimeInSec, @NonNull TransitStepInfo[] steps)
  static jmethodID const transitRouteInfoConstructor = jni::GetConstructorID(env, transitRouteInfoClass,
                                                                             "(Ljava/lang/String;Ljava/lang/String;I"
                                                                             "Ljava/lang/String;Ljava/lang/String;I"
                                                                             "[Lcom/mapswithme/maps/routing/TransitStepInfo;)V");
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
Java_com_mapswithme_maps_Framework_nativeRegisterMaps(JNIEnv * env, jclass)
{
  frm()->RegisterAllMaps();
}

JNIEXPORT void JNICALL
Java_com_mapswithme_maps_Framework_nativeDeregisterMaps(JNIEnv * env, jclass)
{
  frm()->DeregisterAllMaps();
}

JNIEXPORT jboolean JNICALL
Java_com_mapswithme_maps_Framework_nativeIsDayTime(JNIEnv * env, jclass, jlong utcTimeSeconds, jdouble lat, jdouble lon)
{
  DayTimeType const dt = GetDayTime(static_cast<time_t>(utcTimeSeconds), lat, lon);
  return (dt == DayTimeType::Day || dt == DayTimeType::PolarDay);
}

JNIEXPORT void JNICALL
Java_com_mapswithme_maps_Framework_nativeSet3dMode(JNIEnv * env, jclass, jboolean allow, jboolean allowBuildings)
{
  bool const allow3d = static_cast<bool>(allow);
  bool const allow3dBuildings = static_cast<bool>(allowBuildings);

  g_framework->Save3dMode(allow3d, allow3dBuildings);
  g_framework->Set3dMode(allow3d, allow3dBuildings);
}

JNIEXPORT void JNICALL
Java_com_mapswithme_maps_Framework_nativeGet3dMode(JNIEnv * env, jclass, jobject result)
{
  bool enabled;
  bool buildings;
  g_framework->Get3dMode(enabled, buildings);

  jclass const resultClass = env->GetObjectClass(result);

  static jfieldID const enabledField = env->GetFieldID(resultClass, "enabled", "Z");
  env->SetBooleanField(result, enabledField, enabled);

  static jfieldID const buildingsField = env->GetFieldID(resultClass, "buildings", "Z");
  env->SetBooleanField(result, buildingsField, buildings);
}

JNIEXPORT void JNICALL
Java_com_mapswithme_maps_Framework_nativeSetAutoZoomEnabled(JNIEnv * env, jclass, jboolean enabled)
{
  bool const autoZoomEnabled = static_cast<bool>(enabled);
  frm()->SaveAutoZoom(autoZoomEnabled);
  frm()->AllowAutoZoom(autoZoomEnabled);
}

JNIEXPORT void JNICALL
Java_com_mapswithme_maps_Framework_nativeSetTransitSchemeEnabled(JNIEnv * env, jclass, jboolean enabled)
{
  frm()->GetTransitManager().EnableTransitSchemeMode(static_cast<bool>(enabled));
}

JNIEXPORT jboolean JNICALL
Java_com_mapswithme_maps_Framework_nativeIsTransitSchemeEnabled(JNIEnv * env, jclass)
{
  return static_cast<jboolean>(frm()->LoadTransitSchemeEnabled());
}

JNIEXPORT void JNICALL
Java_com_mapswithme_maps_Framework_nativeSaveSettingSchemeEnabled(JNIEnv * env, jclass, jboolean enabled)
{
  frm()->SaveTransitSchemeEnabled(static_cast<bool>(enabled));
}

JNIEXPORT jboolean JNICALL
Java_com_mapswithme_maps_Framework_nativeGetAutoZoomEnabled(JNIEnv *, jclass)
{
  return frm()->LoadAutoZoom();
}

// static void nativeZoomToPoint(double lat, double lon, int zoom, boolean animate);
JNIEXPORT void JNICALL
Java_com_mapswithme_maps_Framework_nativeZoomToPoint(JNIEnv * env, jclass, jdouble lat, jdouble lon, jint zoom, jboolean animate)
{
  g_framework->Scale(m2::PointD(MercatorBounds::FromLatLon(lat, lon)), zoom, animate);
}

JNIEXPORT jobject JNICALL
Java_com_mapswithme_maps_Framework_nativeDeleteBookmarkFromMapObject(JNIEnv * env, jclass)
{
  place_page::Info & info = g_framework->GetPlacePageInfo();
  auto const bookmarkId = info.GetBookmarkId();
  frm()->ResetBookmarkInfo(*frm()->GetBookmarkManager().GetBookmark(bookmarkId), info);
  frm()->GetBookmarkManager().GetEditSession().DeleteBookmark(bookmarkId);
  return usermark_helper::CreateMapObject(env, info);
}

JNIEXPORT void JNICALL
Java_com_mapswithme_maps_Framework_nativeOnBookmarkCategoryChanged(JNIEnv * env, jclass, jlong cat, jlong bmk)
{
  place_page::Info & info = g_framework->GetPlacePageInfo();
  ASSERT_GREATER_OR_EQUAL(bmk, 0, ());
  ASSERT_GREATER_OR_EQUAL(cat, 0, ());
  info.SetBookmarkCategoryId(static_cast<kml::MarkGroupId>(cat));
  info.SetBookmarkId(static_cast<kml::MarkId>(bmk));
  info.SetBookmarkCategoryName(frm()->GetBookmarkManager().GetCategoryName(static_cast<kml::MarkGroupId>(cat)));
}

JNIEXPORT void JNICALL
Java_com_mapswithme_maps_Framework_nativeTurnOnChoosePositionMode(JNIEnv *, jclass, jboolean isBusiness, jboolean applyPosition)
{
  g_framework->SetChoosePositionMode(true, isBusiness, applyPosition, applyPosition ? g_framework->GetPlacePageInfo().GetMercator() : m2::PointD());
}

JNIEXPORT void JNICALL
Java_com_mapswithme_maps_Framework_nativeTurnOffChoosePositionMode(JNIEnv *, jclass)
{
  g_framework->SetChoosePositionMode(false, false, false, m2::PointD());
}

JNIEXPORT jboolean JNICALL
Java_com_mapswithme_maps_Framework_nativeIsInChoosePositionMode(JNIEnv *, jclass)
{
  return g_framework->GetChoosePositionMode();
}

JNIEXPORT jboolean JNICALL
Java_com_mapswithme_maps_Framework_nativeIsDownloadedMapAtScreenCenter(JNIEnv *, jclass)
{
  ::Framework * fr = frm();
  return storage::IsPointCoveredByDownloadedMaps(fr->GetViewportCenter(), fr->GetStorage(), fr->GetCountryInfoGetter());
}

JNIEXPORT jstring JNICALL
Java_com_mapswithme_maps_Framework_nativeGetActiveObjectFormattedCuisine(JNIEnv * env, jclass)
{
  return jni::ToJavaString(env, g_framework->GetPlacePageInfo().FormatCuisines());
}

JNIEXPORT void JNICALL
Java_com_mapswithme_maps_Framework_nativeSetVisibleRect(JNIEnv * env, jclass, jint left, jint top, jint right, jint bottom)
{
  frm()->SetVisibleViewport(m2::RectD(left, top, right, bottom));
}

JNIEXPORT jboolean JNICALL
Java_com_mapswithme_maps_Framework_nativeIsRouteFinished(JNIEnv * env, jclass)
{
  return frm()->GetRoutingManager().IsRouteFinished();
}

JNIEXPORT void JNICALL
Java_com_mapswithme_maps_Framework_nativeLogLocalAdsEvent(JNIEnv * env, jclass, jint type,
                                                          jdouble lat, jdouble lon, jint accuracy)
{
  g_framework->LogLocalAdsEvent(static_cast<local_ads::EventType>(type), lat, lon, accuracy);
}

JNIEXPORT void JNICALL
Java_com_mapswithme_maps_Framework_nativeRunFirstLaunchAnimation(JNIEnv * env, jclass)
{
  frm()->RunFirstLaunchAnimation();
}

JNIEXPORT jint JNICALL
Java_com_mapswithme_maps_Framework_nativeOpenRoutePointsTransaction(JNIEnv * env, jclass)
{
  return frm()->GetRoutingManager().OpenRoutePointsTransaction();
}

JNIEXPORT void JNICALL
Java_com_mapswithme_maps_Framework_nativeApplyRoutePointsTransaction(JNIEnv * env, jclass,
                                                                     jint transactionId)
{
  frm()->GetRoutingManager().ApplyRoutePointsTransaction(transactionId);
}

JNIEXPORT void JNICALL
Java_com_mapswithme_maps_Framework_nativeCancelRoutePointsTransaction(JNIEnv * env, jclass,
                                                                      jint transactionId)
{
  frm()->GetRoutingManager().CancelRoutePointsTransaction(transactionId);
}

JNIEXPORT jint JNICALL
Java_com_mapswithme_maps_Framework_nativeInvalidRoutePointsTransactionId(JNIEnv * env, jclass)
{
  return frm()->GetRoutingManager().InvalidRoutePointsTransactionId();
}

JNIEXPORT jboolean JNICALL
Java_com_mapswithme_maps_Framework_nativeHasSavedRoutePoints()
{
  return frm()->GetRoutingManager().HasSavedRoutePoints();
}

JNIEXPORT void JNICALL
Java_com_mapswithme_maps_Framework_nativeLoadRoutePoints()
{
  frm()->GetRoutingManager().LoadRoutePoints(g_loadRouteHandler);
}

JNIEXPORT void JNICALL
Java_com_mapswithme_maps_Framework_nativeSaveRoutePoints()
{
  frm()->GetRoutingManager().SaveRoutePoints();
}

JNIEXPORT void JNICALL
Java_com_mapswithme_maps_Framework_nativeDeleteSavedRoutePoints()
{
  frm()->GetRoutingManager().DeleteSavedRoutePoints();
}

JNIEXPORT jobjectArray JNICALL
Java_com_mapswithme_maps_Framework_nativeGetSearchBanners(JNIEnv * env, jclass)
{
  auto const & purchase = frm()->GetPurchase();
  if (purchase && purchase->IsSubscriptionActive(SubscriptionType::RemoveAds))
    return nullptr;
  return usermark_helper::ToBannersArray(env, frm()->GetAdsEngine().GetSearchBanners());
}

JNIEXPORT void JNICALL
Java_com_mapswithme_maps_Framework_nativeAuthenticateUser(JNIEnv * env, jclass, jstring socialToken,
                                                          jint socialTokenType,
                                                          jboolean privacyAccepted,
                                                          jboolean termsAccepted,
                                                          jboolean promoAccepted,
                                                          jobject listener)
{
  std::shared_ptr<_jobject> gListener(env->NewGlobalRef(listener), [](jobject l)
  {
    jni::GetEnv()->DeleteGlobalRef(l);
  });

  auto const tokenStr = jni::ToNativeString(env, socialToken);
  auto & user = frm()->GetUser();
  auto s = make_unique<User::Subscriber>();
  s->m_postCallAction = User::Subscriber::Action::RemoveSubscriber;
  s->m_onAuthenticate = [gListener](bool success)
  {
    GetPlatform().RunTask(Platform::Thread::Gui, [gListener, success]
    {
      auto e = jni::GetEnv();
      jmethodID const callback = jni::GetMethodID(e, gListener.get(), "onAuthorized", "(Z)V");
      e->CallVoidMethod(gListener.get(), callback, success);
    });
  };
  user.AddSubscriber(std::move(s));
  user.Authenticate(tokenStr, static_cast<User::SocialTokenType>(socialTokenType),
                    static_cast<bool>(privacyAccepted), static_cast<bool>(termsAccepted),
                    static_cast<bool>(promoAccepted));
}

JNIEXPORT jboolean JNICALL
Java_com_mapswithme_maps_Framework_nativeIsUserAuthenticated()
{
  return frm()->GetUser().IsAuthenticated();
}

JNIEXPORT jstring JNICALL
Java_com_mapswithme_maps_Framework_nativeGetPhoneAuthUrl(JNIEnv * env, jclass, jstring redirectUrl)
{
  return jni::ToJavaString(env, User::GetPhoneAuthUrl(jni::ToNativeString(env, redirectUrl)));
}

JNIEXPORT jstring JNICALL
Java_com_mapswithme_maps_Framework_nativeGetPrivacyPolicyLink(JNIEnv * env, jclass)
{
  return jni::ToJavaString(env, User::GetPrivacyPolicyLink());
}

JNIEXPORT jstring JNICALL
Java_com_mapswithme_maps_Framework_nativeGetTermsOfUseLink(JNIEnv * env, jclass)
{
  return jni::ToJavaString(env, User::GetTermsOfUseLink());
}

JNIEXPORT void JNICALL
Java_com_mapswithme_maps_Framework_nativeShowFeatureByLatLon(JNIEnv * env, jclass,
                                                             jdouble lat, jdouble lon)
{
  frm()->ShowFeatureByMercator(MercatorBounds::FromLatLon(ms::LatLon(lat, lon)));
}

JNIEXPORT void JNICALL
Java_com_mapswithme_maps_Framework_nativeShowBookmarkCategory(JNIEnv * env, jclass, jlong cat)
{
  frm()->ShowBookmarkCategory(static_cast<kml::MarkGroupId>(cat));
}

JNIEXPORT jint JNICALL
Java_com_mapswithme_maps_Framework_nativeGetFilterRating(JNIEnv * env, jclass, jfloat rawRating)
{
  return static_cast<jint>(place_page::rating::GetFilterRating(rawRating));
}

JNIEXPORT jstring JNICALL
Java_com_mapswithme_maps_Framework_nativeMoPubInitializationBannerId(JNIEnv * env, jclass)
{
  return jni::ToJavaString(env, ads::Mopub::InitializationBannerId());
}

JNIEXPORT jboolean JNICALL
Java_com_mapswithme_maps_Framework_nativeHasMegafonDownloaderBanner(JNIEnv * env, jclass,
                                                                    jstring mwmId)
{
  auto const & purchase = frm()->GetPurchase();
  if (purchase && purchase->IsSubscriptionActive(SubscriptionType::RemoveAds))
    return static_cast<jboolean>(false);
  return static_cast<jboolean>(ads::HasMegafonDownloaderBanner(frm()->GetStorage(),
                                                               jni::ToNativeString(env, mwmId),
                                                               languages::GetCurrentNorm()));
}

JNIEXPORT jboolean JNICALL
Java_com_mapswithme_maps_Framework_nativeHasRuTaxiCategoryBanner(JNIEnv * env, jclass)
{
  return static_cast<jboolean>(frm()->HasRuTaxiCategoryBanner());
}

JNIEXPORT jstring JNICALL
Java_com_mapswithme_maps_Framework_nativeGetMegafonDownloaderBannerUrl(JNIEnv * env, jclass)
{
  return jni::ToJavaString(env, ads::GetMegafonDownloaderBannerUrl());
}

JNIEXPORT void JNICALL
Java_com_mapswithme_maps_Framework_nativeMakeCrash(JNIEnv *env, jclass type)
{
  CHECK(false, ("Diagnostic native crash!"));
}

JNIEXPORT void JNICALL
Java_com_mapswithme_maps_Framework_nativeValidatePurchase(JNIEnv * env, jclass, jstring serverId,
                                                          jstring vendorId, jstring purchaseData)
{
  auto const & purchase = frm()->GetPurchase();
  if (purchase == nullptr)
    return;

  Purchase::ValidationInfo info;
  info.m_serverId = jni::ToNativeString(env, serverId);
  info.m_vendorId = jni::ToNativeString(env, vendorId);
  info.m_receiptData = jni::ToNativeString(env, purchaseData);

  purchase->Validate(info, frm()->GetUser().GetAccessToken());
}

JNIEXPORT void JNICALL
Java_com_mapswithme_maps_Framework_nativeStartPurchaseTransaction(JNIEnv * env, jclass,
                                                                  jstring serverId,
                                                                  jstring vendorId)
{
  auto const & purchase = frm()->GetPurchase();
  if (purchase == nullptr)
    return;

  purchase->StartTransaction(jni::ToNativeString(env, serverId),
                             jni::ToNativeString(env, vendorId),
                             frm()->GetUser().GetAccessToken());
}

JNIEXPORT void JNICALL
Java_com_mapswithme_maps_Framework_nativeSetPurchaseValidationListener(JNIEnv *, jclass,
                                                                       jobject listener)
{
  auto const & purchase = frm()->GetPurchase();
  if (purchase == nullptr)
    return;

  if (listener != nullptr)
  {
    purchase->SetValidationCallback(bind(&CallPurchaseValidationListener,
                                         jni::make_global_ref(listener), _1, _2));
  }
  else
  {
    purchase->SetValidationCallback(nullptr);
  }
}

JNIEXPORT void JNICALL
Java_com_mapswithme_maps_Framework_nativeStartPurchaseTransactionListener(JNIEnv *, jclass,
                                                                          jobject listener)
{
  auto const & purchase = frm()->GetPurchase();
  if (purchase == nullptr)
    return;

  if (listener != nullptr)
  {
    purchase->SetStartTransactionCallback(bind(&CallStartPurchaseTransactionListener,
                                          jni::make_global_ref(listener), _1, _2, _3));
  }
  else
  {
    purchase->SetStartTransactionCallback(nullptr);
  }
}

JNIEXPORT jboolean JNICALL
Java_com_mapswithme_maps_Framework_nativeHasActiveRemoveAdsSubscription(JNIEnv *, jclass)
{
  auto const & purchase = frm()->GetPurchase();
  return purchase != nullptr ?
         static_cast<jboolean>(purchase->IsSubscriptionActive(SubscriptionType::RemoveAds)) :
         static_cast<jboolean>(false);
}

JNIEXPORT void JNICALL
Java_com_mapswithme_maps_Framework_nativeSetActiveRemoveAdsSubscription(JNIEnv *, jclass,
                                                                        jboolean isActive)
{
  auto const & purchase = frm()->GetPurchase();
  if (purchase == nullptr)
    return;
  purchase->SetSubscriptionEnabled(SubscriptionType::RemoveAds, static_cast<bool>(isActive));
}

JNIEXPORT jint JNICALL
Java_com_mapswithme_maps_Framework_nativeGetCurrentTipsApi(JNIEnv * env, jclass)
{
  auto const & tipsApi = frm()->GetTipsApi();
  auto const tip = tipsApi.GetTip();
  return tip.is_initialized() ? static_cast<jint>(tip.get()) : kUndefinedTip;
}

JNIEXPORT void JNICALL
Java_com_mapswithme_maps_Framework_nativeBindUser(JNIEnv * env, jclass, jobject listener)
{
  auto listenerRef = jni::make_global_ref(listener);
  auto & user = frm()->GetUser();
  user.BindUser([listenerRef](bool success)
  {
    auto e = jni::GetEnv();
    jmethodID const callback = jni::GetMethodID(e, *listenerRef, "onUserBound", "(Z)V");
    e->CallVoidMethod(*listenerRef, callback, static_cast<jboolean>(success));
  });
}

JNIEXPORT jstring JNICALL
Java_com_mapswithme_maps_Framework_nativeGetAccessToken(JNIEnv * env, jclass)
{
  auto & user = frm()->GetUser();
  return jni::ToJavaString(env, user.GetAccessToken());
}

JNIEXPORT jobject JNICALL
Java_com_mapswithme_maps_Framework_nativeGetMapObject(JNIEnv * env, jclass,
                                                      jobject notificationMapObject)
{
  eye::MapObject mapObject;
  auto const getBestTypeId =
      jni::GetMethodID(env, notificationMapObject, "getBestType", "()Ljava/lang/String;");
  auto const bestType =
      static_cast<jstring>(env->CallObjectMethod(notificationMapObject, getBestTypeId));
  mapObject.SetBestType(jni::ToNativeString(env, bestType));

  auto const getMercatorPosXId =
      jni::GetMethodID(env, notificationMapObject, "getMercatorPosX", "()D");
  auto const getMercatorPosYId =
      jni::GetMethodID(env, notificationMapObject, "getMercatorPosY", "()D");

  auto const posX =
      static_cast<double>(env->CallDoubleMethod(notificationMapObject, getMercatorPosXId));
  auto const posY =
      static_cast<double>(env->CallDoubleMethod(notificationMapObject, getMercatorPosYId));
  mapObject.SetPos({posX, posY});

  auto const getDefaultNameId =
      jni::GetMethodID(env, notificationMapObject, "getDefaultName", "()Ljava/lang/String;");
  auto const defaultName =
      static_cast<jstring>(env->CallObjectMethod(notificationMapObject, getDefaultNameId));
  mapObject.SetDefaultName(jni::ToNativeString(env, defaultName));

  place_page::Info info;
  if (frm()->MakePlacePageInfo(mapObject, info))
    return usermark_helper::CreateMapObject(env, info);

  return nullptr;
}

JNIEXPORT void JNICALL
Java_com_mapswithme_maps_Framework_nativeOnBatteryLevelChanged(JNIEnv *, jclass, jint level)
{
  CHECK_GREATER_OR_EQUAL(level, 0, ());
  CHECK_LESS_OR_EQUAL(level, 100, ());

  frm()->GetPowerManager().OnBatteryLevelChanged(static_cast<uint8_t>(level));
}

JNIEXPORT void JNICALL
Java_com_mapswithme_maps_Framework_nativeSetPowerManagerFacility(JNIEnv *, jclass,
                                                                 jint facilityType, jboolean state)
{
  frm()->GetPowerManager().SetFacility(static_cast<PowerManager::Facility>(facilityType),
                                       static_cast<bool>(state));
}

JNIEXPORT void JNICALL
Java_com_mapswithme_maps_Framework_nativeSetPowerManagerScheme(JNIEnv *, jclass, jint schemeType)
{
  frm()->GetPowerManager().SetScheme(static_cast<PowerManager::Scheme>(schemeType));
}
}  // extern "C"
