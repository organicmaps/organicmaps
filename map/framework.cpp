#include "map/framework.hpp"
#include "map/benchmark_tools.hpp"
#include "map/chart_generator.hpp"
#include "map/displayed_categories_modifiers.hpp"
#include "map/everywhere_search_params.hpp"
#include "map/ge0_parser.hpp"
#include "map/geourl_process.hpp"
#include "map/gps_tracker.hpp"
#include "map/taxi_delegate.hpp"
#include "map/user_mark.hpp"
#include "map/utils.hpp"
#include "map/viewport_search_params.hpp"

#include "defines.hpp"
#include "private.h"

#include "routing/city_roads.hpp"
#include "routing/index_router.hpp"
#include "routing/online_absent_fetcher.hpp"
#include "routing/route.hpp"
#include "routing/routing_helpers.hpp"

#include "routing_common/num_mwm_id.hpp"

#include "search/cities_boundaries_table.hpp"
#include "search/downloader_search_callback.hpp"
#include "search/editor_delegate.hpp"
#include "search/engine.hpp"
#include "search/geometry_utils.hpp"
#include "search/intermediate_result.hpp"
#include "search/locality_finder.hpp"
#include "search/reverse_geocoder.hpp"

#include "storage/country_info_getter.hpp"
#include "storage/downloader_search_params.hpp"
#include "storage/routing_helpers.hpp"
#include "storage/storage_helpers.hpp"

#include "drape_frontend/color_constants.hpp"
#include "drape_frontend/gps_track_point.hpp"
#include "drape_frontend/route_renderer.hpp"
#include "drape_frontend/visual_params.hpp"

#include "drape/constants.hpp"

#include "editor/editable_data_source.hpp"

#include "descriptions/loader.hpp"

#include "indexer/categories_holder.hpp"
#include "indexer/classificator.hpp"
#include "indexer/classificator_loader.hpp"
#include "indexer/drawing_rules.hpp"
#include "indexer/editable_map_object.hpp"
#include "indexer/feature.hpp"
#include "indexer/feature_algo.hpp"
#include "indexer/feature_source.hpp"
#include "indexer/feature_utils.hpp"
#include "indexer/feature_visibility.hpp"
#include "indexer/ftypes_sponsored.hpp"
#include "indexer/map_style_reader.hpp"
#include "indexer/scales.hpp"

#include "metrics/eye.hpp"

#include "platform/local_country_file_utils.hpp"
#include "platform/measurement_utils.hpp"
#include "platform/mwm_traits.hpp"
#include "platform/mwm_version.hpp"
#include "platform/network_policy.hpp"
#include "platform/platform.hpp"
#include "platform/preferred_languages.hpp"
#include "platform/settings.hpp"
#include "platform/socket.hpp"

#include "coding/endianness.hpp"
#include "coding/file_name_utils.hpp"
#include "coding/point_coding.hpp"
#include "coding/string_utf8_multilang.hpp"
#include "coding/transliteration.hpp"
#include "coding/url_encode.hpp"
#include "coding/zip_reader.hpp"

#include "geometry/angles.hpp"
#include "geometry/any_rect2d.hpp"
#include "geometry/distance_on_sphere.hpp"
#include "geometry/latlon.hpp"
#include "geometry/mercator.hpp"
#include "geometry/rect2d.hpp"
#include "geometry/tree4d.hpp"
#include "geometry/triangle2d.hpp"

#include "partners_api/ads_engine.hpp"
#include "partners_api/opentable_api.hpp"
#include "partners_api/partners.hpp"

#include "base/logging.hpp"
#include "base/math.hpp"
#include "base/scope_guard.hpp"
#include "base/stl_helpers.hpp"
#include "base/timer.hpp"

#include "std/algorithm.hpp"
#include "std/bind.hpp"
#include "std/map.hpp"
#include "std/target_os.hpp"

#include "api/internal/c/api-client-internals.h"
#include "api/src/c/api-client.h"

#include "3party/Alohalytics/src/alohalytics.h"

#include <memory>

using namespace storage;
using namespace routing;
using namespace location;

using platform::CountryFile;
using platform::LocalCountryFile;

#ifdef FIXED_LOCATION
Framework::FixedPosition::FixedPosition()
{
  m_fixedLatLon = Settings::Get("FixPosition", m_latlon);
  m_fixedDir = Settings::Get("FixDirection", m_dirFromNorth);
}
#endif

namespace
{
char const kMapStyleKey[] = "MapStyleKeyV1";
char const kAllow3dKey[] = "Allow3d";
char const kAllow3dBuildingsKey[] = "Buildings3d";
char const kAllowAutoZoom[] = "AutoZoom";
char const kTrafficEnabledKey[] = "TrafficEnabled";
char const kTransitSchemeEnabledKey[] = "TransitSchemeEnabled";
char const kTrafficSimplifiedColorsKey[] = "TrafficSimplifiedColors";
char const kLargeFontsSize[] = "LargeFontsSize";
char const kTranslitMode[] = "TransliterationMode";

#if defined(OMIM_METAL_AVAILABLE)
char const kMetalAllowed[] = "MetalAllowed";
#endif

#if defined(OMIM_OS_ANDROID)
char const kICUDataFile[] = "icudt57l.dat";
#endif

double const kLargeFontsScaleFactor = 1.6;
size_t constexpr kMaxTrafficCacheSizeBytes = 64 /* Mb */ * 1024 * 1024;

// TODO!
// To adjust GpsTrackFilter was added secret command "?gpstrackaccuracy:xxx;"
// where xxx is a new value for horizontal accuracy.
// This is temporary solution while we don't have a good filter.
bool ParseSetGpsTrackMinAccuracyCommand(string const & query)
{
  const char kGpsAccuracy[] = "?gpstrackaccuracy:";
  if (!strings::StartsWith(query, kGpsAccuracy))
    return false;

  size_t const end = query.find(';', sizeof(kGpsAccuracy) - 1);
  if (end == string::npos)
    return false;

  string s(query.begin() + sizeof(kGpsAccuracy) - 1, query.begin() + end);
  double value;
  if (!strings::to_double(s, value))
    return false;

  GpsTrackFilter::StoreMinHorizontalAccuracy(value);
  return true;
}

string MakeSearchBookingUrl(booking::Api const & bookingApi, search::CityFinder & cityFinder,
                            FeatureType & ft)
{
  string name;
  auto const & info = ft.GetID().m_mwmId.GetInfo();
  ASSERT(info, ());

  int8_t lang = feature::GetNameForSearchOnBooking(info->GetRegionData(), ft.GetNames(), name);

  if (lang == StringUtf8Multilang::kUnsupportedLanguageCode)
    return {};

  string city = cityFinder.GetCityName(feature::GetCenter(ft), lang);

  return bookingApi.GetSearchUrl(city, name);
}

void OnRouteStartBuild(DataSource const & dataSource,
                       std::vector<RouteMarkData> const & routePoints, m2::PointD const & userPos)
{
  using eye::MapObject;

  if (routePoints.size() < 2)
    return;

  for (auto const & pt : routePoints)
  {
    if (pt.m_isMyPosition || pt.m_pointType == RouteMarkType::Start)
      continue;

    m2::RectD rect = MercatorBounds::RectByCenterXYAndOffset(pt.m_position, kMwmPointAccuracy);
    bool found = false;
    dataSource.ForEachInRect([&userPos, &pt, &found](FeatureType & ft)
    {
      if (found || !feature::GetCenter(ft).EqualDxDy(pt.m_position, kMwmPointAccuracy))
        return;

      auto const mapObject = utils::MakeEyeMapObject(ft);
      if (!mapObject.IsEmpty())
      {
        eye::Eye::Event::MapObjectEvent(mapObject, MapObject::Event::Type::RouteToCreated, userPos);
        found = true;
      }
    },
    rect, scales::GetUpperScale());
  }
}
}  // namespace

pair<MwmSet::MwmId, MwmSet::RegResult> Framework::RegisterMap(
    LocalCountryFile const & localFile)
{
  LOG(LINFO, ("Loading map:", localFile.GetCountryName()));
  return m_model.RegisterMap(localFile);
}

void Framework::OnLocationError(TLocationError /*error*/)
{
  m_trafficManager.UpdateMyPosition(TrafficManager::MyPosition());
  if (m_drapeEngine != nullptr)
    m_drapeEngine->LoseLocation();
}

void Framework::OnLocationUpdate(GpsInfo const & info)
{
#ifdef FIXED_LOCATION
  GpsInfo rInfo(info);

  // get fixed coordinates
  m_fixedPos.GetLon(rInfo.m_longitude);
  m_fixedPos.GetLat(rInfo.m_latitude);

  // pretend like GPS position
  rInfo.m_horizontalAccuracy = 5.0;

  if (m_fixedPos.HasNorth())
  {
    // pass compass value (for devices without compass)
    CompassInfo compass;
    m_fixedPos.GetNorth(compass.m_bearing);
    OnCompassUpdate(compass);
  }

#else
  GpsInfo rInfo(info);
#endif

  m_routingManager.OnLocationUpdate(rInfo);
  m_localAdsManager.OnLocationUpdate(rInfo, GetDrawScale());
}

void Framework::OnCompassUpdate(CompassInfo const & info)
{
#ifdef FIXED_LOCATION
  CompassInfo rInfo(info);
  m_fixedPos.GetNorth(rInfo.m_bearing);
#else
  CompassInfo const & rInfo = info;
#endif

  if (m_drapeEngine != nullptr)
    m_drapeEngine->SetCompassInfo(rInfo);
}

void Framework::SwitchMyPositionNextMode()
{
  if (m_drapeEngine != nullptr)
    m_drapeEngine->SwitchMyPositionNextMode();
}

void Framework::SetMyPositionModeListener(TMyPositionModeChanged && fn)
{
  m_myPositionListener = move(fn);
}

TrafficManager & Framework::GetTrafficManager()
{
  return m_trafficManager;
}

LocalAdsManager & Framework::GetLocalAdsManager()
{
  return m_localAdsManager;
}

TransitReadManager & Framework::GetTransitManager()
{
  return m_transitManager;
}

void Framework::OnUserPositionChanged(m2::PointD const & position, bool hasPosition)
{
  GetBookmarkManager().MyPositionMark().SetUserPosition(position, hasPosition);
  m_routingManager.SetUserCurrentPosition(position);
  m_trafficManager.UpdateMyPosition(TrafficManager::MyPosition(position));
}

void Framework::OnViewportChanged(ScreenBase const & screen)
{
  m_currentModelView = screen;

  GetSearchAPI().OnViewportChanged(GetCurrentViewport());

  GetBookmarkManager().UpdateViewport(m_currentModelView);
  m_trafficManager.UpdateViewport(m_currentModelView);
  m_localAdsManager.UpdateViewport(m_currentModelView);
  m_transitManager.UpdateViewport(m_currentModelView);

  if (m_viewportChanged != nullptr)
    m_viewportChanged(screen);
}

void Framework::StopLocationFollow()
{
  if (m_drapeEngine != nullptr)
    m_drapeEngine->StopLocationFollow();
}

bool Framework::IsEnoughSpaceForMigrate() const
{
  return GetPlatform().GetWritableStorageStatus(GetStorage().GetMaxMwmSizeBytes()) ==
         Platform::TStorageStatus::STORAGE_OK;
}

TCountryId Framework::PreMigrate(ms::LatLon const & position,
                           Storage::TChangeCountryFunction const & change,
                           Storage::TProgressFunction const & progress)
{
  GetStorage().PrefetchMigrateData();

  auto const infoGetter =
      CountryInfoReader::CreateCountryInfoReader(GetPlatform());

  TCountryId currentCountryId =
      infoGetter->GetRegionCountryId(MercatorBounds::FromLatLon(position));

  if (currentCountryId == kInvalidCountryId)
    return kInvalidCountryId;

  GetStorage().GetPrefetchStorage()->Subscribe(change, progress);
  GetStorage().GetPrefetchStorage()->DownloadNode(currentCountryId);
  return currentCountryId;
}

void Framework::Migrate(bool keepDownloaded)
{
  // Drape must be suspended while migration is performed since it access different parts of
  // framework (i.e. m_infoGetter) which are reinitialized during migration process.
  // If we do not suspend drape, it tries to access framework fields (i.e. m_infoGetter) which are null
  // while migration is performed.
  if (m_drapeEngine && m_isRenderingEnabled)
  {
    m_drapeEngine->SetRenderingDisabled(true);
    OnDestroyGLContext();
  }
  m_selectedFeature = FeatureID();
  m_discoveryManager.reset();
  m_searchAPI.reset();
  m_infoGetter.reset();
  m_taxiEngine.reset();
  m_cityFinder.reset();
  m_ugcApi.reset();
  TCountriesVec existedCountries;
  GetStorage().DeleteAllLocalMaps(&existedCountries);
  DeregisterAllMaps();
  m_model.Clear();
  GetStorage().Migrate(keepDownloaded ? existedCountries : TCountriesVec());
  InitCountryInfoGetter();
  InitUGC();
  InitSearchAPI();
  InitCityFinder();
  InitDiscoveryManager();
  InitTaxiEngine();
  RegisterAllMaps();

  m_trafficManager.SetCurrentDataVersion(GetStorage().GetCurrentDataVersion());
  if (m_drapeEngine && m_isRenderingEnabled)
  {
    m_drapeEngine->SetRenderingEnabled();
    OnRecoverGLContext(m_currentModelView.PixelRectIn3d().SizeX(),
                       m_currentModelView.PixelRectIn3d().SizeY());
  }
  InvalidateRect(MercatorBounds::FullRect());
}

Framework::Framework(FrameworkParams const & params)
  : m_localAdsManager(bind(&Framework::GetMwmsByRect, this, _1, true /* rough */),
                      bind(&Framework::GetMwmIdByName, this, _1),
                      bind(&Framework::ReadFeatures, this, _1, _2),
                      bind(&Framework::GetFeatureByID, this, _1, _2))
  , m_storage(platform::migrate::NeedMigrate() ? COUNTRIES_OBSOLETE_FILE : COUNTRIES_FILE)
  , m_enabledDiffs(params.m_enableDiffs)
  , m_isRenderingEnabled(true)
  , m_transitManager(m_model.GetDataSource(),
                     [this](FeatureCallback const & fn, vector<FeatureID> const & features) {
                       return m_model.ReadFeatures(fn, features);
                     },
                     bind(&Framework::GetMwmsByRect, this, _1, false /* rough */))
  , m_routingManager(
        RoutingManager::Callbacks(
            [this]() -> DataSource & { return m_model.GetDataSource(); },
            [this]() -> storage::CountryInfoGetter & { return GetCountryInfoGetter(); },
            [this](string const & id) -> string { return m_storage.GetParentIdFor(id); },
            [this]() -> StringsBundle const & { return m_stringsBundle; }),
        static_cast<RoutingManager::Delegate &>(*this))
  , m_trafficManager(bind(&Framework::GetMwmsByRect, this, _1, false /* rough */),
                     kMaxTrafficCacheSizeBytes, m_routingManager.RoutingSession())
  , m_bookingFilterProcessor(m_model.GetDataSource(), *m_bookingApi)
  , m_displacementModeManager([this](bool show) {
    int const mode = show ? dp::displacement::kHotelMode : dp::displacement::kDefaultMode;
    if (m_drapeEngine != nullptr)
      m_drapeEngine->SetDisplacementMode(mode);
  })
  , m_lastReportedCountry(kInvalidCountryId)
  , m_popularityLoader(m_model.GetDataSource())
  , m_descriptionsLoader(std::make_unique<descriptions::Loader>(m_model.GetDataSource()))
  , m_purchase(std::make_unique<Purchase>())
  , m_tipsApi(static_cast<TipsApi::Delegate &>(*this))
  , m_notificationManager(static_cast<notifications::NotificationManager::Delegate &>(*this))
{
  CHECK(IsLittleEndian(), ("Only little-endian architectures are supported."));

  // Editor should be initialized from the main thread to set its ThreadChecker.
  // However, search calls editor upon initialization thus setting the lazy editor's ThreadChecker
  // to a wrong thread. So editor should be initialiazed before serach.
  osm::Editor & editor = osm::Editor::Instance();

  // Restore map style before classificator loading
  MapStyle mapStyle = kDefaultMapStyle;
  string mapStyleStr;
  if (settings::Get(kMapStyleKey, mapStyleStr))
    mapStyle = MapStyleFromSettings(mapStyleStr);
  GetStyleReader().SetCurrentStyle(mapStyle);
  df::LoadTransitColors();

  m_connectToGpsTrack = GpsTracker::Instance().IsEnabled();
  
  // Init strings bundle.
  // @TODO. There are hardcoded strings below which are defined in strings.txt as well.
  // It's better to use strings from strings.txt instead of hardcoding them here.
  m_stringsBundle.SetDefaultString("core_entrance", "Entrance");
  m_stringsBundle.SetDefaultString("core_exit", "Exit");
  m_stringsBundle.SetDefaultString("core_placepage_unknown_place", "Unknown Place");
  m_stringsBundle.SetDefaultString("core_my_places", "My Places");
  m_stringsBundle.SetDefaultString("core_my_position", "My Position");
  // Wi-Fi string is used in categories that's why does not have core_ prefix
  m_stringsBundle.SetDefaultString("wifi", "WiFi");

  m_model.InitClassificator();
  m_model.SetOnMapDeregisteredCallback(bind(&Framework::OnMapDeregistered, this, _1));
  LOG(LDEBUG, ("Classificator initialized"));

  m_displayedCategories = make_unique<search::DisplayedCategories>(GetDefaultCategories());

  // To avoid possible races - init country info getter in constructor.
  InitCountryInfoGetter();
  LOG(LDEBUG, ("Country info getter initialized"));

  InitUGC();
  LOG(LDEBUG, ("UGC initialized"));

  InitSearchAPI();
  LOG(LDEBUG, ("Search API initialized"));

  m_bmManager = make_unique<BookmarkManager>(m_user, BookmarkManager::Callbacks(
      [this]() -> StringsBundle const & { return m_stringsBundle; },
      [this](vector<pair<kml::MarkId, kml::BookmarkData>> const & marks) {
        GetSearchAPI().OnBookmarksCreated(marks);
      },
      [this](vector<pair<kml::MarkId, kml::BookmarkData>> const & marks) {
        GetSearchAPI().OnBookmarksUpdated(marks);
      },
      [this](vector<kml::MarkId> const & marks) { GetSearchAPI().OnBookmarksDeleted(marks); }));

  m_ParsedMapApi.SetBookmarkManager(m_bmManager.get());
  m_routingManager.SetBookmarkManager(m_bmManager.get());
  m_searchMarks.SetBookmarkManager(m_bmManager.get());

  m_user.AddSubscriber(m_bmManager->GetUserSubscriber());

  m_routingManager.SetTransitManager(&m_transitManager);
  m_routingManager.SetRouteStartBuildListener([this](std::vector<RouteMarkData> const & points)
  {
    auto const userPos = GetCurrentPosition();
    if (userPos)
      OnRouteStartBuild(m_model.GetDataSource(), points, userPos.get());
  });

  InitCityFinder();
  InitDiscoveryManager();
  InitTaxiEngine();

  // All members which re-initialize in Migrate() method should be initialized before RegisterAllMaps().
  // Migrate() can be called from RegisterAllMaps().
  RegisterAllMaps();
  LOG(LDEBUG, ("Maps initialized"));

  // Need to reload cities boundaries because maps in indexer were updated.
  GetSearchAPI().LoadCitiesBoundaries();

  // Init storage with needed callback.
  m_storage.Init(
                 bind(&Framework::OnCountryFileDownloaded, this, _1, _2),
                 bind(&Framework::OnCountryFileDelete, this, _1, _2));
  m_storage.SetDownloadingPolicy(&m_storageDownloadingPolicy);
  m_storage.SetStartDownloadingCallback([this]() { UpdatePlacePageInfoForCurrentSelection(); });
  LOG(LDEBUG, ("Storage initialized"));

  // Local ads manager should be initialized after storage initialization.
  if (params.m_enableLocalAds)
  {
    auto const isActive = m_purchase->IsSubscriptionActive(SubscriptionType::RemoveAds);
    m_localAdsManager.Startup(m_bmManager.get(), !isActive);
    m_purchase->RegisterSubscription(&m_localAdsManager);
  }

  m_routingManager.SetRouterImpl(RouterType::Vehicle);

  UpdateMinBuildingsTapZoom();

  LOG(LDEBUG, ("Routing engine initialized"));

  LOG(LINFO, ("System languages:", languages::GetPreferred()));

  editor.SetDelegate(make_unique<search::EditorDelegate>(m_model.GetDataSource()));
  editor.SetInvalidateFn([this](){ InvalidateRect(GetCurrentViewport()); });
  editor.LoadEdits();

  m_model.GetDataSource().AddObserver(editor);

  LOG(LINFO, ("Editor initialized"));

  m_trafficManager.SetCurrentDataVersion(m_storage.GetCurrentDataVersion());
  m_trafficManager.SetSimplifiedColorScheme(LoadTrafficSimplifiedColors());
  m_trafficManager.SetEnabled(LoadTrafficEnabled());

  m_adsEngine = make_unique<ads::Engine>();

  InitTransliteration();
  LOG(LDEBUG, ("Transliterators initialized"));

  m_notificationManager.Load();
  m_notificationManager.TrimExpired();
  eye::Eye::Instance().TrimExpired();
  eye::Eye::Instance().Subscribe(&m_notificationManager);

  GetPowerManager().Subscribe(this);
}

Framework::~Framework()
{
  eye::Eye::Instance().UnsubscribeAll();
  GetPowerManager().UnsubscribeAll();

  m_threadRunner.reset();

  // Must be destroyed implicitly at the start of destruction,
  // since it stores raw pointers to other subsystems.
  m_purchase.reset();

  osm::Editor & editor = osm::Editor::Instance();

  editor.SetDelegate({});
  editor.SetInvalidateFn({});

  GetBookmarkManager().Teardown();
  m_trafficManager.Teardown();
  DestroyDrapeEngine();
  m_model.SetOnMapDeregisteredCallback(nullptr);

  m_user.ClearSubscribers();
  // Must be destroyed implicitly, since it stores reference to m_user.
  m_bmManager.reset();
}

booking::Api * Framework::GetBookingApi(platform::NetworkPolicy const & policy)
{
  ASSERT(m_bookingApi, ());
  if (policy.CanUse())
    return m_bookingApi.get();

  return nullptr;
}

booking::Api const * Framework::GetBookingApi(platform::NetworkPolicy const & policy) const
{
  ASSERT(m_bookingApi, ());
  if (policy.CanUse())
    return m_bookingApi.get();

  return nullptr;
}

viator::Api * Framework::GetViatorApi(platform::NetworkPolicy const & policy)
{
  ASSERT(m_viatorApi, ());
  if (policy.CanUse())
    return m_viatorApi.get();

  return nullptr;
}

taxi::Engine * Framework::GetTaxiEngine(platform::NetworkPolicy const & policy)
{
  ASSERT(m_taxiEngine, ());
  if (policy.CanUse())
    return m_taxiEngine.get();

  return nullptr;
}

locals::Api * Framework::GetLocalsApi(platform::NetworkPolicy const & policy)
{
  ASSERT(m_localsApi, ());
  if (policy.CanUse())
    return m_localsApi.get();

  return nullptr;
}

void Framework::ShowNode(storage::TCountryId const & countryId)
{
  StopLocationFollow();

  ShowRect(CalcLimitRect(countryId, GetStorage(), GetCountryInfoGetter()));
}

void Framework::OnCountryFileDownloaded(storage::TCountryId const & countryId, storage::TLocalFilePtr const localFile)
{
  // Soft reset to signal that mwm file may be out of date in routing caches.
  m_routingManager.ResetRoutingSession();

  m2::RectD rect = MercatorBounds::FullRect();

  if (localFile && HasOptions(localFile->GetFiles(), MapOptions::Map))
  {
    // Add downloaded map.
    auto p = m_model.RegisterMap(*localFile);
    MwmSet::MwmId const & id = p.first;
    if (id.IsAlive())
      rect = id.GetInfo()->m_bordersRect;
  }
  m_trafficManager.Invalidate();
  m_transitManager.Invalidate();
  m_localAdsManager.OnDownloadCountry(countryId);
  InvalidateRect(rect);
  GetSearchAPI().ClearCaches();
}

bool Framework::OnCountryFileDelete(storage::TCountryId const & countryId, storage::TLocalFilePtr const localFile)
{
  // Soft reset to signal that mwm file may be out of date in routing caches.
  m_routingManager.ResetRoutingSession();

  if (countryId == m_lastReportedCountry)
    m_lastReportedCountry = kInvalidCountryId;

  CancelAllSearches();

  m2::RectD rect = MercatorBounds::FullRect();

  bool deferredDelete = false;
  if (localFile)
  {
    rect = m_infoGetter->GetLimitRectForLeaf(countryId);
    m_model.DeregisterMap(platform::CountryFile(countryId));
    deferredDelete = true;
  }
  InvalidateRect(rect);

  GetSearchAPI().ClearCaches();
  return deferredDelete;
}

void Framework::OnMapDeregistered(platform::LocalCountryFile const & localFile)
{
  m_localAdsManager.OnMwmDeregistered(localFile);
  m_transitManager.OnMwmDeregistered(localFile);
  m_trafficManager.OnMwmDeregistered(localFile);
  m_popularityLoader.OnMwmDeregistered(localFile);

  auto action = [this, localFile]
  {
    m_storage.DeleteCustomCountryVersion(localFile);
  };

  // Call action on thread in which the framework was created
  // For more information look at comment for Observer class in mwm_set.hpp
  if (m_storage.GetThreadChecker().CalledOnOriginalThread())
    action();
  else
    GetPlatform().RunTask(Platform::Thread::Gui, action);
}

bool Framework::HasUnsavedEdits(storage::TCountryId const & countryId)
{
  bool hasUnsavedChanges = false;
  auto const forEachInSubtree = [&hasUnsavedChanges, this](storage::TCountryId const & fileName,
      bool groupNode)
  {
    if (groupNode)
      return;
    hasUnsavedChanges |= osm::Editor::Instance().HaveMapEditsToUpload(
          m_model.GetDataSource().GetMwmIdByCountryFile(platform::CountryFile(fileName)));
  };
  GetStorage().ForEachInSubtree(countryId, forEachInSubtree);
  return hasUnsavedChanges;
}

void Framework::RegisterAllMaps()
{
  ASSERT(!m_storage.IsDownloadInProgress(),
         ("Registering maps while map downloading leads to removing downloading maps from "
          "ActiveMapsListener::m_items."));

  m_storage.RegisterAllLocalMaps(m_enabledDiffs);

  // Fast migrate in case there are no downloaded MWM.
  if (platform::migrate::NeedMigrate())
  {
    bool disableFastMigrate;
    if (!settings::Get("DisableFastMigrate", disableFastMigrate))
      disableFastMigrate = false;
    if (!disableFastMigrate && !m_storage.HaveDownloadedCountries())
    {
      GetStorage().PrefetchMigrateData();
      Migrate();
      return;
    }
  }

  int minFormat = numeric_limits<int>::max();

  char const * kLastDownloadedMapsCheck = "LastDownloadedMapsCheck";
  auto const updateInterval = hours(24 * 7); // One week.
  uint32_t timestamp;
  bool const rc = settings::Get(kLastDownloadedMapsCheck, timestamp);
  auto const lastCheck = time_point<system_clock>(seconds(timestamp));
  bool const needStatisticsUpdate = !rc || lastCheck < system_clock::now() - updateInterval;
  stringstream listRegisteredMaps;

  vector<shared_ptr<LocalCountryFile>> maps;
  m_storage.GetLocalMaps(maps);
  for (auto const & localFile : maps)
  {
    auto p = RegisterMap(*localFile);
    if (p.second != MwmSet::RegResult::Success)
      continue;

    MwmSet::MwmId const & id = p.first;
    ASSERT(id.IsAlive(), ());
    minFormat = min(minFormat, static_cast<int>(id.GetInfo()->m_version.GetFormat()));
    if (needStatisticsUpdate)
    {
      listRegisteredMaps << localFile->GetCountryName() << ":" << id.GetInfo()->GetVersion() << ";";
    }
  }

  if (needStatisticsUpdate)
  {
    alohalytics::Stats::Instance().LogEvent("Downloader_Map_list",
    {{"AvailableStorageSpace", strings::to_string(GetPlatform().GetWritableStorageSpace())},
      {"DownloadedMaps", listRegisteredMaps.str()}});
    settings::Set(kLastDownloadedMapsCheck,
                  static_cast<uint64_t>(duration_cast<seconds>(
                                          system_clock::now().time_since_epoch()).count()));
  }
}

void Framework::DeregisterAllMaps()
{
  m_model.Clear();
  m_storage.Clear();
}

void Framework::LoadBookmarks()
{
  GetBookmarkManager().LoadBookmarks();
}

kml::MarkGroupId Framework::AddCategory(string const & categoryName)
{
  return GetBookmarkManager().CreateBookmarkCategory(categoryName);
}

void Framework::FillBookmarkInfo(Bookmark const & bmk, place_page::Info & info) const
{
  info.SetBookmarkCategoryName(GetBookmarkManager().GetCategoryName(bmk.GetGroupId()));
  info.SetBookmarkData(bmk.GetData());
  info.SetBookmarkId(bmk.GetId());
  info.SetBookmarkCategoryId(bmk.GetGroupId());
  FillPointInfo(bmk.GetPivot(), {} /* customTitle */, info);
}

void Framework::ResetBookmarkInfo(Bookmark const & bmk, place_page::Info & info) const
{
  info.SetBookmarkCategoryName("");
  info.SetBookmarkData({});
  info.SetBookmarkId(kml::kInvalidMarkId);
  info.SetBookmarkCategoryId(kml::kInvalidMarkGroupId);
  FillPointInfo(bmk.GetPivot(), {} /* customTitle */, info);
}

void Framework::FillFeatureInfo(FeatureID const & fid, place_page::Info & info) const
{
  if (!fid.IsValid())
  {
    LOG(LERROR, ("FeatureID is invalid:", fid));
    return;
  }

  FeaturesLoaderGuard const guard(m_model.GetDataSource(), fid.m_mwmId);
  FeatureType ft;
  if (!guard.GetFeatureByIndex(fid.m_index, ft))
  {
    LOG(LERROR, ("Feature can't be loaded:", fid));
    return;
  }

  FillInfoFromFeatureType(ft, info);

  // Fill countryId for place page info
  uint32_t const placeContinentType = classif().GetTypeByPath({"place", "continent"});
  if (info.GetTypes().Has(placeContinentType))
    return;

  uint32_t const placeCountryType = classif().GetTypeByPath({"place", "country"});
  uint32_t const placeStateType = classif().GetTypeByPath({"place", "state"});

  bool const isState = info.GetTypes().Has(placeStateType);
  bool const isCountry = info.GetTypes().Has(placeCountryType);
  if (isCountry || isState)
  {
    size_t const level = isState ? 1 : 0;
    TCountriesVec countries;
    TCountryId countryId = m_infoGetter->GetRegionCountryId(info.GetMercator());
    GetStorage().GetTopmostNodesFor(countryId, countries, level);
    if (countries.size() == 1)
      countryId = countries.front();

    info.SetCountryId(countryId);
    info.SetTopmostCountryIds(move(countries));
  }
}

void Framework::FillPointInfo(m2::PointD const & mercator, string const & customTitle, place_page::Info & info) const
{
  auto feature = GetFeatureAtPoint(mercator);
  if (feature)
  {
    FillInfoFromFeatureType(*feature, info);
  }
  else
  {
    if (customTitle.empty())
      info.SetCustomNameWithCoordinates(mercator, m_stringsBundle.GetString("core_placepage_unknown_place"));
    else
      info.SetCustomName(customTitle);
    info.SetCanEditOrAdd(CanEditMap());
  }

  // This line overwrites mercator center from area feature which can be far away.
  info.SetMercator(mercator);
}

void Framework::FillInfoFromFeatureType(FeatureType & ft, place_page::Info & info) const
{
  using place_page::SponsoredType;
  auto const featureStatus = osm::Editor::Instance().GetFeatureStatus(ft.GetID());
  ASSERT_NOT_EQUAL(featureStatus, FeatureStatus::Deleted,
                   ("Deleted features cannot be selected from UI."));
  info.SetFeatureStatus(featureStatus);
  info.SetLocalizedWifiString(m_stringsBundle.GetString("wifi"));

  if (ftypes::IsAddressObjectChecker::Instance()(ft))
    info.SetAddress(GetAddressInfoAtPoint(feature::GetCenter(ft)).FormatHouseAndStreet());

  info.SetFromFeatureType(ft);

  if (ftypes::IsBookingChecker::Instance()(ft))
  {
    ASSERT(m_bookingApi, ());
    info.SetSponsoredType(SponsoredType::Booking);
    auto const & baseUrl = info.GetMetadata().Get(feature::Metadata::FMD_WEBSITE);
    auto const & hotelId = info.GetMetadata().Get(feature::Metadata::FMD_SPONSORED_ID);
    info.SetSponsoredUrl(m_bookingApi->GetBookHotelUrl(baseUrl));
    info.SetSponsoredDeepLink(m_bookingApi->GetDeepLink(hotelId));
    info.SetSponsoredDescriptionUrl(m_bookingApi->GetDescriptionUrl(baseUrl));
    info.SetSponsoredReviewUrl(m_bookingApi->GetHotelReviewsUrl(hotelId, baseUrl));
    if (!m_bookingAvailabilityParams.IsEmpty())
    {
      auto const & url = info.GetSponsoredUrl();
      auto const & urlWithParams =
          m_bookingApi->ApplyAvailabilityParams(url, m_bookingAvailabilityParams);
      info.SetSponsoredUrl(urlWithParams);

      auto const & deepLink = info.GetSponsoredDeepLink();
      auto const & deepLinkWithParams =
          m_bookingApi->ApplyAvailabilityParams(deepLink, m_bookingAvailabilityParams);
      info.SetSponsoredDeepLink(deepLinkWithParams);
    }
  }
  else if (ftypes::IsOpentableChecker::Instance()(ft))
  {
    info.SetSponsoredType(SponsoredType::Opentable);
    auto const & sponsoredId = info.GetMetadata().Get(feature::Metadata::FMD_SPONSORED_ID);
    auto const & url = opentable::Api::GetBookTableUrl(sponsoredId);
    info.SetSponsoredUrl(url);
    info.SetSponsoredDescriptionUrl(url);
  }
  else if (ftypes::IsHotelChecker::Instance()(ft))
  {
    ASSERT(m_cityFinder, ());
    auto const url = MakeSearchBookingUrl(*m_bookingApi, *m_cityFinder, ft);
    info.SetBookingSearchUrl(url);
    LOG(LINFO, (url));
  }
  else if (m_purchase && !m_purchase->IsSubscriptionActive(SubscriptionType::RemoveAds) &&
           PartnerChecker::Instance()(ft))
  {
    info.SetSponsoredType(place_page::SponsoredType::Partner);
    auto const partnerIndex = PartnerChecker::Instance().GetPartnerIndex(ft);
    info.SetPartnerIndex(partnerIndex);
    auto const & partnerInfo = GetPartnerByIndex(partnerIndex);
    if (partnerInfo.m_hasButton && ft.GetID().GetMwmVersion() >= partnerInfo.m_minMapVersion)
    {
      auto url = info.GetMetadata().Get(feature::Metadata::FMD_BANNER_URL);
      if (url.empty())
        url = partnerInfo.m_defaultBannerUrl;
      info.SetSponsoredUrl(url);
      info.SetSponsoredDescriptionUrl(url);
    }
  }
  else if (ftypes::IsHolidayChecker::Instance()(ft) &&
           !info.GetMetadata().Get(feature::Metadata::FMD_RATING).empty())
  {
    info.SetSponsoredType(place_page::SponsoredType::Holiday);
  }

  FillLocalExperts(ft, info);
  FillDescription(ft, info);

  auto const mwmInfo = ft.GetID().m_mwmId.GetInfo();
  bool const isMapVersionEditable = mwmInfo && mwmInfo->m_version.IsEditableMap();
  bool const canEditOrAdd = featureStatus != FeatureStatus::Obsolete && CanEditMap() &&
                            !info.IsNotEditableSponsored() && isMapVersionEditable;
  info.SetCanEditOrAdd(canEditOrAdd);

  if (m_localAdsManager.IsSupportedType(info.GetTypes()))
  {
    info.SetLocalAdsUrl(m_localAdsManager.GetCompanyUrl(ft.GetID()));
    auto const status = m_localAdsManager.Contains(ft.GetID())
                            ? place_page::LocalAdsStatus::Customer
                            : place_page::LocalAdsStatus::Candidate;
    info.SetLocalAdsStatus(status);
  }
  else
  {
    info.SetLocalAdsStatus(place_page::LocalAdsStatus::NotAvailable);
  }

  auto const latlon = MercatorBounds::ToLatLon(feature::GetCenter(ft));
  ASSERT(m_taxiEngine, ());
  info.SetReachableByTaxiProviders(m_taxiEngine->GetProvidersAtPos(latlon));
  info.SetPopularity(m_popularityLoader.Get(ft.GetID()));
}

void Framework::FillApiMarkInfo(ApiMarkPoint const & api, place_page::Info & info) const
{
  FillPointInfo(api.GetPivot(), {} /* customTitle */, info);
  string const & name = api.GetName();
  if (!name.empty())
    info.SetCustomName(name);
  info.SetApiId(api.GetApiID());
  info.SetApiUrl(GenerateApiBackUrl(api));
}

void Framework::FillSearchResultInfo(SearchMarkPoint const & smp, place_page::Info & info) const
{
  if (smp.GetFeatureID().IsValid())
    FillFeatureInfo(smp.GetFeatureID(), info);
  else
    FillPointInfo(smp.GetPivot(), smp.GetMatchedName(), info);
}

void Framework::FillMyPositionInfo(place_page::Info & info, df::TapInfo const & tapInfo) const
{
  auto const position = GetCurrentPosition();
  VERIFY(position, ());
  info.SetMercator(*position);
  info.SetIsMyPosition();
  info.SetCustomName(m_stringsBundle.GetString("core_my_position"));

  UserMark const * mark = FindUserMarkInTapPosition(tapInfo);
  if (mark != nullptr && mark->GetMarkType() == UserMark::Type::ROUTING)
  {
    auto routingMark = static_cast<RouteMarkPoint const *>(mark);
    info.SetIsRoutePoint();
    info.SetRouteMarkType(routingMark->GetRoutePointType());
    info.SetIntermediateIndex(routingMark->GetIntermediateIndex());
  }
}

void Framework::FillRouteMarkInfo(RouteMarkPoint const & rmp, place_page::Info & info) const
{
  FillPointInfo(rmp.GetPivot(), {} /* customTitle */, info);
  info.SetIsRoutePoint();
  info.SetRouteMarkType(rmp.GetRoutePointType());
  info.SetIntermediateIndex(rmp.GetIntermediateIndex());
}

void Framework::ShowBookmark(kml::MarkId id)
{
  auto const * mark = m_bmManager->GetBookmark(id);
  ShowBookmark(mark);
}

void Framework::ShowBookmark(Bookmark const * mark)
{
  if (mark == nullptr)
    return;

  StopLocationFollow();

  auto scale = static_cast<int>(mark->GetScale());
  if (scale == 0)
    scale = scales::GetUpperComfortScale();

  if (m_drapeEngine != nullptr)
    m_drapeEngine->SetModelViewCenter(mark->GetPivot(), scale, true /* isAnim */,
                                      true /* trackVisibleViewport */);

  place_page::Info info;
  FillBookmarkInfo(*mark, info);
  ActivateMapSelection(true, df::SelectionShape::OBJECT_USER_MARK, info);
  // TODO
  // We need to preserve bookmark id in the m_lastTapEvent, because one feature can have several bookmarks.
  m_lastTapEvent = MakeTapEvent(info.GetMercator(), info.GetID(), TapEvent::Source::Other);
}

void Framework::ShowTrack(kml::TrackId trackId)
{
  double const kPaddingScale = 1.2;

  auto const track = GetBookmarkManager().GetTrack(trackId);
  if (track == nullptr)
    return;

  StopLocationFollow();
  auto rect = track->GetLimitRect();
  rect.Scale(kPaddingScale);

  ShowRect(rect);
}

void Framework::ShowBookmarkCategory(kml::MarkGroupId categoryId)
{
  auto const & bm = GetBookmarkManager();
  if (bm.IsCategoryEmpty(categoryId))
    return;

  m2::RectD rect;
  for (auto const id : bm.GetUserMarkIds(categoryId))
  {
    auto const bookmark = bm.GetBookmark(id);
    rect.Add(bookmark->GetPivot());
  }
  for (auto const id : bm.GetTrackIds(categoryId))
  {
    auto const track = bm.GetTrack(id);
    rect.Add(track->GetLimitRect());
  }
  if (!rect.IsValid())
    return;

  double const kPaddingScale = 1.2;
  StopLocationFollow();
  rect.Scale(kPaddingScale);
  ShowRect(rect);
}

void Framework::ShowFeatureByMercator(m2::PointD const & pt)
{
  StopLocationFollow();

  if (m_drapeEngine != nullptr)
  {
    m_drapeEngine->SetModelViewCenter(pt, scales::GetUpperComfortScale(), true /* isAnim */,
                                      true /* trackVisibleViewport */);
  }

  place_page::Info info;
  std::string name;
  GetBookmarkManager().SelectionMark().SetPtOrg(pt);
  FillPointInfo(pt, name, info);
  ActivateMapSelection(false, df::SelectionShape::OBJECT_POI, info);
  m_lastTapEvent = MakeTapEvent(info.GetMercator(), info.GetID(), TapEvent::Source::Other);
}

void Framework::AddBookmarksFile(string const & filePath, bool isTemporaryFile)
{
  GetBookmarkManager().LoadBookmark(filePath, isTemporaryFile);
}

void Framework::PrepareToShutdown()
{
  DestroyDrapeEngine();
}

void Framework::SetDisplacementMode(DisplacementModeManager::Slot slot, bool show)
{
  m_displacementModeManager.Set(slot, show);
}

void Framework::SaveViewport()
{
  m2::AnyRectD rect;
  if (m_currentModelView.isPerspective())
  {
    ScreenBase modelView = m_currentModelView;
    modelView.ResetPerspective();
    rect = modelView.GlobalRect();
  }
  else
  {
    rect = m_currentModelView.GlobalRect();
  }
  settings::Set("ScreenClipRect", rect);
}

void Framework::LoadViewport()
{
  m2::AnyRectD rect;
  if (settings::Get("ScreenClipRect", rect) && df::GetWorldRect().IsRectInside(rect.GetGlobalRect()))
  {
    if (m_drapeEngine != nullptr)
      m_drapeEngine->SetModelViewAnyRect(rect, false /* isAnim */);
  }
  else
  {
    ShowAll();
  }
}

void Framework::ShowAll()
{
  if (m_drapeEngine != nullptr)
    m_drapeEngine->SetModelViewAnyRect(m2::AnyRectD(m_model.GetWorldRect()), false /* isAnim */);
}

m2::PointD Framework::GetPixelCenter() const
{
  return m_currentModelView.PixelRectIn3d().Center();
}

m2::PointD Framework::GetVisiblePixelCenter() const
{
  return m_visibleViewport.Center();
}

m2::PointD const & Framework::GetViewportCenter() const
{
  return m_currentModelView.GetOrg();
}

void Framework::SetViewportCenter(m2::PointD const & pt)
{
  SetViewportCenter(pt, -1);
}

void Framework::SetViewportCenter(m2::PointD const & pt, int zoomLevel)
{
  if (m_drapeEngine != nullptr)
    m_drapeEngine->SetModelViewCenter(pt, zoomLevel, true /* isAnim */, false /* trackVisibleViewport */);
}

m2::RectD Framework::GetCurrentViewport() const
{
  return m_currentModelView.ClipRect();
}

void Framework::SetVisibleViewport(m2::RectD const & rect)
{
  if (m_drapeEngine == nullptr)
    return;
  m_visibleViewport = rect;
  m_drapeEngine->SetVisibleViewport(rect);
}

void Framework::ShowRect(m2::RectD const & rect, int maxScale, bool animation)
{
  if (m_drapeEngine == nullptr)
    return;

  m_drapeEngine->SetModelViewRect(rect, true /* applyRotation */,
                                  maxScale /* zoom */, animation);
}

void Framework::ShowRect(m2::AnyRectD const & rect)
{
  if (m_drapeEngine != nullptr)
    m_drapeEngine->SetModelViewAnyRect(rect, true /* isAnim */);
}

void Framework::GetTouchRect(m2::PointD const & center, uint32_t pxRadius, m2::AnyRectD & rect)
{
  m_currentModelView.GetTouchRect(center, static_cast<double>(pxRadius), rect);
}

void Framework::SetViewportListener(TViewportChanged const & fn)
{
  m_viewportChanged = fn;
}

void Framework::OnSize(int w, int h)
{
  if (m_drapeEngine != nullptr)
    m_drapeEngine->Resize(std::max(w, 2), std::max(h, 2));
}

namespace
{

double ScaleModeToFactor(Framework::EScaleMode mode)
{
  double factors[] = { 2.0, 1.5, 0.5, 0.67 };
  return factors[mode];
}

} // namespace

void Framework::Scale(EScaleMode mode, bool isAnim)
{
  Scale(ScaleModeToFactor(mode), isAnim);
}

void Framework::Scale(Framework::EScaleMode mode, m2::PointD const & pxPoint, bool isAnim)
{
  Scale(ScaleModeToFactor(mode), pxPoint, isAnim);
}

void Framework::Scale(double factor, bool isAnim)
{
  Scale(factor, GetVisiblePixelCenter(), isAnim);
}

void Framework::Scale(double factor, m2::PointD const & pxPoint, bool isAnim)
{
  if (m_drapeEngine != nullptr)
    m_drapeEngine->Scale(factor, pxPoint, isAnim);
}

void Framework::TouchEvent(df::TouchEvent const & touch)
{
  if (m_drapeEngine != nullptr)
    m_drapeEngine->AddTouchEvent(touch);
}

int Framework::GetDrawScale() const
{
  return df::GetDrawTileScale(m_currentModelView);
}

void Framework::RunFirstLaunchAnimation()
{
  if (m_drapeEngine != nullptr)
    m_drapeEngine->RunFirstLaunchAnimation();
}

bool Framework::IsCountryLoaded(m2::PointD const & pt) const
{
  // TODO (@gorshenin, @govako): the method's name is quite
  // obfuscating and should be fixed.

  // Correct, but slow version (check country polygon).
  string const fName = m_infoGetter->GetRegionCountryId(pt);
  if (fName.empty())
    return true;

  return m_model.IsLoaded(fName);
}

bool Framework::IsCountryLoadedByName(string const & name) const
{
  return m_model.IsLoaded(name);
}

void Framework::InvalidateRect(m2::RectD const & rect)
{
  if (m_drapeEngine != nullptr)
    m_drapeEngine->InvalidateRect(rect);
}

void Framework::ClearAllCaches()
{
  m_model.ClearCaches();
  m_infoGetter->ClearCaches();
  GetSearchAPI().ClearCaches();
}

void Framework::OnUpdateCurrentCountry(m2::PointD const & pt, int zoomLevel)
{
  storage::TCountryId newCountryId;
  if (zoomLevel > scales::GetUpperWorldScale())
    newCountryId = m_infoGetter->GetRegionCountryId(pt);

  if (newCountryId == m_lastReportedCountry)
    return;

  m_lastReportedCountry = newCountryId;

  GetPlatform().RunTask(Platform::Thread::Gui, [this, newCountryId]()
  {
    if (m_currentCountryChanged != nullptr)
      m_currentCountryChanged(newCountryId);
  });
}

void Framework::SetCurrentCountryChangedListener(TCurrentCountryChanged const & listener)
{
  m_currentCountryChanged = listener;
  m_lastReportedCountry = kInvalidCountryId;
}

void Framework::PokeSearchInViewport(bool forceSearch)
{
  return GetSearchAPI().PokeSearchInViewport(forceSearch);
}

bool Framework::SearchEverywhere(search::EverywhereSearchParams const & params)
{
  return GetSearchAPI().SearchEverywhere(params);
}

bool Framework::SearchInViewport(search::ViewportSearchParams const & params)
{
  return GetSearchAPI().SearchInViewport(params);
}

bool Framework::SearchInDownloader(DownloaderSearchParams const & params)
{
  return GetSearchAPI().SearchInDownloader(params);
}

bool Framework::SearchInBookmarks(search::BookmarksSearchParams const & params)
{
  return GetSearchAPI().SearchInBookmarks(params);
}

void Framework::CancelSearch(search::Mode mode)
{
  return GetSearchAPI().CancelSearch(mode);
}

void Framework::CancelAllSearches()
{
  return GetSearchAPI().CancelAllSearches();
}

void Framework::MemoryWarning()
{
  LOG(LINFO, ("MemoryWarning"));
  ClearAllCaches();
  SharedBufferManager::instance().clearReserved();
}

void Framework::EnterBackground()
{
  m_startBackgroundTime = base::Timer::LocalTime();
  settings::Set("LastEnterBackground", m_startBackgroundTime);

  SaveViewport();

  m_ugcApi->SaveUGCOnDisk();

  m_trafficManager.OnEnterBackground();
  m_routingManager.SetAllowSendingPoints(false);

  ms::LatLon const ll = MercatorBounds::ToLatLon(GetViewportCenter());
  alohalytics::Stats::Instance().LogEvent("Framework::EnterBackground", {{"zoom", strings::to_string(GetDrawScale())},
                                          {"foregroundSeconds", strings::to_string(
                                           static_cast<int>(m_startBackgroundTime - m_startForegroundTime))}},
                                          alohalytics::Location::FromLatLon(ll.lat, ll.lon));
  // Do not clear caches for Android. This function is called when main activity is paused,
  // but at the same time search activity (for example) is enabled.
  // TODO(AlexZ): Use onStart/onStop on Android to correctly detect app background and remove #ifndef.
#ifndef OMIM_OS_ANDROID
  ClearAllCaches();
#endif
}

void Framework::EnterForeground()
{
  m_startForegroundTime = base::Timer::LocalTime();
  if (m_drapeEngine != nullptr && m_startBackgroundTime != 0.0)
  {
    auto const timeInBackground = m_startForegroundTime - m_startBackgroundTime;
    m_drapeEngine->SetTimeInBackground(timeInBackground);
  }

  m_trafficManager.OnEnterForeground();
  m_routingManager.SetAllowSendingPoints(true);
}

void Framework::InitCountryInfoGetter()
{
  ASSERT(!m_infoGetter.get(), ("InitCountryInfoGetter() must be called only once."));

  auto const & platform = GetPlatform();
  if (platform::migrate::NeedMigrate())
    m_infoGetter = CountryInfoReader::CreateCountryInfoReaderObsolete(platform);
  else
    m_infoGetter = CountryInfoReader::CreateCountryInfoReader(platform);
  m_infoGetter->InitAffiliationsInfo(&m_storage.GetAffiliations());
}

void Framework::InitUGC()
{
  ASSERT(!m_ugcApi.get(), ("InitUGC() must be called only once."));

  m_ugcApi = make_unique<ugc::Api>(m_model.GetDataSource(), [this](size_t numberOfUnsynchronized) {
    if (numberOfUnsynchronized == 0)
      return;

    alohalytics::Stats::Instance().LogEvent(
        "UGC_unsent", {{"num", strings::to_string(numberOfUnsynchronized)},
                       {"is_authenticated", strings::to_string(m_user.IsAuthenticated())}});
  });
}

void Framework::InitSearchAPI()
{
  ASSERT(!m_searchAPI.get(), ("InitSearchAPI() must be called only once."));
  ASSERT(m_infoGetter.get(), ());
  try
  {
    m_searchAPI =
        make_unique<SearchAPI>(m_model.GetDataSource(), m_storage, *m_infoGetter,
                               static_cast<SearchAPI::Delegate &>(*this));
  }
  catch (RootException const & e)
  {
    LOG(LCRITICAL, ("Can't load needed resources for SearchAPI:", e.Msg()));
  }
}

void Framework::InitDiscoveryManager()
{
  CHECK(m_searchAPI.get(), ("InitDiscoveryManager() must be called after InitSearchApi()"));
  CHECK(m_cityFinder.get(), ("InitDiscoveryManager() must be called after InitCityFinder()"));

  discovery::Manager::APIs const apis(*m_searchAPI.get(), *m_viatorApi.get(), *m_localsApi.get());
  m_discoveryManager =
      make_unique<discovery::Manager>(m_model.GetDataSource(), *m_cityFinder.get(), apis);
}

void Framework::InitTransliteration()
{
#if defined(OMIM_OS_ANDROID)
  if (!GetPlatform().IsFileExistsByFullPath(GetPlatform().WritableDir() + kICUDataFile))
  {
    try
    {
      ZipFileReader::UnzipFile(GetPlatform().ResourcesDir(),
                               string("assets/") + kICUDataFile,
                               GetPlatform().WritableDir() + kICUDataFile);
    }
    catch (RootException const & e)
    {
      LOG(LWARNING, ("Can't get transliteration data file \"", kICUDataFile, "\", reason:", e.Msg()));
    }
  }
  Transliteration::Instance().Init(GetPlatform().WritableDir());
#else
  Transliteration::Instance().Init(GetPlatform().ResourcesDir());
#endif

  if (!LoadTransliteration())
    Transliteration::Instance().SetMode(Transliteration::Mode::Disabled);
}

storage::TCountryId Framework::GetCountryIndex(m2::PointD const & pt) const
{
  return m_infoGetter->GetRegionCountryId(pt);
}

string Framework::GetCountryName(m2::PointD const & pt) const
{
  storage::CountryInfo info;
  m_infoGetter->GetRegionInfo(pt, info);
  return info.m_name;
}

Framework::DoAfterUpdate Framework::ToDoAfterUpdate() const
{
  if (platform::migrate::NeedMigrate())
    return DoAfterUpdate::Migrate;

  auto const connectionStatus = Platform::ConnectionStatus();
  if (connectionStatus == Platform::EConnectionType::CONNECTION_NONE)
    return DoAfterUpdate::Nothing;

  auto const & s = GetStorage();
  auto const & rootId = s.GetRootId();
  if (!IsEnoughSpaceForUpdate(rootId, s))
    return DoAfterUpdate::Nothing;

  NodeAttrs attrs;
  s.GetNodeAttrs(rootId, attrs);
  TMwmSize const countrySizeInBytes = attrs.m_localMwmSize;

  if (countrySizeInBytes == 0 || attrs.m_status != NodeStatus::OnDiskOutOfDate)
    return DoAfterUpdate::Nothing;

  if (s.IsPossibleToAutoupdate() && connectionStatus == Platform::EConnectionType::CONNECTION_WIFI)
    return DoAfterUpdate::AutoupdateMaps;

  return DoAfterUpdate::AskForUpdateMaps;
}

SearchAPI & Framework::GetSearchAPI()
{
  ASSERT(m_searchAPI != nullptr, ("Search API is not initialized."));
  return *m_searchAPI;
}

SearchAPI const & Framework::GetSearchAPI() const
{
  ASSERT(m_searchAPI != nullptr, ("Search API is not initialized."));
  return *m_searchAPI;
}

search::DisplayedCategories const & Framework::GetDisplayedCategories()
{
  ASSERT(m_displayedCategories, ());
  ASSERT(m_cityFinder, ());

  string city;

  if (auto const position = GetCurrentPosition())
    city = m_cityFinder->GetCityName(*position, StringUtf8Multilang::kEnglishCode);

  // Apply sponsored modifiers.
  if (m_purchase && !m_purchase->IsSubscriptionActive(SubscriptionType::RemoveAds))
  {
    // Add Category modifiers here.
    //std::tuple<Modifier> modifiers(city);
    //base::for_each_in_tuple(modifiers, [&](size_t, SponsoredCategoryModifier & modifier)
    //{
    //  m_displayedCategories->Modify(modifier);
    //});
  }

  return *m_displayedCategories;
}

void Framework::SelectSearchResult(search::Result const & result, bool animation)
{
  place_page::Info info;
  using namespace search;
  int scale;
  switch (result.GetResultType())
  {
  case Result::Type::Feature:
    FillFeatureInfo(result.GetFeatureID(), info);
    scale = GetFeatureViewportScale(info.GetTypes());
    break;

  case Result::Type::LatLon:
    FillPointInfo(result.GetFeatureCenter(), result.GetString(), info);
    scale = scales::GetUpperComfortScale();
    break;

  case Result::Type::SuggestFromFeature:
  case Result::Type::PureSuggest: ASSERT(false, ("Suggests should not be here.")); return;
  }

  if (m_purchase && !m_purchase->IsSubscriptionActive(SubscriptionType::RemoveAds))
    info.SetAdsEngine(m_adsEngine.get());

  SetPlacePageLocation(info);
  m2::PointD const center = info.GetMercator();
  if (m_drapeEngine != nullptr)
    m_drapeEngine->SetModelViewCenter(center, scale, animation, true /* trackVisibleViewport */);

  GetBookmarkManager().SelectionMark().SetPtOrg(center);
  ActivateMapSelection(false, df::SelectionShape::OBJECT_POI, info);
  m_lastTapEvent = MakeTapEvent(center, info.GetID(), TapEvent::Source::Search);
}

void Framework::ShowSearchResult(search::Result const & res, bool animation)
{
  CancelAllSearches();
  StopLocationFollow();

  alohalytics::LogEvent("searchShowResult", {{"pos", strings::to_string(res.GetPositionInResults())},
                                             {"result", res.ToStringForStats()}});
  SelectSearchResult(res, animation);
}

size_t Framework::ShowSearchResults(search::Results const & results)
{
  using namespace search;

  size_t count = results.GetCount();
  switch (count)
  {
  case 1:
    {
      Result const & r = results[0];
      if (!r.IsSuggest())
        ShowSearchResult(r);
      else
        count = 0;
      // do not put break here
    }
  case 0:
    return count;
  }

  FillSearchResultsMarks(true /* clear */, results);

  // Setup viewport according to results.
  m2::AnyRectD viewport = m_currentModelView.GlobalRect();
  m2::PointD const center = viewport.Center();

  double minDistance = numeric_limits<double>::max();
  int minInd = -1;
  for (size_t i = 0; i < count; ++i)
  {
    Result const & r = results[i];
    if (r.HasPoint())
    {
      double const dist = center.SquaredLength(r.GetFeatureCenter());
      if (dist < minDistance)
      {
        minDistance = dist;
        minInd = static_cast<int>(i);
      }
    }
  }

  if (minInd != -1)
  {
    m2::PointD const pt = results[minInd].GetFeatureCenter();

    if (m_currentModelView.isPerspective())
    {
      StopLocationFollow();
      SetViewportCenter(pt);
      return count;
    }

    if (!viewport.IsPointInside(pt))
    {
      viewport.SetSizesToIncludePoint(pt);
      StopLocationFollow();
    }
  }

  // Graphics engine can be recreated (on Android), so we always set up viewport here.
  ShowRect(viewport);

  return count;
}

void Framework::FillSearchResultsMarks(bool clear, search::Results const & results)
{
  FillSearchResultsMarks(results.begin(), results.end(), clear,
                         Framework::SearchMarkPostProcessing());
}

void Framework::FillSearchResultsMarks(search::Results::ConstIter begin,
                                       search::Results::ConstIter end, bool clear,
                                       SearchMarkPostProcessing fn)
{
  auto editSession = GetBookmarkManager().GetEditSession();
  if (clear)
    editSession.ClearGroup(UserMark::Type::SEARCH);
  editSession.SetIsVisible(UserMark::Type::SEARCH, true);

  for (auto it = begin; it != end; ++it)
  {
    auto const & r = *it;
    if (!r.HasPoint())
      continue;

    auto * mark = editSession.CreateUserMark<SearchMarkPoint>(r.GetFeatureCenter());
    auto const isFeature = r.GetResultType() == search::Result::Type::Feature;
    if (isFeature)
      mark->SetFoundFeature(r.GetFeatureID());

    mark->SetMatchedName(r.GetString());

    if (r.m_metadata.m_isSponsoredHotel)
    {
      mark->SetBookingType(isFeature && m_localAdsManager.Contains(r.GetFeatureID()) /* hasLocalAds */);
      mark->SetRating(r.m_metadata.m_hotelRating);
      mark->SetPricing(r.m_metadata.m_hotelPricing);
    }
    else if (isFeature)
    {
      auto product = GetProductInfo(r);
      auto const type = r.GetFeatureType();
      mark->SetFromType(type, m_localAdsManager.Contains(r.GetFeatureID()));
      if (product.m_ugcRating != search::ProductInfo::kInvalidRating)
        mark->SetRating(product.m_ugcRating);
    }

    if (fn)
      fn(*mark);
  }
}

bool Framework::GetDistanceAndAzimut(m2::PointD const & point,
                                     double lat, double lon, double north,
                                     string & distance, double & azimut)
{
#ifdef FIXED_LOCATION
  m_fixedPos.GetLat(lat);
  m_fixedPos.GetLon(lon);
  m_fixedPos.GetNorth(north);
#endif

  double const d = ms::DistanceOnEarth(lat, lon,
                                       MercatorBounds::YToLat(point.y),
                                       MercatorBounds::XToLon(point.x));

  // Distance may be less than 1.0
  UNUSED_VALUE(measurement_utils::FormatDistance(d, distance));

  if (north >= 0.0)
  {
    // We calculate azimut even when distance is very short (d ~ 0),
    // because return value has 2 states (near me or far from me).

    azimut = ang::Azimuth(MercatorBounds::FromLatLon(lat, lon), point, north);

    double const pi2 = 2.0*math::pi;
    if (azimut < 0.0)
      azimut += pi2;
    else if (azimut > pi2)
      azimut -= pi2;
  }

  // This constant and return value is using for arrow/flag choice.
  return (d < 25000.0);
}

void Framework::CreateDrapeEngine(ref_ptr<dp::GraphicsContextFactory> contextFactory, DrapeCreationParams && params)
{
  auto idReadFn = [this](df::MapDataProvider::TReadCallback<FeatureID const> const & fn,
                         m2::RectD const & r,
                         int scale) -> void { m_model.ForEachFeatureID(r, fn, scale); };

  auto featureReadFn = [this](df::MapDataProvider::TReadCallback<FeatureType> const & fn,
                              vector<FeatureID> const & ids) -> void
  {
    m_model.ReadFeatures(fn, ids);
  };

  auto myPositionModeChangedFn = [this](location::EMyPositionMode mode, bool routingActive)
  {
    GetPlatform().RunTask(Platform::Thread::Gui, [this, mode, routingActive]()
    {
      // Deactivate selection (and hide place page) if we return to routing in F&R mode.
      if (routingActive && mode == location::FollowAndRotate)
        DeactivateMapSelection(true /* notifyUI */);

      if (m_myPositionListener != nullptr)
        m_myPositionListener(mode, routingActive);
    });
  };

  auto filterFeatureFn = [this](FeatureType & ft) -> bool
  {
    if (m_purchase && !m_purchase->IsSubscriptionActive(SubscriptionType::RemoveAds))
      return false;
    return PartnerChecker::Instance().IsFakeObject(ft);
  };

  auto overlaysShowStatsFn = [this](list<df::OverlayShowEvent> && events)
  {
    if (events.empty())
      return;

    list<local_ads::Event> statEvents;
    for (auto const & event : events)
    {
      auto const & mwmInfo = event.m_feature.m_mwmId.GetInfo();
      if (!mwmInfo || !m_localAdsManager.Contains(event.m_feature))
        continue;

      statEvents.emplace_back(local_ads::EventType::ShowPoint,
                              mwmInfo->GetVersion(), mwmInfo->GetCountryName(),
                              event.m_feature.m_index, event.m_zoomLevel, event.m_timestamp,
                              MercatorBounds::YToLat(event.m_myPosition.y),
                              MercatorBounds::XToLon(event.m_myPosition.x),
                              static_cast<uint16_t>(event.m_gpsAccuracy));
    }
    m_localAdsManager.GetStatistics().RegisterEvents(std::move(statEvents));
  };

  auto isUGCFn = [this](FeatureID const & id)
  {
    auto const ugc = m_ugcApi->GetLoader().GetUGC(id);
    return !ugc.IsEmpty();
  };
  auto isCountryLoadedByNameFn = bind(&Framework::IsCountryLoadedByName, this, _1);
  auto updateCurrentCountryFn = bind(&Framework::OnUpdateCurrentCountry, this, _1, _2);

  bool allow3d;
  bool allow3dBuildings;
  Load3dMode(allow3d, allow3dBuildings);

  bool const isAutozoomEnabled = LoadAutoZoom();
  bool const trafficEnabled = m_trafficManager.IsEnabled();
  bool const simplifiedTrafficColors = m_trafficManager.HasSimplifiedColorScheme();
  double const fontsScaleFactor = LoadLargeFontsSize() ? kLargeFontsScaleFactor : 1.0;

  df::DrapeEngine::Params p(
      params.m_apiVersion, contextFactory,
      dp::Viewport(0, 0, params.m_surfaceWidth, params.m_surfaceHeight),
      df::MapDataProvider(move(idReadFn), move(featureReadFn), move(filterFeatureFn),
                          move(isCountryLoadedByNameFn), move(updateCurrentCountryFn)),
      params.m_hints, params.m_visualScale, fontsScaleFactor, move(params.m_widgetsInitInfo),
      make_pair(params.m_initialMyPositionState, params.m_hasMyPositionState),
      move(myPositionModeChangedFn), allow3dBuildings, trafficEnabled,
      params.m_isChoosePositionMode, params.m_isChoosePositionMode, GetSelectedFeatureTriangles(),
      m_routingManager.IsRoutingActive() && m_routingManager.IsRoutingFollowing(),
      isAutozoomEnabled, simplifiedTrafficColors, move(overlaysShowStatsFn), move(isUGCFn));

  m_drapeEngine = make_unique_dp<df::DrapeEngine>(move(p));
  m_drapeEngine->SetModelViewListener([this](ScreenBase const & screen)
  {
    GetPlatform().RunTask(Platform::Thread::Gui, [this, screen](){ OnViewportChanged(screen); });
  });
  m_drapeEngine->SetTapEventInfoListener([this](df::TapInfo const & tapInfo) {
    GetPlatform().RunTask(Platform::Thread::Gui, [this, tapInfo]() {
      OnTapEvent({tapInfo, TapEvent::Source::User});
    });
  });
  m_drapeEngine->SetUserPositionListener([this](m2::PointD const & position, bool hasPosition)
  {
    GetPlatform().RunTask(Platform::Thread::Gui, [this, position, hasPosition](){
      OnUserPositionChanged(position, hasPosition);
    });
  });

  OnSize(params.m_surfaceWidth, params.m_surfaceHeight);

  Allow3dMode(allow3d, allow3dBuildings);
  LoadViewport();

  SetVisibleViewport(m2::RectD(0, 0, params.m_surfaceWidth, params.m_surfaceHeight));

  if (m_connectToGpsTrack)
    GpsTracker::Instance().Connect(bind(&Framework::OnUpdateGpsTrackPointsCallback, this, _1, _2));

  GetBookmarkManager().SetDrapeEngine(make_ref(m_drapeEngine));
  m_drapeApi.SetDrapeEngine(make_ref(m_drapeEngine));
  m_routingManager.SetDrapeEngine(make_ref(m_drapeEngine), allow3d);
  m_trafficManager.SetDrapeEngine(make_ref(m_drapeEngine));
  m_transitManager.SetDrapeEngine(make_ref(m_drapeEngine));
  m_localAdsManager.SetDrapeEngine(make_ref(m_drapeEngine));
  m_searchMarks.SetDrapeEngine(make_ref(m_drapeEngine));

  InvalidateUserMarks();

  bool const transitSchemeEnabled = LoadTransitSchemeEnabled();
  m_transitManager.EnableTransitSchemeMode(transitSchemeEnabled);

  benchmark::RunGraphicsBenchmark(this);
}

void Framework::OnRecoverGLContext(int width, int height)
{
  if (m_drapeEngine)
  {
    m_drapeEngine->Update(width, height);

    InvalidateUserMarks();

    UpdatePlacePageInfoForCurrentSelection();

    m_drapeApi.Invalidate();
  }

  m_trafficManager.OnRecoverGLContext();
  m_transitManager.Invalidate();
  m_localAdsManager.Invalidate();
}

void Framework::OnDestroyGLContext()
{
  m_trafficManager.OnDestroyGLContext();
}

ref_ptr<df::DrapeEngine> Framework::GetDrapeEngine()
{
  return make_ref(m_drapeEngine);
}

void Framework::DestroyDrapeEngine()
{
  if (m_drapeEngine != nullptr)
  {
    m_drapeApi.SetDrapeEngine(nullptr);
    m_routingManager.SetDrapeEngine(nullptr, false);
    m_trafficManager.SetDrapeEngine(nullptr);
    m_transitManager.SetDrapeEngine(nullptr);
    m_localAdsManager.SetDrapeEngine(nullptr);
    m_searchMarks.SetDrapeEngine(nullptr);
    GetBookmarkManager().SetDrapeEngine(nullptr);

    m_trafficManager.Teardown();
    GpsTracker::Instance().Disconnect();
    m_drapeEngine.reset();
  }
}

void Framework::SetRenderingEnabled(ref_ptr<dp::GraphicsContextFactory> contextFactory)
{
  m_isRenderingEnabled = true;
  if (m_drapeEngine)
    m_drapeEngine->SetRenderingEnabled(contextFactory);
}

void Framework::SetRenderingDisabled(bool destroyContext)
{
  m_isRenderingEnabled = false;
  if (m_drapeEngine)
    m_drapeEngine->SetRenderingDisabled(destroyContext);
}

void Framework::EnableDebugRectRendering(bool enabled)
{
  if (m_drapeEngine)
    m_drapeEngine->EnableDebugRectRendering(enabled);
}

void Framework::ConnectToGpsTracker()
{
  m_connectToGpsTrack = true;
  if (m_drapeEngine)
  {
    m_drapeEngine->ClearGpsTrackPoints();
    GpsTracker::Instance().Connect(bind(&Framework::OnUpdateGpsTrackPointsCallback, this, _1, _2));
  }
}

void Framework::DisconnectFromGpsTracker()
{
  m_connectToGpsTrack = false;
  GpsTracker::Instance().Disconnect();
  if (m_drapeEngine)
    m_drapeEngine->ClearGpsTrackPoints();
}

void Framework::OnUpdateGpsTrackPointsCallback(vector<pair<size_t, location::GpsTrackInfo>> && toAdd,
                                               pair<size_t, size_t> const & toRemove)
{
  ASSERT(m_drapeEngine.get() != nullptr, ());

  vector<df::GpsTrackPoint> pointsAdd;
  pointsAdd.reserve(toAdd.size());
  for (auto const & ip : toAdd)
  {
    df::GpsTrackPoint pt;
    ASSERT_LESS(ip.first, static_cast<size_t>(numeric_limits<uint32_t>::max()), ());
    pt.m_id = static_cast<uint32_t>(ip.first);
    pt.m_speedMPS = ip.second.m_speed;
    pt.m_timestamp = ip.second.m_timestamp;
    pt.m_point = MercatorBounds::FromLatLon(ip.second.m_latitude, ip.second.m_longitude);
    pointsAdd.emplace_back(pt);
  }

  vector<uint32_t> indicesRemove;
  if (toRemove.first != GpsTrack::kInvalidId)
  {
    ASSERT_LESS_OR_EQUAL(toRemove.first, toRemove.second, ());
    indicesRemove.reserve(toRemove.second - toRemove.first + 1);
    for (size_t i = toRemove.first; i <= toRemove.second; ++i)
      indicesRemove.emplace_back(i);
  }

  m_drapeEngine->UpdateGpsTrackPoints(move(pointsAdd), move(indicesRemove));
}

void Framework::MarkMapStyle(MapStyle mapStyle)
{
  ASSERT_NOT_EQUAL(mapStyle, MapStyle::MapStyleMerged, ());

  // Store current map style before classificator reloading
  string mapStyleStr = MapStyleToString(mapStyle);
  if (mapStyleStr.empty())
  {
    mapStyle = kDefaultMapStyle;
    mapStyleStr = MapStyleToString(mapStyle);
  }
  settings::Set(kMapStyleKey, mapStyleStr);
  GetStyleReader().SetCurrentStyle(mapStyle);

  alohalytics::TStringMap details {{"mapStyle", mapStyleStr}};
  alohalytics::Stats::Instance().LogEvent("MapStyle_Changed", details);
}

void Framework::SetMapStyle(MapStyle mapStyle)
{
  MarkMapStyle(mapStyle);
  if (m_drapeEngine != nullptr)
    m_drapeEngine->UpdateMapStyle();
  InvalidateUserMarks();
  m_localAdsManager.Invalidate();
  UpdateMinBuildingsTapZoom();
}

MapStyle Framework::GetMapStyle() const
{
  return GetStyleReader().GetCurrentStyle();
}

void Framework::SetupMeasurementSystem()
{
  GetPlatform().SetupMeasurementSystem();

  auto units = measurement_utils::Units::Metric;
  settings::TryGet(settings::kMeasurementUnits, units);

  m_routingManager.SetTurnNotificationsUnits(units);
}

void Framework::SetWidgetLayout(gui::TWidgetsLayoutInfo && layout)
{
  ASSERT(m_drapeEngine != nullptr, ());
  m_drapeEngine->SetWidgetLayout(move(layout));
}

bool Framework::ShowMapForURL(string const & url)
{
  m2::PointD point;
  m2::RectD rect;
  string name;
  ApiMarkPoint const * apiMark = nullptr;

  enum ResultT { FAILED, NEED_CLICK, NO_NEED_CLICK };
  ResultT result = FAILED;

  using namespace url_scheme;
  using namespace strings;

  if (StartsWith(url, "ge0"))
  {
    Ge0Parser parser;
    double zoom;
    ApiPoint pt;

    if (parser.Parse(url, pt, zoom))
    {
      point = MercatorBounds::FromLatLon(pt.m_lat, pt.m_lon);
      rect = df::GetRectForDrawScale(zoom, point);
      name = pt.m_name;
      result = NEED_CLICK;
    }
  }
  else if (m_ParsedMapApi.IsValid())
  {
    if (!m_ParsedMapApi.GetViewportRect(rect))
      rect = df::GetWorldRect();

    apiMark = m_ParsedMapApi.GetSinglePoint();
    result = apiMark ? NEED_CLICK : NO_NEED_CLICK;
  }
  else  // Actually, we can parse any geo url scheme with correct coordinates.
  {
    Info info;
    ParseGeoURL(url, info);
    if (info.IsValid())
    {
      point = MercatorBounds::FromLatLon(info.m_lat, info.m_lon);
      rect = df::GetRectForDrawScale(info.m_zoom, point);
      result = NEED_CLICK;
    }
  }

  if (result != FAILED)
  {
    // Always hide current map selection.
    DeactivateMapSelection(true /* notifyUI */);

    // set viewport and stop follow mode if any
    StopLocationFollow();
    ShowRect(rect);

    if (result != NO_NEED_CLICK)
    {
      place_page::Info info;
      if (apiMark)
      {
        FillApiMarkInfo(*apiMark, info);
        ActivateMapSelection(false, df::SelectionShape::OBJECT_USER_MARK, info);
      }
      else
      {
        GetBookmarkManager().SelectionMark().SetPtOrg(point);
        FillPointInfo(point, name, info);
        ActivateMapSelection(false, df::SelectionShape::OBJECT_POI, info);
      }
      m_lastTapEvent = MakeTapEvent(info.GetMercator(), info.GetID(), TapEvent::Source::Other);
    }

    return true;
  }

  return false;
}

url_scheme::ParsedMapApi::ParsingResult Framework::ParseAndSetApiURL(string const & url)
{
  using namespace url_scheme;

  // Clear every current API-mark.
  {
    auto editSession = GetBookmarkManager().GetEditSession();
    editSession.ClearGroup(UserMark::Type::API);
    editSession.SetIsVisible(UserMark::Type::API, true);
  }

  return m_ParsedMapApi.SetUriAndParse(url);
}

Framework::ParsedRoutingData Framework::GetParsedRoutingData() const
{
  return Framework::ParsedRoutingData(m_ParsedMapApi.GetRoutePoints(),
                                      routing::FromString(m_ParsedMapApi.GetRoutingType()));
}

url_scheme::SearchRequest Framework::GetParsedSearchRequest() const
{
  return m_ParsedMapApi.GetSearchRequest();
}

unique_ptr<FeatureType> Framework::GetFeatureAtPoint(m2::PointD const & mercator) const
{
  unique_ptr<FeatureType> poi, line, area;
  uint32_t const coastlineType = classif().GetCoastType();
  indexer::ForEachFeatureAtPoint(m_model.GetDataSource(), [&, coastlineType](FeatureType & ft)
  {
    // TODO @alexz
    // remove manual parsing after refactoring with usermarks'll be finished
    ft.ParseEverything();
    switch (ft.GetFeatureType())
    {
    case feature::GEOM_POINT:
      poi.reset(new FeatureType(ft));
      break;
    case feature::GEOM_LINE:
      line.reset(new FeatureType(ft));
      break;
    case feature::GEOM_AREA:
      // Buildings have higher priority over other types.
      if (area && ftypes::IsBuildingChecker::Instance()(*area))
        return;
      // Skip/ignore coastlines.
      if (feature::TypesHolder(ft).Has(coastlineType))
        return;
      area.reset(new FeatureType(ft));
      break;
    case feature::GEOM_UNDEFINED:
      ASSERT(false, ("case feature::GEOM_UNDEFINED"));
      break;
    }
  }, mercator);
  return poi ? move(poi) : (area ? move(area) : move(line));
}

bool Framework::GetFeatureByID(FeatureID const & fid, FeatureType & ft) const
{
  ASSERT(fid.IsValid(), ());

  FeaturesLoaderGuard guard(m_model.GetDataSource(), fid.m_mwmId);
  if (!guard.GetFeatureByIndex(fid.m_index, ft))
    return false;

  ft.ParseEverything();
  return true;
}

BookmarkManager & Framework::GetBookmarkManager()
{
  ASSERT(m_bmManager != nullptr, ("Bookmark manager is not initialized."));
  return *m_bmManager.get();
}

BookmarkManager const & Framework::GetBookmarkManager() const
{
  ASSERT(m_bmManager != nullptr, ("Bookmark manager is not initialized."));
  return *m_bmManager.get();
}

void Framework::SetMapSelectionListeners(TActivateMapSelectionFn const & activator,
                                         TDeactivateMapSelectionFn const & deactivator)
{
  m_activateMapSelectionFn = activator;
  m_deactivateMapSelectionFn = deactivator;
}

void Framework::ActivateMapSelection(bool needAnimation, df::SelectionShape::ESelectedObject selectionType,
                                     place_page::Info const & info)
{
  ASSERT_NOT_EQUAL(selectionType, df::SelectionShape::OBJECT_EMPTY, ("Empty selections are impossible."));
  m_selectedFeature = info.GetID();
  if (m_drapeEngine != nullptr)
    m_drapeEngine->SelectObject(selectionType, info.GetMercator(), info.GetID(), needAnimation);

  SetDisplacementMode(DisplacementModeManager::SLOT_MAP_SELECTION,
                      ftypes::IsHotelChecker::Instance()(info.GetTypes()) /* show */);

  if (m_activateMapSelectionFn)
    m_activateMapSelectionFn(info);
  else
    LOG(LWARNING, ("m_activateMapSelectionFn has not been set up."));
}

void Framework::DeactivateMapSelection(bool notifyUI)
{
  bool const somethingWasAlreadySelected = (m_lastTapEvent != nullptr);
  m_lastTapEvent.reset();

  if (notifyUI && m_deactivateMapSelectionFn)
    m_deactivateMapSelectionFn(!somethingWasAlreadySelected);

  if (somethingWasAlreadySelected && m_drapeEngine != nullptr)
    m_drapeEngine->DeselectObject();

  SetDisplacementMode(DisplacementModeManager::SLOT_MAP_SELECTION, false /* show */);
}

void Framework::UpdatePlacePageInfoForCurrentSelection()
{
  if (m_lastTapEvent == nullptr)
    return;

  place_page::Info info;

  auto const obj = OnTapEventImpl(*m_lastTapEvent, info);

  if (obj == df::SelectionShape::OBJECT_EMPTY)
    return;

  SetPlacePageLocation(info);

  ActivateMapSelection(false, obj, info);
}

void Framework::InvalidateUserMarks()
{
  GetBookmarkManager().GetEditSession();
}

void Framework::OnTapEvent(TapEvent const & tapEvent)
{
  using place_page::SponsoredType;
  auto const & tapInfo = tapEvent.m_info;

  bool const somethingWasAlreadySelected = (m_lastTapEvent != nullptr);

  place_page::Info info;
  df::SelectionShape::ESelectedObject const selection = OnTapEventImpl(tapEvent, info);
  if (selection != df::SelectionShape::OBJECT_EMPTY)
  {
    // Back up last tap event to recover selection in case of Drape reinitialization.
    m_lastTapEvent = make_unique<TapEvent>(tapEvent);

    { // Log statistics event.
      auto const ll = info.GetLatLon();
      double metersToTap = -1;
      if (info.IsMyPosition())
      {
        metersToTap = 0;
      }
      else if (auto const position = GetCurrentPosition())
      {
        auto const tapPoint = MercatorBounds::FromLatLon(ll);
        metersToTap = MercatorBounds::DistanceOnEarth(*position, tapPoint);
      }

      alohalytics::TStringMap kv = {{"longTap", tapInfo.m_isLong ? "1" : "0"},
                                    {"title", info.GetTitle()},
                                    {"bookmark", info.IsBookmark() ? "1" : "0"},
                                    {"meters", strings::to_string_dac(metersToTap, 0)}};
      if (info.IsFeature())
        kv["types"] = DebugPrint(info.GetTypes());

      if (info.GetSponsoredType() == SponsoredType::Holiday)
      {
        kv["holiday"] = "1";
        auto const & mwmInfo = info.GetID().m_mwmId.GetInfo();
        if (mwmInfo)
          kv["mwmVersion"] = strings::to_string(mwmInfo->GetVersion());
      }
      else if (info.GetSponsoredType() == SponsoredType::Partner)
      {
        if (!info.GetPartnerName().empty())
          kv["partner"] = info.GetPartnerName();
      }

      // Older version of statistics used "$GetUserMark" event.
      alohalytics::Stats::Instance().LogEvent("$SelectMapObject", kv,
                                              alohalytics::Location::FromLatLon(ll.lat, ll.lon));

      if (info.GetSponsoredType() == SponsoredType::Booking)
      {
        GetPlatform().GetMarketingService().SendPushWooshTag(marketing::kBookHotelOnBookingComDiscovered);
        GetPlatform().GetMarketingService().SendMarketingEvent(marketing::kPlacepageHotelBook,
                                                               {{"provider", "booking.com"}});
      }
    }

    SetPlacePageLocation(info);

    ActivateMapSelection(true, selection, info);
  }
  else
  {
    alohalytics::Stats::Instance().LogEvent(somethingWasAlreadySelected ? "$DelectMapObject" : "$EmptyTapOnMap");
    // UI is always notified even if empty map is tapped,
    // because empty tap event switches on/off full screen map view mode.
    DeactivateMapSelection(true /* notifyUI */);
  }
}

void Framework::InvalidateRendering()
{
  if (m_drapeEngine)
    m_drapeEngine->Invalidate();
}

void Framework::UpdateMinBuildingsTapZoom()
{
  constexpr int kMinTapZoom = 16;
  m_minBuildingsTapZoom = max(kMinTapZoom,
                              feature::GetDrawableScaleRange(classif().GetTypeByPath({"building"})).first);
}

FeatureID Framework::FindBuildingAtPoint(m2::PointD const & mercator) const
{
  FeatureID featureId;
  if (GetDrawScale() >= m_minBuildingsTapZoom)
  {
    constexpr int kScale = scales::GetUpperScale();
    constexpr double kSelectRectWidthInMeters = 1.1;
    m2::RectD const rect = MercatorBounds::RectByCenterXYAndSizeInMeters(mercator, kSelectRectWidthInMeters);
    m_model.ForEachFeature(rect, [&](FeatureType & ft)
    {
      if (!featureId.IsValid() &&
          ft.GetFeatureType() == feature::GEOM_AREA &&
          ftypes::IsBuildingChecker::Instance()(ft) &&
          ft.GetLimitRect(kScale).IsPointInside(mercator) &&
          feature::GetMinDistanceMeters(ft, mercator) == 0.0)
      {
        featureId = ft.GetID();
      }
    }, kScale);
  }
  return featureId;
}

df::SelectionShape::ESelectedObject Framework::OnTapEventImpl(TapEvent const & tapEvent,
                                                              place_page::Info & outInfo)
{
  if (m_drapeEngine == nullptr)
    return df::SelectionShape::OBJECT_EMPTY;

  auto const & tapInfo = tapEvent.m_info;

  if (tapInfo.m_isMyPositionTapped)
  {
    FillMyPositionInfo(outInfo, tapInfo);
    return df::SelectionShape::OBJECT_MY_POSITION;
  }

  if (m_purchase && !m_purchase->IsSubscriptionActive(SubscriptionType::RemoveAds))
    outInfo.SetAdsEngine(m_adsEngine.get());

  UserMark const * mark = FindUserMarkInTapPosition(tapInfo);
  if (mark != nullptr)
  {
    switch (mark->GetMarkType())
    {
    case UserMark::Type::API:
      FillApiMarkInfo(*static_cast<ApiMarkPoint const *>(mark), outInfo);
      break;
    case UserMark::Type::BOOKMARK:
      FillBookmarkInfo(*static_cast<Bookmark const *>(mark), outInfo);
      break;
    case UserMark::Type::SEARCH:
      FillSearchResultInfo(*static_cast<SearchMarkPoint const *>(mark), outInfo);
      break;
    case UserMark::Type::ROUTING:
      FillRouteMarkInfo(*static_cast<RouteMarkPoint const *>(mark), outInfo);
      break;
    default:
      ASSERT(false, ("FindNearestUserMark returned invalid mark."));
    }
    return df::SelectionShape::OBJECT_USER_MARK;
  }

  FeatureID featureTapped = tapInfo.m_featureTapped;

  if (!featureTapped.IsValid())
    featureTapped = FindBuildingAtPoint(tapInfo.m_mercator);

  bool showMapSelection = false;
  if (featureTapped.IsValid())
  {
    FillFeatureInfo(featureTapped, outInfo);
    showMapSelection = true;
  }
  else if (tapInfo.m_isLong || tapEvent.m_source == TapEvent::Source::Search)
  {
    FillPointInfo(tapInfo.m_mercator, {} /* customTitle */, outInfo);
    showMapSelection = true;
  }

  if (showMapSelection)
  {
    GetBookmarkManager().SelectionMark().SetPtOrg(outInfo.GetMercator());

    auto const userPos = GetCurrentPosition();
    if (userPos)
    {
      auto const mapObject = utils::MakeEyeMapObject(outInfo);
      if (!mapObject.IsEmpty())
      {
        eye::Eye::Event::MapObjectEvent(mapObject, eye::MapObject::Event::Type::Open,
                                        userPos.get());
      }
    }

    return df::SelectionShape::OBJECT_POI;
  }

  return df::SelectionShape::OBJECT_EMPTY;
}

UserMark const * Framework::FindUserMarkInTapPosition(df::TapInfo const & tapInfo) const
{
  UserMark const * mark = GetBookmarkManager().FindNearestUserMark([this, &tapInfo](UserMark::Type type)
  {
    if (type == UserMark::Type::BOOKMARK)
      return tapInfo.GetBookmarkSearchRect(m_currentModelView);
    if (type == UserMark::Type::ROUTING)
      return tapInfo.GetRoutingPointSearchRect(m_currentModelView);
    return tapInfo.GetDefaultSearchRect(m_currentModelView);
  });
  return mark;
}

unique_ptr<Framework::TapEvent> Framework::MakeTapEvent(m2::PointD const & center,
                                                        FeatureID const & fid,
                                                        TapEvent::Source source) const
{
  return make_unique<TapEvent>(df::TapInfo{center, false, false, fid}, source);
}

void Framework::PredictLocation(double & lat, double & lon, double accuracy,
                                double bearing, double speed, double elapsedSeconds)
{
  double offsetInM = speed * elapsedSeconds;
  double angle = base::DegToRad(90.0 - bearing);

  m2::PointD mercatorPt = MercatorBounds::MetersToXY(lon, lat, accuracy).Center();
  mercatorPt = MercatorBounds::GetSmPoint(mercatorPt, offsetInM * cos(angle), offsetInM * sin(angle));
  lon = MercatorBounds::XToLon(mercatorPt.x);
  lat = MercatorBounds::YToLat(mercatorPt.y);
}

StringsBundle const & Framework::GetStringsBundle()
{
  return m_stringsBundle;
}

string Framework::CodeGe0url(Bookmark const * bmk, bool addName)
{
  double lat = MercatorBounds::YToLat(bmk->GetPivot().y);
  double lon = MercatorBounds::XToLon(bmk->GetPivot().x);
  return CodeGe0url(lat, lon, bmk->GetScale(), addName ? bmk->GetPreferredName() : "");
}

string Framework::CodeGe0url(double lat, double lon, double zoomLevel, string const & name)
{
  size_t const resultSize = MapsWithMe_GetMaxBufferSize(static_cast<int>(name.size()));

  string res(resultSize, 0);
  int const len = MapsWithMe_GenShortShowMapUrl(lat, lon, zoomLevel, name.c_str(), &res[0],
                                                static_cast<int>(res.size()));

  ASSERT_LESS_OR_EQUAL(len, static_cast<int>(res.size()), ());
  res.resize(len);

  return res;
}

string Framework::GenerateApiBackUrl(ApiMarkPoint const & point) const
{
  string res = m_ParsedMapApi.GetGlobalBackUrl();
  if (!res.empty())
  {
    ms::LatLon const ll = point.GetLatLon();
    res += "pin?ll=" + strings::to_string(ll.lat) + "," + strings::to_string(ll.lon);
    if (!point.GetName().empty())
      res += "&n=" + UrlEncode(point.GetName());
    if (!point.GetApiID().empty())
      res += "&id=" + UrlEncode(point.GetApiID());
  }
  return res;
}

bool Framework::IsDataVersionUpdated()
{
  int64_t storedVersion;
  if (settings::Get("DataVersion", storedVersion))
  {
    return storedVersion < m_storage.GetCurrentDataVersion();
  }
  // no key in the settings, assume new version
  return true;
}

void Framework::UpdateSavedDataVersion()
{
  settings::Set("DataVersion", m_storage.GetCurrentDataVersion());
}

int64_t Framework::GetCurrentDataVersion() const { return m_storage.GetCurrentDataVersion(); }

#if defined(OMIM_METAL_AVAILABLE)
bool Framework::LoadMetalAllowed()
{
  bool allowed;
  if (settings::Get(kMetalAllowed, allowed))
    return allowed;
  return true;
}

void Framework::SaveMetalAllowed(bool allowed)
{
  settings::Set(kMetalAllowed, allowed);
}
#endif

void Framework::AllowTransliteration(bool allowTranslit)
{
  Transliteration::Instance().SetMode(allowTranslit ? Transliteration::Mode::Enabled
                                                    : Transliteration::Mode::Disabled);
  InvalidateRect(GetCurrentViewport());
}

bool Framework::LoadTransliteration()
{
  Transliteration::Mode mode;
  if (settings::Get(kTranslitMode, mode))
    return mode == Transliteration::Mode::Enabled;
  return true;
}

void Framework::SaveTransliteration(bool allowTranslit)
{
  settings::Set(kTranslitMode, allowTranslit ? Transliteration::Mode::Enabled
                                             : Transliteration::Mode::Disabled);
}

void Framework::Allow3dMode(bool allow3d, bool allow3dBuildings)
{
  if (m_drapeEngine != nullptr)
    m_drapeEngine->Allow3dMode(allow3d, allow3dBuildings);
}

void Framework::Save3dMode(bool allow3d, bool allow3dBuildings)
{
  settings::Set(kAllow3dKey, allow3d);
  settings::Set(kAllow3dBuildingsKey, allow3dBuildings);
}

void Framework::Load3dMode(bool & allow3d, bool & allow3dBuildings)
{
  if (!settings::Get(kAllow3dKey, allow3d))
    allow3d = true;

  if (!settings::Get(kAllow3dBuildingsKey, allow3dBuildings))
    allow3dBuildings = true;
}

void Framework::SaveLargeFontsSize(bool isLargeSize)
{
  settings::Set(kLargeFontsSize, isLargeSize);
}

bool Framework::LoadLargeFontsSize()
{
  bool isLargeSize;
  if (!settings::Get(kLargeFontsSize, isLargeSize))
    isLargeSize = false;
  return isLargeSize;
}

void Framework::SetLargeFontsSize(bool isLargeSize)
{
  double const scaleFactor = isLargeSize ? kLargeFontsScaleFactor : 1.0;

  ASSERT(m_drapeEngine.get() != nullptr, ());
  m_drapeEngine->SetFontScaleFactor(scaleFactor);

  InvalidateRect(GetCurrentViewport());
}

bool Framework::LoadTrafficEnabled()
{
  bool enabled;
  if (!settings::Get(kTrafficEnabledKey, enabled))
    enabled = false;
  return enabled;
}

void Framework::SaveTrafficEnabled(bool trafficEnabled)
{
  settings::Set(kTrafficEnabledKey, trafficEnabled);
}

bool Framework::LoadTrafficSimplifiedColors()
{
  bool simplified;
  if (!settings::Get(kTrafficSimplifiedColorsKey, simplified))
    simplified = true;
  return simplified;
}

void Framework::SaveTrafficSimplifiedColors(bool simplified)
{
  settings::Set(kTrafficSimplifiedColorsKey, simplified);
}

bool Framework::LoadAutoZoom()
{
  bool allowAutoZoom;
  if (!settings::Get(kAllowAutoZoom, allowAutoZoom))
    allowAutoZoom = true;
  return allowAutoZoom;
}

void Framework::AllowAutoZoom(bool allowAutoZoom)
{
  routing::RouterType const type = m_routingManager.GetRouter();
  bool const isPedestrianRoute = type == RouterType::Pedestrian;
  bool const isTaxiRoute = type == RouterType::Taxi;

  if (m_drapeEngine != nullptr)
    m_drapeEngine->AllowAutoZoom(allowAutoZoom && !isPedestrianRoute && !isTaxiRoute);
}

void Framework::SaveAutoZoom(bool allowAutoZoom)
{
  settings::Set(kAllowAutoZoom, allowAutoZoom);
}

bool Framework::LoadTransitSchemeEnabled()
{
  bool enabled;
  if (!settings::Get(kTransitSchemeEnabledKey, enabled))
    enabled = false;
  return enabled;
}

void Framework::SaveTransitSchemeEnabled(bool enabled)
{
  settings::Set(kTransitSchemeEnabledKey, enabled);
}

void Framework::EnableChoosePositionMode(bool enable, bool enableBounds, bool applyPosition, m2::PointD const & position)
{
  if (m_drapeEngine != nullptr)
    m_drapeEngine->EnableChoosePositionMode(enable, enableBounds ? GetSelectedFeatureTriangles() : vector<m2::TriangleD>(),
                                            applyPosition, position);
}

discovery::Manager::Params Framework::GetDiscoveryParams(
    discovery::ClientParams && clientParams) const
{
  auto constexpr kRectSideM = 2000.0;
  discovery::Manager::Params p;
  p.m_viewportCenter = GetDiscoveryViewportCenter();
  p.m_viewport = MercatorBounds::RectByCenterXYAndSizeInMeters(p.m_viewportCenter, kRectSideM);
  p.m_curency = clientParams.m_currency;
  p.m_lang = clientParams.m_lang;
  p.m_itemsCount = clientParams.m_itemsCount;
  p.m_itemTypes = move(clientParams.m_itemTypes);
  return p;
}

std::string Framework::GetDiscoveryViatorUrl() const
{
  return m_discoveryManager->GetViatorUrl(GetDiscoveryViewportCenter());
}

std::string Framework::GetDiscoveryLocalExpertsUrl() const
{
  return m_discoveryManager->GetLocalExpertsUrl(GetDiscoveryViewportCenter());
}

m2::PointD Framework::GetDiscoveryViewportCenter() const
{
  auto const currentPosition = GetCurrentPosition();
  return currentPosition ? *currentPosition : GetViewportCenter();
}

vector<m2::TriangleD> Framework::GetSelectedFeatureTriangles() const
{
  vector<m2::TriangleD> triangles;
  if (!m_selectedFeature.IsValid())
    return triangles;

  FeaturesLoaderGuard const guard(m_model.GetDataSource(), m_selectedFeature.m_mwmId);
  FeatureType ft;
  if (!guard.GetFeatureByIndex(m_selectedFeature.m_index, ft))
    return triangles;

  if (ftypes::IsBuildingChecker::Instance()(feature::TypesHolder(ft)))
  {
    triangles.reserve(10);
    ft.ForEachTriangle([&](m2::PointD const & p1, m2::PointD const & p2, m2::PointD const & p3)
    {
      triangles.emplace_back(p1, p2, p3);
    }, scales::GetUpperScale());
  }
  m_selectedFeature = FeatureID();

  return triangles;
}

void Framework::BlockTapEvents(bool block)
{
  if (m_drapeEngine != nullptr)
    m_drapeEngine->BlockTapEvents(block);
}

namespace feature
{
string GetPrintableTypes(FeatureType & ft) { return DebugPrint(feature::TypesHolder(ft)); }
uint32_t GetBestType(FeatureType & ft) { return feature::TypesHolder(ft).GetBestType(); }
}

bool Framework::ParseDrapeDebugCommand(string const & query)
{
  MapStyle desiredStyle = MapStyleCount;
  if (query == "?dark" || query == "mapstyle:dark")
    desiredStyle = MapStyleDark;
  else if (query == "?light" || query == "mapstyle:light")
    desiredStyle = MapStyleClear;
  else if (query == "?vdark" || query == "mapstyle:vehicle_dark")
    desiredStyle = MapStyleVehicleDark;
  else if (query == "?vlight" || query == "mapstyle:vehicle_light")
    desiredStyle = MapStyleVehicleClear;

  if (desiredStyle != MapStyleCount)
  {
#if defined(OMIM_OS_ANDROID)
    MarkMapStyle(desiredStyle);
#else
    SetMapStyle(desiredStyle);
#endif
    return true;
  }

  if (query == "?aa" || query == "effect:antialiasing")
  {
    m_drapeEngine->SetPosteffectEnabled(df::PostprocessRenderer::Antialiasing,
                                        true /* enabled */);
    return true;
  }
  if (query == "?no-aa" || query == "effect:no-antialiasing")
  {
    m_drapeEngine->SetPosteffectEnabled(df::PostprocessRenderer::Antialiasing,
                                        false /* enabled */);
    return true;
  }
  if (query == "?ugc")
  {
    m_drapeEngine->EnableUGCRendering(true /* enabled */);
    return true;
  }
  if (query == "?no-ugc")
  {
    m_drapeEngine->EnableUGCRendering(false /* enabled */);
    return true;
  }
  if (query == "?scheme")
  {
    m_transitManager.EnableTransitSchemeMode(true /* enable */);
    return true;
  }
  if (query == "?no-scheme")
  {
    m_transitManager.EnableTransitSchemeMode(false /* enable */);
    return true;
  }
  if (query == "?debug-info")
  {
    m_drapeEngine->ShowDebugInfo(true /* shown */);
    return true;
  }
  if (query == "?no-debug-info")
  {
    m_drapeEngine->ShowDebugInfo(false /* shown */);
    return true;
  }
  if (query == "?debug-rect")
  {
    m_drapeEngine->EnableDebugRectRendering(true /* shown */);
    return true;
  }
  if (query == "?no-debug-rect")
  {
    m_drapeEngine->EnableDebugRectRendering(false /* shown */);
    return true;
  }
#if defined(OMIM_METAL_AVAILABLE)
  if (query == "?metal")
  {
    SaveMetalAllowed(true);
    return true;
  }
  if (query == "?gl")
  {
    SaveMetalAllowed(false);
    return true;
  }
#endif
  return false;
}

bool Framework::ParseEditorDebugCommand(search::SearchParams const & params)
{
  if (params.m_query == "?edits")
  {
    osm::Editor::Stats const stats = osm::Editor::Instance().GetStats();
    search::Results results;
    results.AddResultNoChecks(search::Result("Uploaded: " + strings::to_string(stats.m_uploadedCount), "?edits"));
    for (auto const & edit : stats.m_edits)
    {
      FeatureID const & fid = edit.first;

      FeatureType ft;
      if (!GetFeatureByID(fid, ft))
      {
        LOG(LERROR, ("Feature can't be loaded:", fid));
        return true;
      }

      string name;
      ft.GetReadableName(name);
      feature::TypesHolder const types(ft);
      search::Result::Metadata smd;
      results.AddResultNoChecks(
          search::Result(fid, feature::GetCenter(ft), name, edit.second, types.GetBestType(), smd));
    }
    params.m_onResults(results);

    results.SetEndMarker(false /* isCancelled */);
    params.m_onResults(results);
    return true;
  }
  else if (params.m_query == "?eclear")
  {
    osm::Editor::Instance().ClearAllLocalEdits();
    return true;
  }
  return false;
}

bool Framework::ParseRoutingDebugCommand(search::SearchParams const & params)
{
  // This is an example.
  /*
    if (params.m_query == "?speedcams")
    {
      GetRoutingManager().RoutingSession().EnableMyFeature();
      return true;
    }
  */
  return false;
}

namespace
{
WARN_UNUSED_RESULT bool LocalizeStreet(DataSource const & dataSource, FeatureID const & fid,
                                       osm::LocalizedStreet & result)
{
  FeaturesLoaderGuard g(dataSource, fid.m_mwmId);
  FeatureType ft;
  if (!g.GetFeatureByIndex(fid.m_index, ft))
    return false;

  ft.GetName(StringUtf8Multilang::kDefaultCode, result.m_defaultName);
  ft.GetReadableName(result.m_localizedName);
  if (result.m_localizedName == result.m_defaultName)
    result.m_localizedName.clear();
  return true;
}

vector<osm::LocalizedStreet> TakeSomeStreetsAndLocalize(
    vector<search::ReverseGeocoder::Street> const & streets, DataSource const & dataSource)

{
  vector<osm::LocalizedStreet> results;
  // Exact feature street always goes first in Editor UI street list.

  // Reasonable number of different nearby street names to display in UI.
  constexpr size_t kMaxNumberOfNearbyStreetsToDisplay = 8;
  for (auto const & street : streets)
  {
    auto const isDuplicate = find_if(begin(results), end(results),
                                     [&street](osm::LocalizedStreet const & s)
                                     {
                                       return s.m_defaultName == street.m_name;
                                     }) != results.end();
    if (isDuplicate)
      continue;

    osm::LocalizedStreet ls;
    if (!LocalizeStreet(dataSource, street.m_id, ls))
      continue;

    results.emplace_back(move(ls));
    if (results.size() >= kMaxNumberOfNearbyStreetsToDisplay)
      break;
  }
  return results;
}

void SetStreet(search::ReverseGeocoder const & coder, DataSource const & dataSource,
               FeatureType & ft, osm::EditableMapObject & emo)
{
  auto const & editor = osm::Editor::Instance();
  // Get exact feature's street address (if any) from mwm,
  // together with all nearby streets.
  auto const streets = coder.GetNearbyFeatureStreets(ft);
  auto const & streetsPool = streets.first;
  auto const & featureStreetIndex = streets.second;

  string street;
  bool const featureIsInEditor = editor.GetEditedFeatureStreet(ft.GetID(), street);
  bool const featureHasStreetInMwm = featureStreetIndex < streetsPool.size();
  if (!featureIsInEditor && featureHasStreetInMwm)
    street = streetsPool[featureStreetIndex].m_name;

  auto localizedStreets = TakeSomeStreetsAndLocalize(streetsPool, dataSource);

  if (!street.empty())
  {
    auto it = find_if(begin(streetsPool), end(streetsPool),
                      [&street](search::ReverseGeocoder::Street const & s)
                      {
                        return s.m_name == street;
                      });

    if (it != end(streetsPool))
    {
      osm::LocalizedStreet ls;
      if (!LocalizeStreet(dataSource, it->m_id, ls))
        ls.m_defaultName = street;

      emo.SetStreet(ls);

      // A street that a feature belongs to should always be in the first place in the list.
      auto it =
          find_if(begin(localizedStreets), end(localizedStreets), [&ls](osm::LocalizedStreet const & rs)
                  {
                    return ls.m_defaultName == rs.m_defaultName;
                  });
      if (it != end(localizedStreets))
        iter_swap(it, begin(localizedStreets));
      else
        localizedStreets.insert(begin(localizedStreets), ls);
    }
    else
    {
      emo.SetStreet({street, ""});
    }
  }
  else
  {
    emo.SetStreet({});
  }

  emo.SetNearbyStreets(move(localizedStreets));
}

void SetHostingBuildingAddress(FeatureID const & hostingBuildingFid, DataSource const & dataSource,
                               search::ReverseGeocoder const & coder, osm::EditableMapObject & emo)
{
  if (!hostingBuildingFid.IsValid())
    return;

  FeatureType hostingBuildingFeature;

  FeaturesLoaderGuard g(dataSource, hostingBuildingFid.m_mwmId);
  if (!g.GetFeatureByIndex(hostingBuildingFid.m_index, hostingBuildingFeature))
    return;

  search::ReverseGeocoder::Address address;
  if (coder.GetExactAddress(hostingBuildingFeature, address))
  {
    if (emo.GetHouseNumber().empty())
      emo.SetHouseNumber(address.GetHouseNumber());
    if (emo.GetStreet().m_defaultName.empty())
      // TODO(mgsergio): Localize if localization is required by UI.
      emo.SetStreet({address.GetStreetName(), ""});
  }
}
}  // namespace

bool Framework::CanEditMap() const
{
  return version::IsSingleMwm(GetCurrentDataVersion()) && !GetStorage().IsDownloadInProgress();
}

bool Framework::CreateMapObject(m2::PointD const & mercator, uint32_t const featureType,
                                osm::EditableMapObject & emo) const
{
  emo = {};
  auto const & dataSource = m_model.GetDataSource();
  MwmSet::MwmId const mwmId = dataSource.GetMwmIdByCountryFile(
        platform::CountryFile(m_infoGetter->GetRegionCountryId(mercator)));
  if (!mwmId.IsAlive())
    return false;

  GetPlatform().GetMarketingService().SendMarketingEvent(marketing::kEditorAddStart, {});

  search::ReverseGeocoder const coder(m_model.GetDataSource());
  vector<search::ReverseGeocoder::Street> streets;

  coder.GetNearbyStreets(mwmId, mercator, streets);
  emo.SetNearbyStreets(TakeSomeStreetsAndLocalize(streets, m_model.GetDataSource()));

  // TODO(mgsergio): Check emo is a poi. For now it is the only option.
  SetHostingBuildingAddress(FindBuildingAtPoint(mercator), dataSource, coder, emo);

  return osm::Editor::Instance().CreatePoint(featureType, mercator, mwmId, emo);
}

bool Framework::GetEditableMapObject(FeatureID const & fid, osm::EditableMapObject & emo) const
{
  if (!fid.IsValid())
    return false;

  FeatureType ft;
  if (!GetFeatureByID(fid, ft))
    return false;

  GetPlatform().GetMarketingService().SendMarketingEvent(marketing::kEditorEditStart, {});

  emo = {};
  emo.SetFromFeatureType(ft);
  emo.SetHouseNumber(ft.GetHouseNumber());
  auto const & editor = osm::Editor::Instance();
  emo.SetEditableProperties(editor.GetEditableProperties(ft));

  auto const & dataSource = m_model.GetDataSource();
  search::ReverseGeocoder const coder(dataSource);
  SetStreet(coder, dataSource, ft, emo);

  if (!ftypes::IsBuildingChecker::Instance()(ft) &&
      (emo.GetHouseNumber().empty() || emo.GetStreet().m_defaultName.empty()))
  {
    SetHostingBuildingAddress(FindBuildingAtPoint(feature::GetCenter(ft)),
                              dataSource, coder, emo);
  }

  return true;
}

osm::Editor::SaveResult Framework::SaveEditedMapObject(osm::EditableMapObject emo)
{
  if (!m_lastTapEvent)
  {
    // Automatically select newly created objects.
    m_lastTapEvent = MakeTapEvent(emo.GetMercator(), emo.GetID(), TapEvent::Source::Other);
  }

  auto & editor = osm::Editor::Instance();

  ms::LatLon issueLatLon;

  auto shouldNotify = false;
  // Notify if a poi address and it's hosting building address differ.
  do
  {
    auto const isCreatedFeature = editor.IsCreatedFeature(emo.GetID());

    FeaturesLoaderGuard g(m_model.GetDataSource(), emo.GetID().m_mwmId);
    FeatureType originalFeature;
    if (!isCreatedFeature)
    {
      if (!g.GetOriginalFeatureByIndex(emo.GetID().m_index, originalFeature))
        return osm::Editor::SaveResult::NoUnderlyingMapError;
    }
    else
    {
      originalFeature.ReplaceBy(emo);
    }

    // Handle only pois.
    if (ftypes::IsBuildingChecker::Instance()(originalFeature))
      break;

    auto const hostingBuildingFid = FindBuildingAtPoint(feature::GetCenter(originalFeature));
    // The is no building to take address from. Fallback to simple saving.
    if (!hostingBuildingFid.IsValid())
      break;

    FeatureType hostingBuildingFeature;
    if (!g.GetFeatureByIndex(hostingBuildingFid.m_index, hostingBuildingFeature))
      break;

    issueLatLon = MercatorBounds::ToLatLon(feature::GetCenter(hostingBuildingFeature));

    search::ReverseGeocoder::Address hostingBuildingAddress;
    search::ReverseGeocoder const coder(m_model.GetDataSource());
    // The is no address to take from a hosting building. Fallback to simple saving.
    if (!coder.GetExactAddress(hostingBuildingFeature, hostingBuildingAddress))
      break;

    string originalFeatureStreet;
    if (!isCreatedFeature)
    {
      auto const streets = coder.GetNearbyFeatureStreets(originalFeature);
      if (streets.second < streets.first.size())
        originalFeatureStreet = streets.first[streets.second].m_name;
    }
    else
    {
      originalFeatureStreet = emo.GetStreet().m_defaultName;
    }

    auto isStreetOverridden = false;
    if (!hostingBuildingAddress.GetStreetName().empty() &&
        emo.GetStreet().m_defaultName != hostingBuildingAddress.GetStreetName())
    {
      isStreetOverridden = true;
      shouldNotify = true;
    }
    auto isHouseNumberOverridden = false;
    if (!hostingBuildingAddress.GetHouseNumber().empty() &&
        emo.GetHouseNumber() != hostingBuildingAddress.GetHouseNumber())
    {
      isHouseNumberOverridden = true;
      shouldNotify = true;
    }

    // TODO(mgsergio): Consider this:
    // User changed a feature that had no original address and the address he entered
    // is different from the one on the hosting building. He saved it and the changes
    // were uploaded to OSM. Then he set the address from the hosting building back
    // to the feature. As a result this address won't be merged in OSM because emo.SetStreet({})
    // and emo.SetHouseNumber("") will be called in the following code. So OSM ends up
    // with incorrect data.

    // There is (almost) always a street and/or house number set in emo. We must keep them from
    // saving to editor and pushing to OSM if they ware not overidden. To be saved to editor
    // emo is first converted to FeatureType and FeatureType is then saved to a file and editor.
    // To keep street and house number from penetrating to FeatureType we set them to be empty.

    // Do not save street if it was taken from hosting building.
    if ((originalFeatureStreet.empty() || isCreatedFeature) && !isStreetOverridden)
        emo.SetStreet({});
    // Do not save house number if it was taken from hosting building.
    if ((originalFeature.GetHouseNumber().empty() || isCreatedFeature) && !isHouseNumberOverridden)
      emo.SetHouseNumber("");

    if (!isStreetOverridden && !isHouseNumberOverridden)
    {
      // Address was taken from the hosting building of a feature. Nothing to note.
      shouldNotify = false;
      break;
    }

    if (shouldNotify)
    {
      FeatureType editedFeature;
      string editedFeatureStreet;
      // Such a notification have been already sent. I.e at least one of
      // street of house number should differ in emo and editor.
      shouldNotify = !isCreatedFeature &&
                     ((editor.GetEditedFeature(emo.GetID(), editedFeature) &&
                       !editedFeature.GetHouseNumber().empty() &&
                       editedFeature.GetHouseNumber() != emo.GetHouseNumber()) ||
                      (editor.GetEditedFeatureStreet(emo.GetID(), editedFeatureStreet) &&
                       !editedFeatureStreet.empty() &&
                       editedFeatureStreet != emo.GetStreet().m_defaultName));
    }
  } while (0);

  if (shouldNotify)
  {
    // TODO @mgsergio fill with correct NoteProblemType
    editor.CreateNote(emo.GetLatLon(), emo.GetID(), emo.GetTypes(), emo.GetDefaultName(),
                      osm::Editor::NoteProblemType::General,
                      "The address on this POI is different from the building address."
                      " It is either a user's mistake, or an issue in the data. Please"
                      " check this and fix if needed. (This note was created automatically"
                      " without a user's input. Feel free to close it if it's wrong).");
  }

  emo.RemoveNeedlessNames();

  return osm::Editor::Instance().SaveEditedFeature(emo);
}

void Framework::DeleteFeature(FeatureID const & fid) const
{
  osm::Editor::Instance().DeleteFeature(fid);
  if (m_selectedFeature == fid)
    m_selectedFeature = FeatureID();
}

osm::NewFeatureCategories Framework::GetEditorCategories() const
{
  return osm::Editor::Instance().GetNewFeatureCategories();
}

bool Framework::RollBackChanges(FeatureID const & fid)
{
  if (m_selectedFeature == fid) // reset selected feature since it becomes invalid after rollback
    m_selectedFeature = FeatureID();
  auto & editor = osm::Editor::Instance();
  auto const status = editor.GetFeatureStatus(fid);
  auto const rolledBack = editor.RollBackChanges(fid);
  if (rolledBack)
  {
    if (status == FeatureStatus::Created)
      DeactivateMapSelection(true /* notifyUI */);
    else
      UpdatePlacePageInfoForCurrentSelection();
  }
  return rolledBack;
}

void Framework::CreateNote(osm::MapObject const & mapObject,
                           osm::Editor::NoteProblemType const type, string const & note)
{
  osm::Editor::Instance().CreateNote(mapObject.GetLatLon(), mapObject.GetID(), mapObject.GetTypes(),
                                     mapObject.GetDefaultName(), type, note);
  if (type == osm::Editor::NoteProblemType::PlaceDoesNotExist)
    DeactivateMapSelection(true /* notifyUI */);
}

storage::TCountriesVec Framework::GetTopmostCountries(ms::LatLon const & latlon) const
{
  m2::PointD const point = MercatorBounds::FromLatLon(latlon);
  auto const countryId = m_infoGetter->GetRegionCountryId(point);
  storage::TCountriesVec topmostCountryIds;
  GetStorage().GetTopmostNodesFor(countryId, topmostCountryIds);
  return topmostCountryIds;
}

namespace
{
vector<dp::Color> colorList = {
    dp::Color(255, 0, 0, 255),   dp::Color(0, 255, 0, 255),   dp::Color(0, 0, 255, 255),
    dp::Color(255, 255, 0, 255), dp::Color(0, 255, 255, 255), dp::Color(255, 0, 255, 255),
    dp::Color(100, 0, 0, 255),   dp::Color(0, 100, 0, 255),   dp::Color(0, 0, 100, 255),
    dp::Color(100, 100, 0, 255), dp::Color(0, 100, 100, 255), dp::Color(100, 0, 100, 255)};

dp::Color const cityBoundaryBBColor = dp::Color(255, 0, 0, 255);
dp::Color const cityBoundaryCBColor = dp::Color(0, 255, 0, 255);
dp::Color const cityBoundaryDBColor = dp::Color(0, 0, 255, 255);

template <class Box>
void DrawLine(Box const & box, dp::Color const & color, df::DrapeApi & drapeApi, string const & id)
{
  auto points = box.Points();
  CHECK(!points.empty(), ());
  points.push_back(points.front());

  points.erase(unique(points.begin(), points.end(), [](m2::PointD const & p1, m2::PointD const & p2) {
    m2::PointD const delta = p2 - p1;
    return delta.IsAlmostZero();
  }), points.end());

  if (points.size() <= 1)
    return;

  drapeApi.AddLine(id, df::DrapeApiLineData(points, color).Width(3.0f).ShowPoints(true).ShowId());
}

void VisualizeFeatureInRect(m2::RectD const & rect, FeatureType & ft, df::DrapeApi & drapeApi,
                            size_t & counter)
{
  bool allPointsOutside = true;
  vector<m2::PointD> points;
  ft.ForEachPoint([&points, &rect, &allPointsOutside](m2::PointD const & pt)
                  {
                    if (rect.IsPointInside(pt))
                      allPointsOutside = false;
                    points.push_back(pt);
                  }, scales::GetUpperScale());

  if (!allPointsOutside)
  {
    size_t const colorIndex = counter % colorList.size();
    // Note. The first param at DrapeApi::AddLine() should be unique. Other way last added line
    // replaces the previous added line with the same name.
    // As a consequence VisualizeFeatureInRect() should be applied to single mwm. Other way
    // feature ids will be dubbed.
    drapeApi.AddLine(
        strings::to_string(ft.GetID().m_index),
        df::DrapeApiLineData(points, colorList[colorIndex]).Width(3.0f).ShowPoints(true).ShowId());
    counter++;
  }
}
}  // namespace

void Framework::VisualizeRoadsInRect(m2::RectD const & rect)
{
  size_t counter = 0;
  m_model.ForEachFeature(rect, [this, &counter, &rect](FeatureType & ft)
  {
    if (routing::IsRoad(feature::TypesHolder(ft)))
      VisualizeFeatureInRect(rect, ft, m_drapeApi, counter);
  }, scales::GetUpperScale());
}

void Framework::VisualizeCityBoundariesInRect(m2::RectD const & rect)
{
  search::CitiesBoundariesTable table(GetDataSource());
  table.Load();

  vector<uint32_t> featureIds;
  GetCityBoundariesInRectForTesting(table, rect, featureIds);

  FeaturesLoaderGuard loader(GetDataSource(), GetDataSource().GetMwmIdByCountryFile(CountryFile("World")));
  for (auto const fid : featureIds)
  {
    search::CitiesBoundariesTable::Boundaries boundaries;
    table.Get(fid, boundaries);

    string id = "fid:" + strings::to_string(fid);
    FeatureType ft;
    if (loader.GetFeatureByIndex(fid, ft))
    {
      string name;
      ft.GetName(StringUtf8Multilang::kDefaultCode, name);
      id += ", name:" + name;
    }

    size_t const boundariesSize = boundaries.GetBoundariesForTesting().size();
    for (size_t i = 0; i < boundariesSize; ++i)
    {
      string idWithIndex = id;
      auto const & cityBoundary = boundaries.GetBoundariesForTesting()[i];
      if (boundariesSize > 1)
        idWithIndex = id + " , i:" + strings::to_string(i);

      DrawLine(cityBoundary.m_bbox, cityBoundaryBBColor, m_drapeApi, idWithIndex + ", bb");
      DrawLine(cityBoundary.m_cbox, cityBoundaryCBColor, m_drapeApi, idWithIndex + ", cb");
      DrawLine(cityBoundary.m_dbox, cityBoundaryDBColor, m_drapeApi, idWithIndex + ", db");
    }
  }
}

void Framework::VisualizeCityRoadsInRect(m2::RectD const & rect)
{
  map<MwmSet::MwmId, unique_ptr<CityRoads>> cityRoads;
  size_t counter = 0;
  GetDataSource().ForEachInRect(
      [this, &rect, &cityRoads, &counter](FeatureType & ft) {
        if (ft.GetFeatureType() != feature::GEOM_LINE)
          return;

        auto const & mwmId = ft.GetID().m_mwmId;
        auto const it = cityRoads.find(mwmId);
        if (it == cityRoads.cend())
        {
          MwmSet::MwmHandle handle = m_model.GetDataSource().GetMwmHandleById(mwmId);
          if (!handle.IsAlive())
            return;

          cityRoads[mwmId] = LoadCityRoads(GetDataSource(), handle);
        }

        if (!cityRoads[mwmId]->IsCityRoad(ft.GetID().m_index))
          return;  // ft is not a city road.

        VisualizeFeatureInRect(rect, ft, m_drapeApi, counter);
      },
      rect, scales::GetUpperScale());
}

ads::Engine const & Framework::GetAdsEngine() const
{
  ASSERT(m_adsEngine, ());
  return *m_adsEngine;
}

void Framework::DisableAdProvider(ads::Banner::Type const type, ads::Banner::Place const place)
{
  ASSERT(m_adsEngine, ());
  m_adsEngine.get()->DisableAdProvider(type, place);
}

bool Framework::HasRuTaxiCategoryBanner()
{
  auto const & purchase = GetPurchase();
  if (purchase && purchase->IsSubscriptionActive(SubscriptionType::RemoveAds))
    return false;

  auto const position = GetCurrentPosition();
  if (!position)
    return false;

  auto const taxiEngine = GetTaxiEngine(platform::GetCurrentNetworkPolicy());
  if (!taxiEngine)
    return false;

  auto const providers = taxiEngine->GetProvidersAtPos(MercatorBounds::ToLatLon(position.get()));
  return std::find(providers.begin(), providers.end(), taxi::Provider::Rutaxi) != providers.end();
}

void Framework::RunUITask(function<void()> fn)
{
  GetPlatform().RunTask(Platform::Thread::Gui, move(fn));
}

void Framework::SetSearchDisplacementModeEnabled(bool enabled)
{
  SetDisplacementMode(DisplacementModeManager::SLOT_INTERACTIVE_SEARCH, enabled /* show */);
}

void Framework::ShowViewportSearchResults(search::Results::ConstIter begin,
                                          search::Results::ConstIter end, bool clear)
{
  FillSearchResultsMarks(begin, end, clear,
                         Framework::SearchMarkPostProcessing());
}

void Framework::ShowViewportSearchResults(search::Results::ConstIter begin,
                                          search::Results::ConstIter end, bool clear,
                                          booking::filter::Types types)
{
  using booking::filter::Type;
  using booking::filter::CachedResults;

  ASSERT(!types.empty(), ());

  search::Results results;
  results.AddResultsNoChecks(begin, end);

  auto const fillCallback = [this, clear, results](CachedResults filtersResults)
  {
    auto const postProcessing = [filtersResults = move(filtersResults)](SearchMarkPoint & mark)
    {
      auto const & id = mark.GetFeatureID();

      if (!id.IsValid())
        return;

      for (auto const filterResult : filtersResults)
      {
        auto const found = std::binary_search(filterResult.m_featuresSorted.cbegin(),
                                              filterResult.m_featuresSorted.cend(), id);

        switch (filterResult.m_type)
        {
        case Type::Deals: mark.SetSale(found); break;
        case Type::Availability: mark.SetPreparing(!found); break;
        }
      }
    };

    FillSearchResultsMarks(results.begin(), results.end(), clear,
                           postProcessing);
  };

  m_bookingFilterProcessor.GetFeaturesFromCache(types, results, fillCallback);
}

void Framework::ClearViewportSearchResults()
{
  GetBookmarkManager().GetEditSession().ClearGroup(UserMark::Type::SEARCH);
}

boost::optional<m2::PointD> Framework::GetCurrentPosition() const
{
  auto const & myPosMark = GetBookmarkManager().MyPositionMark();
  if (!myPosMark.HasPosition())
    return {};
  return myPosMark.GetPivot();
}

bool Framework::ParseSearchQueryCommand(search::SearchParams const & params)
{
  if (ParseDrapeDebugCommand(params.m_query))
    return true;
  if (ParseSetGpsTrackMinAccuracyCommand(params.m_query))
    return true;
  if (ParseEditorDebugCommand(params))
    return true;
  if (ParseRoutingDebugCommand(params))
    return true;
  return false;
}

search::ProductInfo Framework::GetProductInfo(search::Result const & result) const
{
  ASSERT(m_ugcApi, ());
  if (result.GetResultType() != search::Result::Type::Feature)
    return {};

  search::ProductInfo productInfo;

  productInfo.m_isLocalAdsCustomer = m_localAdsManager.Contains(result.GetFeatureID());

  auto const ugc = m_ugcApi->GetLoader().GetUGC(result.GetFeatureID());
  productInfo.m_ugcRating = ugc.m_totalRating;

  return productInfo;
}

double Framework::GetMinDistanceBetweenResults() const
{
  return m_searchMarks.GetMaxDimension(m_currentModelView);
}

vector<MwmSet::MwmId> Framework::GetMwmsByRect(m2::RectD const & rect, bool rough) const
{
  vector<MwmSet::MwmId> result;
  if (!m_infoGetter)
    return result;

  auto countryIds = m_infoGetter->GetRegionsCountryIdByRect(rect, rough);
  for (auto const & countryId : countryIds)
    result.push_back(GetMwmIdByName(countryId));

  return result;
}

MwmSet::MwmId Framework::GetMwmIdByName(string const & name) const
{
  return m_model.GetDataSource().GetMwmIdByCountryFile(platform::CountryFile(name));
}

void Framework::ReadFeatures(function<void(FeatureType &)> const & reader,
                             vector<FeatureID> const & features)
{
  m_model.ReadFeatures(reader, features);
}

// RoutingManager::Delegate
void Framework::OnRouteFollow(routing::RouterType type)
{
  bool const isPedestrianRoute = type == RouterType::Pedestrian;
  bool const enableAutoZoom = isPedestrianRoute ? false : LoadAutoZoom();
  int const scale =
      isPedestrianRoute ? scales::GetPedestrianNavigationScale() : scales::GetNavigationScale();
  int scale3d =
      isPedestrianRoute ? scales::GetPedestrianNavigation3dScale() : scales::GetNavigation3dScale();
  if (enableAutoZoom)
    ++scale3d;

  bool const isBicycleRoute = type == RouterType::Bicycle;
  if ((isPedestrianRoute || isBicycleRoute) && LoadTrafficEnabled())
  {
    m_trafficManager.SetEnabled(false /* enabled */);
    SaveTrafficEnabled(false /* enabled */);
  }

  m_drapeEngine->FollowRoute(scale, scale3d, enableAutoZoom);
}

// RoutingManager::Delegate
void Framework::RegisterCountryFilesOnRoute(shared_ptr<routing::NumMwmIds> ptr) const
{
  m_storage.ForEachCountryFile(
      [&ptr](platform::CountryFile const & file) { ptr->RegisterFile(file); });
}

void Framework::InitCityFinder()
{
  ASSERT(!m_cityFinder, ());

  m_cityFinder = make_unique<search::CityFinder>(m_model.GetDataSource());
}

void Framework::InitTaxiEngine()
{
  ASSERT(!m_taxiEngine, ());
  ASSERT(m_infoGetter, ());
  ASSERT(m_cityFinder, ());

  m_taxiEngine = std::make_unique<taxi::Engine>();

  m_taxiEngine->SetDelegate(
      std::make_unique<TaxiDelegate>(GetStorage(), *m_infoGetter, *m_cityFinder));
}

void Framework::SetPlacePageLocation(place_page::Info & info)
{
  ASSERT(m_infoGetter, ());

  if (info.GetCountryId().empty())
    info.SetCountryId(m_infoGetter->GetRegionCountryId(info.GetMercator()));

  TCountriesVec countries;
  if (info.GetTopmostCountryIds().empty())
  {
    GetStorage().GetTopmostNodesFor(info.GetCountryId(), countries);
    info.SetTopmostCountryIds(move(countries));
  }
}

void Framework::InjectViator(place_page::Info & info)
{
  auto needToInject = GetDrawScale() <= scales::GetUpperWorldScale() && !info.IsSponsored() &&
                      !info.GetCountryId().empty() &&
                      GetStorage().IsNodeDownloaded(info.GetCountryId()) &&
                      ftypes::IsCityChecker::Instance()(info.GetTypes());

  if (!needToInject)
    return;

  auto const & country = GetStorage().CountryByCountryId(info.GetCountryId());
  auto const mwmId = m_model.GetDataSource().GetMwmIdByCountryFile(country.GetFile());

  if (!mwmId.IsAlive() || !mwmId.GetInfo()->IsRegistered())
    return;

  auto const point = MercatorBounds::FromLatLon(info.GetLatLon());
  // 3 meters - empirically calculated search radius.
  static double constexpr kSearchRadiusM = 3.0;
  m2::RectD const rect = MercatorBounds::RectByCenterXYAndSizeInMeters(point, kSearchRadiusM);

  m_model.GetDataSource().ForEachInRectForMWM(
      [&info](FeatureType & ft) {
        if (ft.GetFeatureType() != feature::EGeomType::GEOM_POINT || info.IsSponsored() ||
            !ftypes::IsViatorChecker::Instance()(ft))
        {
          return;
        }

        info.SetSponsoredType(place_page::SponsoredType::Viator);
        auto const & sponsoredId = ft.GetMetadata().Get(feature::Metadata::FMD_SPONSORED_ID);
        info.SetSponsoredDescriptionUrl(viator::Api::GetCityUrl(sponsoredId));
      },
      rect, scales::GetUpperScale(), mwmId);
}

void Framework::FillLocalExperts(FeatureType & ft, place_page::Info & info) const
{
  if (GetDrawScale() > scales::GetUpperWorldScale() ||
      !ftypes::IsCityChecker::Instance()(ft))
  {
    info.SetLocalsStatus(place_page::LocalsStatus::NotAvailable);
    return;
  }

  info.SetLocalsStatus(place_page::LocalsStatus::Available);
  info.SetLocalsPageUrl(locals::Api::GetLocalsPageUrl());
}

void Framework::FillDescription(FeatureType & ft, place_page::Info & info) const
{
  if (!ft.GetID().m_mwmId.IsAlive())
    return;
  auto const & regionData = ft.GetID().m_mwmId.GetInfo()->GetRegionData();
  auto const deviceLang = StringUtf8Multilang::GetLangIndex(languages::GetCurrentNorm());
  auto const langPriority = feature::GetDescriptionLangPriority(regionData, deviceLang);

  std::string description;
  if (m_descriptionsLoader->GetDescription(ft.GetID(), langPriority, description))
    info.SetDescription(std::move(description));
}

void Framework::UploadUGC(User::CompleteUploadingHandler const & onCompleteUploading)
{
  if (GetPlatform().ConnectionStatus() == Platform::EConnectionType::CONNECTION_NONE ||
      !m_user.IsAuthenticated())
  {
    if (onCompleteUploading != nullptr)
      onCompleteUploading(false);

    return;
  }

  m_ugcApi->GetUGCToSend([this, onCompleteUploading](string && json, size_t numberOfUnsynchronized)
  {
    if (!json.empty())
    {
      m_user.UploadUserReviews(std::move(json), numberOfUnsynchronized,
                               [this, onCompleteUploading](bool isSuccessful)
      {
        if (onCompleteUploading != nullptr)
          onCompleteUploading(isSuccessful);

        if (isSuccessful)
          m_ugcApi->SendingCompleted();
      });
    }
    else
    {
      if (onCompleteUploading != nullptr)
        onCompleteUploading(true);
    }
  });
}

void Framework::GetUGC(FeatureID const & id, ugc::Api::UGCCallback const & callback)
{
  m_ugcApi->GetUGC(id, [this, callback](ugc::UGC const & ugc, ugc::UGCUpdate const & update)
  {
    ugc::UGC filteredUGC = ugc;
    filteredUGC.m_reviews = FilterUGCReviews(ugc.m_reviews);
    std::sort(filteredUGC.m_reviews.begin(), filteredUGC.m_reviews.end(),
              [](ugc::Review const & r1, ugc::Review const & r2)
    {
      return r1.m_time > r2.m_time;
    });
    callback(filteredUGC, update);
  });
}

ugc::Reviews Framework::FilterUGCReviews(ugc::Reviews const & reviews) const
{
  ugc::Reviews result;
  auto const details = m_user.GetDetails();
  ASSERT(std::is_sorted(details.m_reviewIds.begin(), details.m_reviewIds.end()), ());
  for (auto const & review : reviews)
  {
    if (!std::binary_search(details.m_reviewIds.begin(), details.m_reviewIds.end(), review.m_id))
      result.push_back(review);
  }
  return result;
}

void Framework::FilterResultsForHotelsQuery(booking::filter::Tasks const & filterTasks,
                                            search::Results const & results, bool inViewport)
{
  using namespace booking::filter;

  TasksInternal tasksInternal;

  for (auto const & task : filterTasks)
  {
    auto const type = task.m_type;
    auto const & apiParams = task.m_filterParams.m_apiParams;
    auto const & cb = task.m_filterParams.m_callback;

    if (apiParams->IsEmpty())
      continue;

    ParamsInternal paramsInternal
      {
        apiParams,
        [this, type, apiParams, cb, inViewport](search::Results const & results)
        {
          if (results.GetCount() == 0)
            return;

          std::vector<FeatureID> features;
          for (auto const & r : results)
            features.push_back(r.GetFeatureID());

          std::sort(features.begin(), features.end());

          if (inViewport)
          {
            GetPlatform().RunTask(Platform::Thread::Gui, [this, type, features]()
            {
              switch (type)
              {
              case Type::Deals:
                m_searchMarks.SetSales(features, true /* hasSale */);
                break;
              case Type::Availability:
                m_searchMarks.SetPreparingState(features, false /* isPreparing */);
                break;
              }
            });
          }
          cb(apiParams, features);
        }
      };

    tasksInternal.emplace_back(type, std::move(paramsInternal));
  }

  m_bookingFilterProcessor.ApplyFilters(results, std::move(tasksInternal), filterTasks.GetMode());
}

void Framework::OnBookingFilterParamsUpdate(booking::filter::Tasks const & filterTasks)
{
  for (auto const & task : filterTasks)
  {
    if (task.m_type == booking::filter::Type::Availability)
      m_bookingAvailabilityParams.Set(*task.m_filterParams.m_apiParams);

    m_bookingFilterProcessor.OnParamsUpdated(task.m_type, task.m_filterParams.m_apiParams);
  }
}

booking::AvailabilityParams Framework::GetLastBookingAvailabilityParams() const
{
  return m_bookingAvailabilityParams;
}

void Framework::OnPowerFacilityChanged(PowerManager::Facility const facility, bool enabled)
{
  // Dummy.
  // TODO: process facilities which do not have switch in UI.
}

TipsApi const & Framework::GetTipsApi() const
{
  return m_tipsApi;
}

bool Framework::HaveTransit(m2::PointD const & pt) const
{
  auto const & dataSource = m_model.GetDataSource();
  MwmSet::MwmId const mwmId =
      dataSource.GetMwmIdByCountryFile(platform::CountryFile(m_infoGetter->GetRegionCountryId(pt)));

  MwmSet::MwmHandle handle = m_model.GetDataSource().GetMwmHandleById(mwmId);
  if (!handle.IsAlive())
    return false;

  return handle.GetValue<MwmValue>()->m_cont.IsExist(TRANSIT_FILE_TAG);
}

double Framework::GetLastBackgroundTime() const
{
  return m_startBackgroundTime;
}

bool Framework::MakePlacePageInfo(eye::MapObject const & mapObject, place_page::Info & info) const
{
  m2::RectD rect = MercatorBounds::RectByCenterXYAndOffset(mapObject.GetPos(), kMwmPointAccuracy);
  bool found = false;

  m_model.GetDataSource().ForEachInRect([this, &info, &mapObject, &found](FeatureType & ft)
  {
   if (found || !feature::GetCenter(ft).EqualDxDy(mapObject.GetPos(), kMwmPointAccuracy))
     return;

   auto const foundMapObject = utils::MakeEyeMapObject(ft);
   if (!foundMapObject.IsEmpty() && mapObject.AlmostEquals(foundMapObject))
   {
     FillInfoFromFeatureType(ft, info);
     found = true;
   }
  },
  rect, scales::GetUpperScale());

  return found;
}
