#include "map/framework.hpp"
#include "map/ge0_parser.hpp"
#include "map/geourl_process.hpp"
#include "map/gps_tracker.hpp"

#include "defines.hpp"

#include "routing/online_absent_fetcher.hpp"
#include "routing/osrm_router.hpp"
#include "routing/road_graph_router.hpp"
#include "routing/route.hpp"
#include "routing/routing_algorithm.hpp"

#include "search/geometry_utils.hpp"
#include "search/intermediate_result.hpp"
#include "search/result.hpp"
#include "search/reverse_geocoder.hpp"
#include "search/search_engine.hpp"
#include "search/search_query_factory.hpp"

#include "storage/storage_helpers.hpp"

#include "drape_frontend/color_constants.hpp"
#include "drape_frontend/gps_track_point.hpp"
#include "drape_frontend/visual_params.hpp"
#include "drape_frontend/watch/cpu_drawer.hpp"
#include "drape_frontend/watch/feature_processor.hpp"

#include "indexer/categories_holder.hpp"
#include "indexer/classificator.hpp"
#include "indexer/classificator_loader.hpp"
#include "indexer/drawing_rules.hpp"
#include "indexer/editable_map_object.hpp"
#include "indexer/feature.hpp"
#include "indexer/feature_visibility.hpp"
#include "indexer/map_style_reader.hpp"
#include "indexer/osm_editor.hpp"
#include "indexer/scales.hpp"

/// @todo Probably it's better to join this functionality.
//@{
#include "indexer/feature_algo.hpp"
#include "indexer/feature_utils.hpp"
//@}

#include "storage/country_info_getter.hpp"

#include "platform/local_country_file_utils.hpp"
#include "platform/measurement_utils.hpp"
#include "platform/mwm_version.hpp"
#include "platform/platform.hpp"
#include "platform/preferred_languages.hpp"
#include "platform/settings.hpp"

#include "coding/internal/file_data.hpp"
#include "coding/zip_reader.hpp"
#include "coding/url_encode.hpp"
#include "coding/file_name_utils.hpp"
#include "coding/png_memory_encoder.hpp"

#include "geometry/angles.hpp"
#include "geometry/distance_on_sphere.hpp"
#include "geometry/triangle2d.hpp"

#include "base/math.hpp"
#include "base/timer.hpp"
#include "base/scope_guard.hpp"

#include "std/algorithm.hpp"
#include "std/bind.hpp"
#include "std/target_os.hpp"
#include "std/vector.hpp"

#include "api/internal/c/api-client-internals.h"
#include "api/src/c/api-client.h"

#include "3party/Alohalytics/src/alohalytics.h"

#define KMZ_EXTENSION ".kmz"

#define DEFAULT_BOOKMARK_TYPE "placemark-red"

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
static const int BM_TOUCH_PIXEL_INCREASE = 20;
static const int kKeepPedestrianDistanceMeters = 10000;
char const kRouterTypeKey[] = "router";
char const kMapStyleKey[] = "MapStyleKeyV1";
char const kAllow3dKey[] = "Allow3d";
char const kAllow3dBuildingsKey[] = "Buildings3d";

double const kDistEqualQuery = 100.0;

double const kRotationAngle = math::pi4;
double const kAngleFOV = math::pi / 3.0;

// TODO!
// To adjust GpsTrackFilter was added secret command "?gpstrackaccuracy:xxx;"
// where xxx is a new value for horizontal accuracy.
// This is temporary solution while we don't have a good filter.
void ParseSetGpsTrackMinAccuracyCommand(string const & query)
{
  const char kGpsAccuracy[] = "?gpstrackaccuracy:";
  if (strings::StartsWith(query, kGpsAccuracy))
  {
    size_t const end = query.find(';', sizeof(kGpsAccuracy) - 1);
    if (end != string::npos)
    {
      string s(query.begin() + sizeof(kGpsAccuracy) - 1, query.begin() + end);
      double value;
      if (strings::to_double(s, value))
      {
        GpsTrackFilter::StoreMinHorizontalAccuracy(value);
      }
    }
  }
}

// Cancels search query by |handle|.
void CancelQuery(weak_ptr<search::QueryHandle> & handle)
{
  auto queryHandle = handle.lock();
  if (queryHandle)
    queryHandle->Cancel();
  handle.reset();
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
  CallDrapeFunction(bind(&df::DrapeEngine::LoseLocation, _1));
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

#ifdef OMIM_OS_ANDROID
  m_lastGPSInfo.reset(new GpsInfo(rInfo));
#endif
  location::RouteMatchingInfo routeMatchingInfo;
  CheckLocationForRouting(rInfo);

  MatchLocationToRoute(rInfo, routeMatchingInfo);

  CallDrapeFunction(bind(&df::DrapeEngine::SetGpsInfo, _1, rInfo,
                         m_routingSession.IsNavigable(), routeMatchingInfo));
}

void Framework::OnCompassUpdate(CompassInfo const & info)
{
#ifdef FIXED_LOCATION
  CompassInfo rInfo(info);
  m_fixedPos.GetNorth(rInfo.m_bearing);
#else
  CompassInfo const & rInfo = info;
#endif

#ifdef OMIM_OS_ANDROID
  m_lastCompassInfo.reset(new CompassInfo(rInfo));
#endif
  CallDrapeFunction(bind(&df::DrapeEngine::SetCompassInfo, _1, rInfo));
}

void Framework::SwitchMyPositionNextMode()
{
  CallDrapeFunction(bind(&df::DrapeEngine::SwitchMyPositionNextMode, _1));
}

void Framework::SetMyPositionModeListener(TMyPositionModeChanged && fn)
{
  m_myPositionListener = move(fn);
}

void Framework::OnUserPositionChanged(m2::PointD const & position)
{
  MyPositionMarkPoint * myPosition = UserMarkContainer::UserMarkForMyPostion();
  myPosition->SetUserPosition(position);

  if (IsRoutingActive())
    m_routingSession.SetUserCurrentPosition(position);
}

void Framework::CallDrapeFunction(TDrapeFunction const & fn) const
{
  if (m_drapeEngine)
    fn(m_drapeEngine.get());
}

void Framework::StopLocationFollow()
{
  CallDrapeFunction(bind(&df::DrapeEngine::StopLocationFollow, _1));
}

bool Framework::IsEnoughSpaceForMigrate() const
{
  return GetPlatform().GetWritableStorageStatus(kMaxMwmSizeBytes) == Platform::TStorageStatus::STORAGE_OK;
}

TCountryId Framework::PreMigrate(ms::LatLon const & position,
                           Storage::TChangeCountryFunction const & change,
                           Storage::TProgressFunction const & progress)
{
  Storage().PrefetchMigrateData();

  auto const infoGetter =
      CountryInfoReader::CreateCountryInfoReaderOneComponentMwms(GetPlatform());

  TCountryId currentCountryId =
      infoGetter->GetRegionCountryId(MercatorBounds::FromLatLon(position));

  if (currentCountryId == kInvalidCountryId)
    return kInvalidCountryId;

  Storage().GetPrefetchStorage()->Subscribe(change, progress);
  Storage().GetPrefetchStorage()->DownloadNode(currentCountryId);
  return currentCountryId;
}

void Framework::Migrate(bool keepDownloaded)
{
  // Drape must be suspended while migration is performed since it access different parts of
  // framework (i.e. m_infoGetter) which are reinitialized during migration process.
  // If we do not suspend drape, it tries to access framework fields (i.e. m_infoGetter) which are null
  // while migration is performed.
  if (m_drapeEngine && m_isRenderingEnabled)
    m_drapeEngine->SetRenderingEnabled(false);
  m_searchEngine.reset();
  m_infoGetter.reset();
  TCountriesVec existedCountries;
  Storage().DeleteAllLocalMaps(&existedCountries);
  DeregisterAllMaps();
  m_model.Clear();
  Storage().Migrate(keepDownloaded ? existedCountries : TCountriesVec());
  InitCountryInfoGetter();
  InitSearchEngine();
  RegisterAllMaps();
  if (m_drapeEngine && m_isRenderingEnabled)
    m_drapeEngine->SetRenderingEnabled(true);
  InvalidateRect(MercatorBounds::FullRect());
}

Framework::Framework()
  : m_startForegroundTime(0.0)
  , m_storage(platform::migrate::NeedMigrate() ? COUNTRIES_OBSOLETE_FILE : COUNTRIES_FILE)
  , m_bmManager(*this)
  , m_isRenderingEnabled(true)
  , m_fixedSearchResults(0)
  , m_lastReportedCountry(kInvalidCountryId)
{
  m_startBackgroundTime = my::Timer::LocalTime();

  // Restore map style before classificator loading
  int mapStyle;
  if (!settings::Get(kMapStyleKey, mapStyle))
    mapStyle = kDefaultMapStyle;
  GetStyleReader().SetCurrentStyle(static_cast<MapStyle>(mapStyle));

  m_connectToGpsTrack = GpsTracker::Instance().IsEnabled();

  m_ParsedMapApi.SetBookmarkManager(&m_bmManager);

  // Init strings bundle.
  // @TODO. There are hardcoded strings below which are defined in strings.txt as well.
  // It's better to use strings form strings.txt intead of hardcoding them here.
  m_stringsBundle.SetDefaultString("placepage_unknown_place", "Unknown Place");
  m_stringsBundle.SetDefaultString("my_places", "My Places");
  m_stringsBundle.SetDefaultString("routes", "Routes");
  m_stringsBundle.SetDefaultString("wifi", "WiFi");

  m_stringsBundle.SetDefaultString("routing_failed_unknown_my_position", "Current location is undefined. Please specify location to create route.");
  m_stringsBundle.SetDefaultString("routing_failed_has_no_routing_file", "Additional data is required to create the route. Download data now?");
  m_stringsBundle.SetDefaultString("routing_failed_start_point_not_found", "Cannot calculate the route. No roads near your starting point.");
  m_stringsBundle.SetDefaultString("routing_failed_dst_point_not_found", "Cannot calculate the route. No roads near your destination.");
  m_stringsBundle.SetDefaultString("routing_failed_cross_mwm_building", "Routes can only be created that are fully contained within a single map.");
  m_stringsBundle.SetDefaultString("routing_failed_route_not_found", "There is no route found between the selected origin and destination.Please select a different start or end point.");
  m_stringsBundle.SetDefaultString("routing_failed_internal_error", "Internal error occurred. Please try to delete and download the map again. If problem persist please contact us at support@maps.me.");

  m_model.InitClassificator();
  m_model.SetOnMapDeregisteredCallback(bind(&Framework::OnMapDeregistered, this, _1));
  LOG(LDEBUG, ("Classificator initialized"));


  // To avoid possible races - init country info getter once in constructor.
  InitCountryInfoGetter();
  LOG(LDEBUG, ("Country info getter initialized"));

  // To avoid possible races - init search engine once in constructor.
  InitSearchEngine();
  LOG(LDEBUG, ("Search engine initialized"));

  RegisterAllMaps();
  LOG(LDEBUG, ("Maps initialized"));

  // Init storage with needed callback.
  m_storage.Init(
                 bind(&Framework::OnCountryFileDownloaded, this, _1, _2),
                 bind(&Framework::OnCountryFileDelete, this, _1, _2));
  m_storage.SetDownloadingPolicy(&m_storageDownloadingPolicy);
  LOG(LDEBUG, ("Storage initialized"));

  auto const routingStatisticsFn = [](map<string, string> const & statistics)
  {
    alohalytics::LogEvent("Routing_CalculatingRoute", statistics);
  };
#ifdef DEBUG
  auto const routingVisualizerFn = [this](m2::PointD const & pt)
  {
    UserMarkControllerGuard guard(m_bmManager, UserMarkType::DEBUG_MARK);
    guard.m_controller.SetIsVisible(true);
    guard.m_controller.SetIsDrawable(true);

    guard.m_controller.CreateUserMark(pt);
  };
#else
  routing::RouterDelegate::TPointCheckCallback const routingVisualizerFn = nullptr;
#endif
  m_routingSession.Init(routingStatisticsFn, routingVisualizerFn);

  SetRouterImpl(RouterType::Vehicle);

  UpdateMinBuildingsTapZoom();

  LOG(LDEBUG, ("Routing engine initialized"));

  LOG(LINFO, ("System languages:", languages::GetPreferred()));

  osm::Editor & editor = osm::Editor::Instance();
  editor.SetMwmIdByNameAndVersionFn([this](string const & name) -> MwmSet::MwmId
  {
    return m_model.GetIndex().GetMwmIdByCountryFile(platform::CountryFile(name));
  });
  editor.SetInvalidateFn([this](){ InvalidateRect(GetCurrentViewport()); });
  editor.SetFeatureLoaderFn([this](FeatureID const & fid) -> unique_ptr<FeatureType>
  {
    unique_ptr<FeatureType> feature(new FeatureType());
    Index::FeaturesLoaderGuard const guard(m_model.GetIndex(), fid.m_mwmId);
    guard.GetOriginalFeatureByIndex(fid.m_index, *feature);
    feature->ParseEverything();
    return feature;
  });
  editor.SetFeatureOriginalStreetFn([this](FeatureType & ft) -> string
  {
    search::ReverseGeocoder const coder(m_model.GetIndex());
    auto const streets = coder.GetNearbyFeatureStreets(ft);
    if (streets.second < streets.first.size())
      return streets.first[streets.second].m_name;
    return {};
  });
  editor.SetForEachFeatureAtPointFn(bind(&Framework::ForEachFeatureAtPoint, this, _1, _2));
  editor.LoadMapEdits();
}

Framework::~Framework()
{
  m_drapeEngine.reset();

  m_model.SetOnMapDeregisteredCallback(nullptr);
}

void Framework::DrawWatchFrame(m2::PointD const & center, int zoomModifier,
                               uint32_t pxWidth, uint32_t pxHeight,
                               df::watch::FrameSymbols const & symbols,
                               df::watch::FrameImage & image)
{
  ASSERT(IsWatchFrameRendererInited(), ());

  int resultZoom = -1;
  ScreenBase screen = m_cpuDrawer->CalculateScreen(center, zoomModifier, pxWidth, pxHeight, symbols, resultZoom);
  ASSERT_GREATER(resultZoom, 0, ());

  uint32_t const bgColor = drule::rules().GetBgColor(resultZoom);
  m_cpuDrawer->BeginFrame(pxWidth, pxHeight, dp::Extract(bgColor, 255 - (bgColor >> 24)));

  m2::RectD renderRect = m2::RectD(0, 0, pxWidth, pxHeight);
  m2::RectD selectRect;
  m2::RectD clipRect;
  double const inflationSize = 24 * m_cpuDrawer->GetVisualScale();
  screen.PtoG(m2::Inflate(renderRect, inflationSize, inflationSize), clipRect);
  screen.PtoG(renderRect, selectRect);

  uint32_t const tileSize = static_cast<uint32_t>(df::CalculateTileSize(pxWidth, pxHeight));
  int const drawScale = df::GetDrawTileScale(screen, tileSize, m_cpuDrawer->GetVisualScale());
  df::watch::FeatureProcessor doDraw(make_ref(m_cpuDrawer), clipRect, screen, drawScale);

  int const upperScale = scales::GetUpperScale();
  m_model.ForEachFeature(selectRect, doDraw, min(upperScale, drawScale));

  m_cpuDrawer->Flush();
  m_cpuDrawer->DrawMyPosition(screen.GtoP(center));

  if (symbols.m_showSearchResult)
  {
    if (!screen.PixelRect().IsPointInside(screen.GtoP(symbols.m_searchResult)))
      m_cpuDrawer->DrawSearchArrow(ang::AngleTo(center, symbols.m_searchResult));
    else
      m_cpuDrawer->DrawSearchResult(screen.GtoP(symbols.m_searchResult));
  }

  m_cpuDrawer->EndFrame(image);
}

void Framework::InitWatchFrameRenderer(float visualScale)
{
  using namespace df::watch;

  ASSERT(!IsWatchFrameRendererInited(), ());
  if (m_cpuDrawer == nullptr)
  {
    string resPostfix = df::VisualParams::GetResourcePostfix(visualScale);
    m_cpuDrawer = make_unique_dp<CPUDrawer>(CPUDrawer::Params(resPostfix, visualScale));
  }
}

void Framework::ReleaseWatchFrameRenderer()
{
  if (IsWatchFrameRendererInited())
    m_cpuDrawer.reset();
}

bool Framework::IsWatchFrameRendererInited() const
{
  return m_cpuDrawer != nullptr;
}

void Framework::ShowNode(storage::TCountryId const & countryId)
{
  StopLocationFollow();

  ShowRect(CalcLimitRect(countryId, Storage(), CountryInfoGetter()));
}

void Framework::OnCountryFileDownloaded(storage::TCountryId const & countryId, storage::Storage::TLocalFilePtr const localFile)
{
  // Soft reset to signal that mwm file may be out of date in routing caches.
  m_routingSession.Reset();

  m2::RectD rect = MercatorBounds::FullRect();

  if (localFile && HasOptions(localFile->GetFiles(), MapOptions::Map))
  {
    // Add downloaded map.
    auto p = m_model.RegisterMap(*localFile);
    MwmSet::MwmId const & id = p.first;
    if (id.IsAlive())
      rect = id.GetInfo()->m_limitRect;
  }
  InvalidateRect(rect);
  m_searchEngine->ClearCaches();
}

bool Framework::OnCountryFileDelete(storage::TCountryId const & countryId, storage::Storage::TLocalFilePtr const localFile)
{
  // Soft reset to signal that mwm file may be out of date in routing caches.
  m_routingSession.Reset();

  if (countryId == m_lastReportedCountry)
    m_lastReportedCountry = kInvalidCountryId;

  if(auto handle = m_lastQueryHandle.lock())
    handle->Cancel();

  m2::RectD rect = MercatorBounds::FullRect();

  bool deferredDelete = false;
  if (localFile)
  {
    rect = m_infoGetter->GetLimitRectForLeaf(countryId);
    m_model.DeregisterMap(platform::CountryFile(countryId));
    deferredDelete = true;
  }
  InvalidateRect(rect);

  m_searchEngine->ClearCaches();
  return deferredDelete;
}

void Framework::OnMapDeregistered(platform::LocalCountryFile const & localFile)
{
  auto action = [this, localFile]
  {
    m_storage.DeleteCustomCountryVersion(localFile);
  };

  // Call action on thread in which the framework was created
  // For more information look at comment for Observer class in mwm_set.hpp
  if (m_storage.GetThreadChecker().CalledOnOriginalThread())
    action();
  else
    GetPlatform().RunOnGuiThread(action);
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
          m_model.GetIndex().GetMwmIdByCountryFile(platform::CountryFile(fileName)));
  };
  Storage().ForEachInSubtree(countryId, forEachInSubtree);
  return hasUnsavedChanges;
}

void Framework::RegisterAllMaps()
{
  ASSERT(!m_storage.IsDownloadInProgress(),
         ("Registering maps while map downloading leads to removing downloading maps from "
          "ActiveMapsListener::m_items."));

  m_storage.RegisterAllLocalMaps();

  // Fast migrate in case there are no downloaded MWM.
  if (platform::migrate::NeedMigrate())
  {
    bool disableFastMigrate = false;
    settings::Get("DisableFastMigrate", disableFastMigrate);
    if (!disableFastMigrate && !m_storage.HaveDownloadedCountries())
    {
      Storage().PrefetchMigrateData();
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

  m_searchEngine->SetSupportOldFormat(minFormat < static_cast<int>(version::Format::v3));
}

void Framework::DeregisterAllMaps()
{
  m_model.Clear();
  m_storage.Clear();
}

void Framework::LoadBookmarks()
{
  m_bmManager.LoadBookmarks();
}

size_t Framework::AddBookmark(size_t categoryIndex, const m2::PointD & ptOrg, BookmarkData & bm)
{
  return m_bmManager.AddBookmark(categoryIndex, ptOrg, bm);
}

size_t Framework::MoveBookmark(size_t bmIndex, size_t curCatIndex, size_t newCatIndex)
{
  return m_bmManager.MoveBookmark(bmIndex, curCatIndex, newCatIndex);
}

void Framework::ReplaceBookmark(size_t catIndex, size_t bmIndex, BookmarkData const & bm)
{
  m_bmManager.ReplaceBookmark(catIndex, bmIndex, bm);
}

size_t Framework::AddCategory(string const & categoryName)
{
  return m_bmManager.CreateBmCategory(categoryName);
}

namespace
{
  class EqualCategoryName
  {
    string const & m_name;
  public:
    EqualCategoryName(string const & name) : m_name(name) {}
    bool operator() (BookmarkCategory const * cat) const
    {
      return (cat->GetName() == m_name);
    }
  };
}

BookmarkCategory * Framework::GetBmCategory(size_t index) const
{
  return m_bmManager.GetBmCategory(index);
}

bool Framework::DeleteBmCategory(size_t index)
{
  return m_bmManager.DeleteBmCategory(index);
}

void Framework::FillBookmarkInfo(Bookmark const & bmk, BookmarkAndCategory const & bac, place_page::Info & info) const
{
  FillPointInfo(bmk.GetPivot(), string(), info);
  info.m_bac = bac;
}

void Framework::FillFeatureInfo(FeatureID const & fid, place_page::Info & info) const
{
  if (!fid.IsValid())
  {
    LOG(LERROR, ("FeatureID is invalid:", fid));
    return;
  }

  Index::FeaturesLoaderGuard const guard(m_model.GetIndex(), fid.m_mwmId);
  FeatureType ft;
  guard.GetFeatureByIndex(fid.m_index, ft);
  FillInfoFromFeatureType(ft, info);

  // Fill countryId for place page info
  info.m_countryId = m_infoGetter->GetRegionCountryId(info.GetMercator());

  uint32_t placeCountryType = classif().GetTypeByPath({"place", "country"});
  if (info.GetTypes().Has(placeCountryType))
  {
    TCountriesVec countries;
    Storage().GetTopmostNodesFor(info.m_countryId, countries);
    if (countries.size() == 1)
      info.m_countryId = countries.front();
  }
}

void Framework::FillPointInfo(m2::PointD const & mercator, string const & customTitle, place_page::Info & info) const
{
  auto feature = GetFeatureAtPoint(mercator);
  if (feature)
    FillInfoFromFeatureType(*feature, info);
  else
    info.m_customName = customTitle.empty() ? m_stringsBundle.GetString("placepage_unknown_place") : customTitle;

  // This line overwrites mercator center from area feature which can be far away.
  info.SetMercator(mercator);
}

void Framework::FillInfoFromFeatureType(FeatureType const & ft, place_page::Info & info) const
{
  info.SetFromFeatureType(ft);

  info.m_isEditable = osm::Editor::Instance().GetEditableProperties(ft).IsEditable();
  info.m_localizedWifiString = m_stringsBundle.GetString("wifi");

  if (ftypes::IsAddressObjectChecker::Instance()(ft))
    info.m_address = GetAddressInfoAtPoint(feature::GetCenter(ft)).FormatHouseAndStreet();
}

void Framework::FillApiMarkInfo(ApiMarkPoint const & api, place_page::Info & info) const
{
  FillPointInfo(api.GetPivot(), "", info);
  string const & name = api.GetName();
  if (!name.empty())
    info.m_customName = name;
  info.m_apiId = api.GetID();
  info.m_apiUrl = GenerateApiBackUrl(api);
}

void Framework::FillSearchResultInfo(SearchMarkPoint const & smp, place_page::Info & info) const
{
  if (smp.m_foundFeatureID.IsValid())
    FillFeatureInfo(smp.m_foundFeatureID, info);
  else
    FillPointInfo(smp.GetPivot(), smp.m_matchedName, info);
}

void Framework::FillMyPositionInfo(place_page::Info & info) const
{
  double lat, lon;
  VERIFY(GetCurrentPosition(lat, lon), ());
  info.SetMercator(MercatorBounds::FromLatLon(lat, lon));
  info.m_isMyPosition = true;
  info.m_customName = m_stringsBundle.GetString("my_position");
}

void Framework::ShowBookmark(BookmarkAndCategory const & bnc)
{
  StopLocationFollow();

  Bookmark const * mark = static_cast<Bookmark const *>(GetBmCategory(bnc.first)->GetUserMark(bnc.second));

  double scale = mark->GetScale();
  if (scale == -1.0)
    scale = scales::GetUpperComfortScale();

  CallDrapeFunction(bind(&df::DrapeEngine::SetModelViewCenter, _1, mark->GetPivot(), scale, true));

  place_page::Info info;
  FillBookmarkInfo(*mark, bnc, info);
  ActivateMapSelection(true, df::SelectionShape::OBJECT_USER_MARK, info);
  m_lastTapEvent.reset(new df::TapInfo { m_currentModelView.GtoP(info.GetMercator()), false, false, info.GetID() });
}

void Framework::ShowTrack(Track const & track)
{
  StopLocationFollow();
  ShowRect(track.GetLimitRect());
}

void Framework::ClearBookmarks()
{
  m_bmManager.ClearItems();
}

namespace
{

/// @return extension with a dot in lower case
string const GetFileExt(string const & filePath)
{
  string ext = my::GetFileExtension(filePath);
  transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
  return ext;
}

string const GetFileName(string const & filePath)
{
  string ret = filePath;
  my::GetNameFromFullPath(ret);
  return ret;
}

string const GenerateValidAndUniqueFilePathForKML(string const & fileName)
{
  string filePath = BookmarkCategory::RemoveInvalidSymbols(fileName);
  filePath = BookmarkCategory::GenerateUniqueFileName(GetPlatform().SettingsDir(), filePath);
  return filePath;
}

} // namespace

bool Framework::AddBookmarksFile(string const & filePath)
{
  string const fileExt = GetFileExt(filePath);
  string fileSavePath;
  if (fileExt == BOOKMARKS_FILE_EXTENSION)
  {
    fileSavePath = GenerateValidAndUniqueFilePathForKML(GetFileName(filePath));
    if (!my::CopyFileX(filePath, fileSavePath))
      return false;
  }
  else if (fileExt == KMZ_EXTENSION)
  {
    try
    {
      ZipFileReader::FileListT files;
      ZipFileReader::FilesList(filePath, files);
      string kmlFileName;
      for (size_t i = 0; i < files.size(); ++i)
      {
        if (GetFileExt(files[i].first) == BOOKMARKS_FILE_EXTENSION)
        {
          kmlFileName = files[i].first;
          break;
        }
      }
      if (kmlFileName.empty())
        return false;

      fileSavePath = GenerateValidAndUniqueFilePathForKML(kmlFileName);
      ZipFileReader::UnzipFile(filePath, kmlFileName, fileSavePath);
    }
    catch (RootException const & e)
    {
      LOG(LWARNING, ("Error unzipping file", filePath, e.Msg()));
      return false;
    }
  }
  else
  {
    LOG(LWARNING, ("Unknown file type", filePath));
    return false;
  }

  // Update freshly added bookmarks
  m_bmManager.LoadBookmark(fileSavePath);

  return true;
}

void Framework::PrepareToShutdown()
{
  DestroyDrapeEngine();
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
    CallDrapeFunction(bind(&df::DrapeEngine::SetModelViewAnyRect, _1, rect, false));
  else
    ShowAll();
}

void Framework::ShowAll()
{
  CallDrapeFunction(bind(&df::DrapeEngine::SetModelViewAnyRect, _1, m2::AnyRectD(m_model.GetWorldRect()), false));
}

m2::PointD Framework::GetPixelCenter() const
{
  return m_currentModelView.isPerspective() ? m_currentModelView.PixelRectIn3d().Center()
                                            : m_currentModelView.PixelRect().Center();
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
  CallDrapeFunction(bind(&df::DrapeEngine::SetModelViewCenter, _1, pt, zoomLevel, true));
}

m2::RectD Framework::GetCurrentViewport() const
{
  return m_currentModelView.ClipRect();
}

void Framework::ShowRect(m2::RectD const & rect, int maxScale)
{
  CallDrapeFunction(bind(&df::DrapeEngine::SetModelViewRect, _1, rect, true, maxScale, true));
}

void Framework::ShowRect(m2::AnyRectD const & rect)
{
  CallDrapeFunction(bind(&df::DrapeEngine::SetModelViewAnyRect, _1, rect, true));
}

void Framework::GetTouchRect(m2::PointD const & center, uint32_t pxRadius, m2::AnyRectD & rect)
{
  m_currentModelView.GetTouchRect(center, static_cast<double>(pxRadius), rect);
}

int Framework::AddViewportListener(TViewportChanged const & fn)
{
  ASSERT(m_drapeEngine, ());
  return m_drapeEngine->AddModelViewListener(fn);
}

void Framework::RemoveViewportListener(int slotID)
{
  ASSERT(m_drapeEngine, ());
  m_drapeEngine->RemoveModeViewListener(slotID);
}

void Framework::OnSize(int w, int h)
{
  CallDrapeFunction(bind(&df::DrapeEngine::Resize, _1, max(w, 2), max(h, 2)));
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
  Scale(factor, GetPixelCenter(), isAnim);
}

void Framework::Scale(double factor, m2::PointD const & pxPoint, bool isAnim)
{
  CallDrapeFunction(bind(&df::DrapeEngine::Scale, _1, factor, pxPoint, isAnim));
}

void Framework::TouchEvent(df::TouchEvent const & touch)
{
  CallDrapeFunction(bind(&df::DrapeEngine::AddTouchEvent, _1, touch));
}

int Framework::GetDrawScale() const
{
  return df::GetDrawTileScale(m_currentModelView);
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
  CallDrapeFunction(bind(&df::DrapeEngine::InvalidateRect, _1, rect));
}

void Framework::StartInteractiveSearch(search::SearchParams const & params)
{
  using namespace search;

  m_lastInteractiveSearchParams = params;
  m_lastInteractiveSearchParams.SetForceSearch(false);
  m_lastInteractiveSearchParams.SetMode(Mode::Viewport);
  m_lastInteractiveSearchParams.SetSuggestsEnabled(false);
  m_lastInteractiveSearchParams.m_onResults = [this](Results const & results)
  {
    if (!results.IsEndMarker())
    {
      GetPlatform().RunOnGuiThread([this, results]()
      {
        if (IsInteractiveSearchActive())
          FillSearchResultsMarks(results);
      });
    }
  };
}

void Framework::ClearAllCaches()
{
  m_model.ClearCaches();
  m_infoGetter->ClearCaches();
  m_searchEngine->ClearCaches();
}

void Framework::OnUpdateCurrentCountry(m2::PointF const & pt, int zoomLevel)
{
   storage::TCountryId newCountryId;
   if (zoomLevel > scales::GetUpperWorldScale())
     newCountryId = m_infoGetter->GetRegionCountryId(m2::PointD(pt));

   if (newCountryId == m_lastReportedCountry)
     return;

   m_lastReportedCountry = newCountryId;

   GetPlatform().RunOnGuiThread([this, newCountryId]()
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

void Framework::UpdateUserViewportChanged()
{
  if (IsInteractiveSearchActive())
  {
    (void)GetCurrentPosition(m_lastInteractiveSearchParams.m_lat,
                             m_lastInteractiveSearchParams.m_lon);
    Search(m_lastInteractiveSearchParams);
  }
}

bool Framework::Search(search::SearchParams const & params)
{
#ifdef FIXED_LOCATION
  search::SearchParams rParams(params);
  if (params.IsValidPosition())
  {
    m_fixedPos.GetLat(rParams.m_lat);
    m_fixedPos.GetLon(rParams.m_lon);
  }
#else
  search::SearchParams const & rParams = params;
#endif

  ParseSetGpsTrackMinAccuracyCommand(params.m_query);
  if (ParseEditorDebugCommand(params))
    return true;

  m2::RectD const viewport = GetCurrentViewport();

  if (QueryMayBeSkipped(rParams, viewport))
    return false;

  m_lastQueryParams = rParams;
  m_lastQueryViewport = viewport;

  // Cancels previous search request (if any) and initiates new search request.
  CancelQuery(m_lastQueryHandle);
  m_lastQueryHandle = m_searchEngine->Search(m_lastQueryParams, m_lastQueryViewport);
  return true;
}

bool Framework::GetGroupCountryIdFromFeature(FeatureType const & ft, string & name) const
{
  int8_t langIndices[] = { StringUtf8Multilang::kEnglishCode,
                           StringUtf8Multilang::kDefaultCode,
                           StringUtf8Multilang::kInternationalCode };

  for (auto const langIndex : langIndices)
  {
    if (!ft.GetName(langIndex, name))
      continue;
    if (Storage().IsCoutryIdCountryTreeInnerNode(name))
      return true;
  }
  return false;
}

bool Framework::SearchInDownloader(DownloaderSearchParams const & params)
{
  search::SearchParams searchParam;
  searchParam.m_query = params.m_query;
  searchParam.m_inputLocale = params.m_inputLocale;
  searchParam.SetMode(search::Mode::World);
  searchParam.SetSuggestsEnabled(false);
  searchParam.SetForceSearch(true);
  searchParam.m_onResults = [this, params](search::Results const & results)
  {
    DownloaderSearchResults downloaderSearchResults;
    for (auto it = results.Begin(); it != results.End(); ++it)
    {
      if (!it->HasPoint())
        continue;

      if (it->GetResultType() != search::Result::RESULT_LATLON)
      {
        FeatureID const & fid = it->GetFeatureID();
        Index::FeaturesLoaderGuard loader(m_model.GetIndex(), fid.m_mwmId);
        FeatureType ft;
        loader.GetFeatureByIndex(fid.m_index, ft);
        ftypes::Type const type = ftypes::IsLocalityChecker::Instance().GetType(ft);

        if (type == ftypes::COUNTRY || type == ftypes::STATE)
        {
          string groupFeatureName;
          if (GetGroupCountryIdFromFeature(ft, groupFeatureName))
          {
            downloaderSearchResults.m_results.emplace_back(groupFeatureName,
                                                           it->GetString() /* m_matchedName */);
            continue;
          }
        }
      }

      auto const & mercator = it->GetFeatureCenter();
      TCountryId const & countryId = CountryInfoGetter().GetRegionCountryId(mercator);
      if (countryId == kInvalidCountryId)
        continue;
      downloaderSearchResults.m_results.emplace_back(countryId,
                                                     it->GetString() /* m_matchedName */);
    }
    downloaderSearchResults.m_query = params.m_query;
    downloaderSearchResults.m_endMarker = results.IsEndMarker();
    params.m_onResults(downloaderSearchResults);
  };

  return Search(searchParam);
}

void Framework::MemoryWarning()
{
  LOG(LINFO, ("MemoryWarning"));
  ClearAllCaches();
  SharedBufferManager::instance().clearReserved();
}

void Framework::EnterBackground()
{
  m_startBackgroundTime = my::Timer::LocalTime();
  settings::Set("LastEnterBackground", m_startBackgroundTime);

  SaveViewport();

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
  m_startForegroundTime = my::Timer::LocalTime();
  double const time = m_startForegroundTime - m_startBackgroundTime;
  CallDrapeFunction(bind(&df::DrapeEngine::SetTimeInBackground, _1, time));
}

bool Framework::GetCurrentPosition(double & lat, double & lon) const
{
  m2::PointD pos;
  MyPositionMarkPoint * myPosMark = UserMarkContainer::UserMarkForMyPostion();
  if (!myPosMark->HasPosition())
    return false;

  pos = myPosMark->GetPivot();

  lat = MercatorBounds::YToLat(pos.y);
  lon = MercatorBounds::XToLon(pos.x);
  return true;
}

void Framework::InitCountryInfoGetter()
{
  ASSERT(!m_infoGetter.get(), ("InitCountryInfoGetter() must be called only once."));

  m_infoGetter = CountryInfoReader::CreateCountryInfoReader(GetPlatform());
  m_infoGetter->InitAffiliationsInfo(&m_storage.GetAffiliations());
}

void Framework::InitSearchEngine()
{
  ASSERT(!m_searchEngine.get(), ("InitSearchEngine() must be called only once."));
  ASSERT(m_infoGetter.get(), ());
  try
  {
    search::Engine::Params params;
    params.m_locale = languages::GetCurrentOrig();
    params.m_numThreads = 1;
    m_searchEngine.reset(new search::Engine(const_cast<Index &>(m_model.GetIndex()),
                                            GetDefaultCategories(), *m_infoGetter,
                                            make_unique<search::SearchQueryFactory>(), params));
  }
  catch (RootException const & e)
  {
    LOG(LCRITICAL, ("Can't load needed resources for search::Engine:", e.Msg()));
  }
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

bool Framework::QueryMayBeSkipped(search::SearchParams const & params,
                                  m2::RectD const & viewport) const
{
  if (params.IsForceSearch())
    return false;
  if (!m_lastQueryParams.IsEqualCommon(params))
    return false;
  if (!m_lastQueryViewport.IsValid() ||
      !search::IsEqualMercator(m_lastQueryViewport, viewport, kDistEqualQuery))
  {
    return false;
  }
  if (!m_lastQueryParams.IsSearchAroundPosition() ||
      ms::DistanceOnEarth(m_lastQueryParams.m_lat, m_lastQueryParams.m_lon, params.m_lat,
                          params.m_lon) <= kDistEqualQuery)
  {
    return false;
  }
  return true;
}

void Framework::LoadSearchResultMetadata(search::Result & res) const
{
  if (res.m_metadata.m_isInitialized || res.GetResultType() != search::Result::RESULT_FEATURE)
    return;

  FeatureID const & id = res.GetFeatureID();
  ASSERT(id.IsValid(), ("Search result doesn't contain valid FeatureID."));
  // TODO @yunikkk refactor to format search result metadata accordingly with place_page::Info
  search::ProcessMetadata(*GetFeatureByID(id), res.m_metadata);
  // res.m_metadata.m_isInitialized is set to true in ProcessMetadata.
}

void Framework::ShowSearchResult(search::Result const & res)
{
  CancelInteractiveSearch();
  StopLocationFollow();

  alohalytics::LogEvent("searchShowResult", {{"pos", strings::to_string(res.GetPositionInResults())},
                                             {"result", res.ToStringForStats()}});
  place_page::Info info;
  using namespace search;
  int scale;
  switch (res.GetResultType())
  {
    case Result::RESULT_FEATURE:
      FillFeatureInfo(res.GetFeatureID(), info);
      scale = GetFeatureViewportScale(info.GetTypes());
      break;

    case Result::RESULT_LATLON:
      FillPointInfo(res.GetFeatureCenter(), res.GetString(), info);
      scale = scales::GetUpperComfortScale();
      break;

    case Result::RESULT_SUGGEST_PURE:
    case Result::RESULT_SUGGEST_FROM_FEATURE:
      ASSERT(false, ("Suggests should not be here."));
      return;
  }

  m2::PointD const center = info.GetMercator();
  if (m_currentModelView.isPerspective())
    CallDrapeFunction(bind(&df::DrapeEngine::SetModelViewCenter, _1, center, scale, true));
  else
    ShowRect(df::GetRectForDrawScale(scale, center));

  UserMarkContainer::UserMarkForPoi()->SetPtOrg(center);
  ActivateMapSelection(false, df::SelectionShape::OBJECT_POI, info);
  m_lastTapEvent.reset(new df::TapInfo { m_currentModelView.GtoP(center), false, false, info.GetID() });
}

size_t Framework::ShowSearchResults(search::Results const & results)
{
  using namespace search;

  size_t count = results.GetCount();
  switch (count)
  {
  case 1:
    {
      Result const & r = results.GetResult(0);
      if (!r.IsSuggest())
        ShowSearchResult(r);
      else
        count = 0;
      // do not put break here
    }
  case 0:
    return count;
  }

  m_fixedSearchResults = 0;
  FillSearchResultsMarks(results);
  m_fixedSearchResults = count;

  // Setup viewport according to results.
  m2::AnyRectD viewport = m_currentModelView.GlobalRect();
  m2::PointD const center = viewport.Center();

  double minDistance = numeric_limits<double>::max();
  int minInd = -1;
  for (size_t i = 0; i < count; ++i)
  {
    Result const & r = results.GetResult(i);
    if (r.HasPoint())
    {
      double const dist = center.SquareLength(r.GetFeatureCenter());
      if (dist < minDistance)
      {
        minDistance = dist;
        minInd = static_cast<int>(i);
      }
    }
  }

  if (minInd != -1)
  {
    m2::PointD const pt = results.GetResult(minInd).GetFeatureCenter();

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

void Framework::FillSearchResultsMarks(search::Results const & results)
{
  UserMarkControllerGuard guard(m_bmManager, UserMarkType::SEARCH_MARK);
  guard.m_controller.SetIsVisible(true);
  guard.m_controller.SetIsDrawable(true);
  guard.m_controller.Clear(m_fixedSearchResults);

  size_t const count = results.GetCount();
  for (size_t i = 0; i < count; ++i)
  {
    search::Result const & r = results.GetResult(i);
    if (r.HasPoint())
    {
      SearchMarkPoint * mark = static_cast<SearchMarkPoint *>(guard.m_controller.CreateUserMark(r.GetFeatureCenter()));
      ASSERT_EQUAL(mark->GetMarkType(), UserMark::Type::SEARCH, ());
      if (r.GetResultType() == search::Result::RESULT_FEATURE)
        mark->m_foundFeatureID = r.GetFeatureID();
      mark->m_matchedName = r.GetString();
    }
  }
}

void Framework::CancelInteractiveSearch()
{
  UserMarkControllerGuard(m_bmManager, UserMarkType::SEARCH_MARK).m_controller.Clear();
  if (IsInteractiveSearchActive())
  {
    m_lastInteractiveSearchParams.Clear();
    CancelQuery(m_lastQueryHandle);
  }

  m_fixedSearchResults = 0;
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
  (void) MeasurementUtils::FormatDistance(d, distance);

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

void Framework::CreateDrapeEngine(ref_ptr<dp::OGLContextFactory> contextFactory, DrapeCreationParams && params)
{
  auto idReadFn = [this](df::MapDataProvider::TReadCallback<FeatureID> const & fn,
                         m2::RectD const & r, int scale) -> void
  {
    m_model.ForEachFeatureID(r, fn, scale);
  };

  auto featureReadFn = [this](df::MapDataProvider::TReadCallback<FeatureType> const & fn,
                              vector<FeatureID> const & ids) -> void
  {
    m_model.ReadFeatures(fn, ids);
  };

  auto isCountryLoadedByNameFn = bind(&Framework::IsCountryLoadedByName, this, _1);
  auto updateCurrentCountryFn = bind(&Framework::OnUpdateCurrentCountry, this, _1, _2);

  bool allow3d;
  bool allow3dBuildings;
  Load3dMode(allow3d, allow3dBuildings);

  df::DrapeEngine::Params p(contextFactory,
                            make_ref(&m_stringsBundle),
                            df::Viewport(0, 0, params.m_surfaceWidth, params.m_surfaceHeight),
                            df::MapDataProvider(idReadFn, featureReadFn, isCountryLoadedByNameFn, updateCurrentCountryFn),
                            params.m_visualScale, move(params.m_widgetsInitInfo),
                            make_pair(params.m_initialMyPositionState, params.m_hasMyPositionState),
                            allow3dBuildings, params.m_isChoosePositionMode,
                            params.m_isChoosePositionMode, GetSelectedFeatureTriangles(), params.m_isFirstLaunch,
                            m_routingSession.IsActive() && m_routingSession.IsFollowing());

  m_drapeEngine = make_unique_dp<df::DrapeEngine>(move(p));
  AddViewportListener([this](ScreenBase const & screen)
  {
    if (!screen.GlobalRect().EqualDxDy(m_currentModelView.GlobalRect(), 1.0E-4))
      UpdateUserViewportChanged();
    m_currentModelView = screen;
  });
  m_drapeEngine->SetTapEventInfoListener(bind(&Framework::OnTapEvent, this, _1));
  m_drapeEngine->SetUserPositionListener(bind(&Framework::OnUserPositionChanged, this, _1));
  OnSize(params.m_surfaceWidth, params.m_surfaceHeight);

  m_drapeEngine->SetMyPositionModeListener(m_myPositionListener);

  InvalidateUserMarks();

#ifdef OMIM_OS_ANDROID
  // In case of the engine reinitialization recover compass and location data
  // for correct my position state.
  if (m_lastCompassInfo != nullptr)
    OnCompassUpdate(*m_lastCompassInfo.release());
  if (m_lastGPSInfo != nullptr)
    OnLocationUpdate(*m_lastGPSInfo.release());
#endif

  Allow3dMode(allow3d, allow3dBuildings);
  LoadViewport();

  // In case of the engine reinitialization recover route.
  if (m_routingSession.IsActive())
  {
    InsertRoute(m_routingSession.GetRoute());
    if (allow3d && m_routingSession.IsFollowing())
      m_drapeEngine->EnablePerspective(kRotationAngle, kAngleFOV);
  }

  if (m_connectToGpsTrack)
    GpsTracker::Instance().Connect(bind(&Framework::OnUpdateGpsTrackPointsCallback, this, _1, _2));

  // In case of the engine reinitialization simulate the last tap to show selection mark.
  if (m_lastTapEvent)
  {
    place_page::Info info;
    ActivateMapSelection(false, OnTapEventImpl(*m_lastTapEvent, info), info);
  }
}

ref_ptr<df::DrapeEngine> Framework::GetDrapeEngine()
{
  return make_ref(m_drapeEngine);
}

void Framework::DestroyDrapeEngine()
{
  GpsTracker::Instance().Disconnect();
  m_drapeEngine.reset();
}

void Framework::SetRenderingEnabled(bool enable)
{
  m_isRenderingEnabled = enable;
  if (m_drapeEngine)
    m_drapeEngine->SetRenderingEnabled(enable);
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
    pt.m_id = ip.first;
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
  // Store current map style before classificator reloading
  settings::Set(kMapStyleKey, static_cast<uint32_t>(mapStyle));
  GetStyleReader().SetCurrentStyle(mapStyle);

  alohalytics::TStringMap details {{"mapStyle", strings::to_string(static_cast<int>(mapStyle))}};
  alohalytics::Stats::Instance().LogEvent("MapStyle_Changed", details);
}

void Framework::SetMapStyle(MapStyle mapStyle)
{
  MarkMapStyle(mapStyle);
  CallDrapeFunction(bind(&df::DrapeEngine::UpdateMapStyle, _1));
  InvalidateUserMarks();
  UpdateMinBuildingsTapZoom();
}

MapStyle Framework::GetMapStyle() const
{
  return GetStyleReader().GetCurrentStyle();
}

void Framework::SetupMeasurementSystem()
{
  GetPlatform().SetupMeasurementSystem();

  settings::Units units = settings::Metric;
  settings::Get(settings::kMeasurementUnits, units);

  m_routingSession.SetTurnNotificationsUnits(units);
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
  else if (StartsWith(url, "mapswithme://") || StartsWith(url, "mwm://"))
  {
    // clear every current API-mark.
    {
      UserMarkControllerGuard guard(m_bmManager, UserMarkType::API_MARK);
      guard.m_controller.Clear();
      guard.m_controller.SetIsVisible(true);
      guard.m_controller.SetIsDrawable(true);
    }

    if (m_ParsedMapApi.SetUriAndParse(url))
    {
      if (!m_ParsedMapApi.GetViewportRect(rect))
        rect = df::GetWorldRect();

      if ((apiMark = m_ParsedMapApi.GetSinglePoint()))
        result = NEED_CLICK;
      else
        result = NO_NEED_CLICK;
    }
    else
    {
      UserMarkControllerGuard guard(m_bmManager, UserMarkType::API_MARK);
      guard.m_controller.SetIsVisible(false);
    }
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
    DeactivateMapSelection(true);

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
        UserMarkContainer::UserMarkForPoi()->SetPtOrg(point);
        FillPointInfo(point, name, info);
        ActivateMapSelection(false, df::SelectionShape::OBJECT_POI, info);
      }
      m_lastTapEvent.reset(new df::TapInfo{ m_currentModelView.GtoP(info.GetMercator()), false, false, info.GetID() });
    }

    return true;
  }

  return false;
}

void Framework::ForEachFeatureAtPoint(TFeatureTypeFn && fn, m2::PointD const & mercator) const
{
  constexpr double kSelectRectWidthInMeters = 1.1;
  constexpr double kMetersToLinearFeature = 3;
  constexpr int kScale = scales::GetUpperScale();
  m2::RectD const rect = MercatorBounds::RectByCenterXYAndSizeInMeters(mercator, kSelectRectWidthInMeters);
  m_model.ForEachFeature(rect, [&](FeatureType & ft)
  {
    switch (ft.GetFeatureType())
    {
    case feature::GEOM_POINT:
      if (rect.IsPointInside(ft.GetCenter()))
        fn(ft);
      break;
    case feature::GEOM_LINE:
      if (feature::GetMinDistanceMeters(ft, mercator) < kMetersToLinearFeature)
        fn(ft);
      break;
    case feature::GEOM_AREA:
      if (ft.GetLimitRect(kScale).IsPointInside(mercator) &&
          feature::GetMinDistanceMeters(ft, mercator) == 0.0)
      {
        fn(ft);
      }
      break;
    case feature::GEOM_UNDEFINED:
      ASSERT(false, ("case feature::GEOM_UNDEFINED"));
      break;
    }
  }, kScale);
}

unique_ptr<FeatureType> Framework::GetFeatureAtPoint(m2::PointD const & mercator) const
{
  unique_ptr<FeatureType> poi, line, area;
  uint32_t const coastlineType = classif().GetCoastType();
  ForEachFeatureAtPoint([&, coastlineType](FeatureType & ft)
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

unique_ptr<FeatureType> Framework::GetFeatureByID(FeatureID const & fid, bool parse) const
{
  ASSERT(fid.IsValid(), ());

  unique_ptr<FeatureType> feature(new FeatureType);
  // Note: all parse methods should be called with guard alive.
  Index::FeaturesLoaderGuard guard(m_model.GetIndex(), fid.m_mwmId);
  guard.GetFeatureByIndex(fid.m_index, *feature);
  if (parse)
    feature->ParseEverything();
  return feature;
}

BookmarkAndCategory Framework::FindBookmark(UserMark const * mark) const
{
  BookmarkAndCategory empty = MakeEmptyBookmarkAndCategory();
  BookmarkAndCategory result = empty;
  ASSERT_LESS_OR_EQUAL(GetBmCategoriesCount(), numeric_limits<int>::max(), ());
  for (size_t i = 0; i < GetBmCategoriesCount(); ++i)
  {
    if (mark->GetContainer() == GetBmCategory(i))
    {
      result.first = static_cast<int>(i);
      break;
    }
  }

  ASSERT(result.first != empty.first, ());
  BookmarkCategory const * cat = GetBmCategory(result.first);
  ASSERT_LESS_OR_EQUAL(cat->GetUserMarkCount(), numeric_limits<int>::max(), ());
  for (size_t i = 0; i < cat->GetUserMarkCount(); ++i)
  {
    if (mark == cat->GetUserMark(i))
    {
      result.second = static_cast<int>(i);
      break;
    }
  }

  ASSERT(result != empty, ());
  return result;
}

void Framework::SetMapSelectionListeners(TActivateMapSelectionFn const & activator,
                                         TDeactivateMapSelectionFn const & deactivator)
{
  m_activateMapSelectionFn = activator;
  m_deactivateMapSelectionFn = deactivator;
}

void Framework::ActivateMapSelection(bool needAnimation, df::SelectionShape::ESelectedObject selectionType,
                                     place_page::Info const & info) const
{
  ASSERT_NOT_EQUAL(selectionType, df::SelectionShape::OBJECT_EMPTY, ("Empty selections are impossible."));
  m_selectedFeature = info.GetID();
  CallDrapeFunction(bind(&df::DrapeEngine::SelectObject, _1, selectionType, info.GetMercator(),
                         needAnimation));
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

  if (somethingWasAlreadySelected)
    CallDrapeFunction(bind(&df::DrapeEngine::DeselectObject, _1));
}

void Framework::UpdatePlacePageInfoForCurrentSelection()
{
  ASSERT(m_lastTapEvent, ());

  place_page::Info info;
  ActivateMapSelection(false, OnTapEventImpl(*m_lastTapEvent, info), info);
}

void Framework::InvalidateUserMarks()
{
  m_bmManager.InitBookmarks();

  vector<UserMarkType> const types = { UserMarkType::SEARCH_MARK, UserMarkType::API_MARK, UserMarkType::DEBUG_MARK };
  for (size_t typeIndex = 0; typeIndex < types.size(); typeIndex++)
  {
    UserMarkControllerGuard guard(m_bmManager, types[typeIndex]);
    guard.m_controller.Update();
  }
}

void Framework::OnTapEvent(df::TapInfo const & tapInfo)
{
  bool const somethingWasAlreadySelected = (m_lastTapEvent != nullptr);

  place_page::Info info;
  df::SelectionShape::ESelectedObject const selection = OnTapEventImpl(tapInfo, info);
  if (selection != df::SelectionShape::OBJECT_EMPTY)
  {
    // Back up last tap event to recover selection in case of Drape reinitialization.
    m_lastTapEvent.reset(new df::TapInfo(tapInfo));

    { // Log statistics event.
      ms::LatLon const ll = info.GetLatLon();
      double myLat, myLon;
      double metersToTap = -1;
      if (info.IsMyPosition())
        metersToTap = 0;
      else if (GetCurrentPosition(myLat, myLon))
        metersToTap = ms::DistanceOnEarth(myLat, myLon, ll.lat, ll.lon);

      alohalytics::TStringMap kv = {{"longTap", tapInfo.m_isLong ? "1" : "0"},
                                    {"title", info.GetTitle()},
                                    {"bookmark", info.IsBookmark() ? "1" : "0"},
                                    {"meters", strings::to_string_dac(metersToTap, 0)}};
      if (info.IsFeature())
        kv["types"] = DebugPrint(info.GetTypes());
      // Older version of statistics used "$GetUserMark" event.
      alohalytics::Stats::Instance().LogEvent("$SelectMapObject", kv, alohalytics::Location::FromLatLon(ll.lat, ll.lon));
    }

    ActivateMapSelection(true, selection, info);
  }
  else
  {
    alohalytics::Stats::Instance().LogEvent(somethingWasAlreadySelected ? "$DelectMapObject" : "$EmptyTapOnMap");
    // UI is always notified even if empty map is tapped,
    // because empty tap event switches on/off full screen map view mode.
    DeactivateMapSelection(true);
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

df::SelectionShape::ESelectedObject Framework::OnTapEventImpl(df::TapInfo const & tapInfo,
                                                              place_page::Info & outInfo) const
{
  m2::PointD const pxPoint2d = m_currentModelView.P3dtoP(tapInfo.m_pixelPoint);

  if (tapInfo.m_isMyPositionTapped)
  {
    FillMyPositionInfo(outInfo);
    return df::SelectionShape::OBJECT_MY_POSITION;
  }

  df::VisualParams const & vp = df::VisualParams::Instance();

  m2::AnyRectD rect;
  uint32_t const touchRadius = vp.GetTouchRectRadius();
  m_currentModelView.GetTouchRect(pxPoint2d, touchRadius, rect);

  m2::AnyRectD bmSearchRect;
  double const bmAddition = BM_TOUCH_PIXEL_INCREASE * vp.GetVisualScale();
  double const pxWidth = touchRadius;
  double const pxHeight = touchRadius + bmAddition;
  m_currentModelView.GetTouchRect(pxPoint2d + m2::PointD(0, bmAddition),
                                  pxWidth, pxHeight, bmSearchRect);
  UserMark const * mark = m_bmManager.FindNearestUserMark(
        [&rect, &bmSearchRect](UserMarkType type) -> m2::AnyRectD const &
        {
          return (type == UserMarkType::BOOKMARK_MARK ? bmSearchRect : rect);
        });

  if (mark)
  {
    switch (mark->GetMarkType())
    {
    case UserMark::Type::API:
      FillApiMarkInfo(*static_cast<ApiMarkPoint const *>(mark), outInfo);
      break;
    case UserMark::Type::BOOKMARK:
      FillBookmarkInfo(*static_cast<Bookmark const *>(mark), FindBookmark(mark), outInfo);
      break;
    case UserMark::Type::SEARCH:
      FillSearchResultInfo(*static_cast<SearchMarkPoint const *>(mark), outInfo);
      break;
    default:
      ASSERT(false, ("FindNearestUserMark returned invalid mark."));
    }
    return df::SelectionShape::OBJECT_USER_MARK;
  }

  FeatureID featureTapped = tapInfo.m_featureTapped;

  if (!featureTapped.IsValid())
    featureTapped = FindBuildingAtPoint(m_currentModelView.PtoG(pxPoint2d));

  bool showMapSelection = false;
  if (featureTapped.IsValid())
  {
    FillFeatureInfo(featureTapped, outInfo);
    showMapSelection = true;
  }
  else if (tapInfo.m_isLong)
  {
    FillPointInfo(m_currentModelView.PtoG(pxPoint2d), "", outInfo);
    showMapSelection = true;
  }

  if (showMapSelection)
  {
    UserMarkContainer::UserMarkForPoi()->SetPtOrg(outInfo.GetMercator());
    return df::SelectionShape::OBJECT_POI;
  }

  return df::SelectionShape::OBJECT_EMPTY;
}

void Framework::PredictLocation(double & lat, double & lon, double accuracy,
                                double bearing, double speed, double elapsedSeconds)
{
  double offsetInM = speed * elapsedSeconds;
  double angle = my::DegToRad(90.0 - bearing);

  m2::PointD mercatorPt = MercatorBounds::MetresToXY(lon, lat, accuracy).Center();
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
  return CodeGe0url(lat, lon, bmk->GetScale(), addName ? bmk->GetName() : "");
}

string Framework::CodeGe0url(double lat, double lon, double zoomLevel, string const & name)
{
  size_t const resultSize = MapsWithMe_GetMaxBufferSize(name.size());

  string res(resultSize, 0);
  int const len = MapsWithMe_GenShortShowMapUrl(lat, lon, zoomLevel, name.c_str(), &res[0], res.size());

  ASSERT_LESS_OR_EQUAL(len, res.size(), ());
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
    if (!point.GetID().empty())
      res += "&id=" + UrlEncode(point.GetID());
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

int64_t Framework::GetCurrentDataVersion()
{
  return m_storage.GetCurrentDataVersion();
}

void Framework::BuildRoute(m2::PointD const & finish, uint32_t timeoutSec)
{
  ASSERT_THREAD_CHECKER(m_threadChecker, ("BuildRoute"));
  ASSERT(m_drapeEngine != nullptr, ());

  m2::PointD start;
  if (!m_drapeEngine->GetMyPosition(start))
  {
    CallRouteBuilded(IRouter::NoCurrentPosition, storage::TCountriesVec());
    return;
  }
  BuildRoute(start, finish, timeoutSec);
}

void Framework::BuildRoute(m2::PointD const & start, m2::PointD const & finish, uint32_t timeoutSec)
{
  ASSERT_THREAD_CHECKER(m_threadChecker, ("BuildRoute"));
  ASSERT(m_drapeEngine != nullptr, ());

  if (IsRoutingActive())
    CloseRouting();

  SetLastUsedRouter(m_currentRouterType);
  m_routingSession.SetUserCurrentPosition(start);

  auto readyCallback = [this] (Route const & route, IRouter::ResultCode code)
  {
    storage::TCountriesVec absentCountries;
    if (code == IRouter::NoError)
    {
      double const kRouteScaleMultiplier = 1.5;

      InsertRoute(route);
      StopLocationFollow();
      m2::RectD routeRect = route.GetPoly().GetLimitRect();
      routeRect.Scale(kRouteScaleMultiplier);
      ShowRect(routeRect, -1);
    }
    else
    {
      absentCountries.assign(route.GetAbsentCountries().begin(), route.GetAbsentCountries().end());

      if (code != IRouter::NeedMoreMaps)
        RemoveRoute(true /* deactivateFollowing */);
    }
    CallRouteBuilded(code, absentCountries);
  };

  m_routingSession.BuildRoute(start, finish, readyCallback, m_progressCallback, timeoutSec);
}

void Framework::FollowRoute()
{
  ASSERT(m_drapeEngine != nullptr, ());

  if (!m_routingSession.EnableFollowMode())
    return;

  int const scale = (m_currentRouterType == RouterType::Pedestrian) ? scales::GetPedestrianNavigationScale()
                                                                    : scales::GetNavigationScale();
  int const scale3d = (m_currentRouterType == RouterType::Pedestrian) ? scales::GetPedestrianNavigation3dScale()
                                                                      : scales::GetNavigation3dScale();
  m_drapeEngine->FollowRoute(scale, scale3d, kRotationAngle, kAngleFOV);
  m_drapeEngine->SetRoutePoint(m2::PointD(), true /* isStart */, false /* isValid */);
}

bool Framework::DisableFollowMode()
{
  bool const disabled = m_routingSession.DisableFollowMode();
  if (disabled && m_drapeEngine != nullptr)
    m_drapeEngine->DeactivateRouteFollowing();

  return disabled;
}

void Framework::SetRouter(RouterType type)
{
  ASSERT_THREAD_CHECKER(m_threadChecker, ("SetRouter"));

  if (m_currentRouterType == type)
    return;
  SetRouterImpl(type);
}

routing::RouterType Framework::GetRouter() const
{
  return m_currentRouterType;
}

void Framework::SetRouterImpl(RouterType type)
{
  unique_ptr<IRouter> router;
  unique_ptr<OnlineAbsentCountriesFetcher> fetcher;

  auto countryFileGetter = [this](m2::PointD const & p) -> string
  {
    // TODO (@gorshenin): fix CountryInfoGetter to return CountryFile
    // instances instead of plain strings.
    return m_infoGetter->GetRegionCountryId(p);
  };

  if (type == RouterType::Pedestrian)
  {
    router = CreatePedestrianAStarBidirectionalRouter(m_model.GetIndex(), countryFileGetter);
    m_routingSession.SetRoutingSettings(routing::GetPedestrianRoutingSettings());
  }
  else if (type == RouterType::Bicycle)
  {
    router = CreateBicycleAStarBidirectionalRouter(m_model.GetIndex(), countryFileGetter);
    m_routingSession.SetRoutingSettings(routing::GetBicycleRoutingSettings());
  }
  else
  {
    auto localFileChecker = [this](string const & countryFile) -> bool
    {
      return m_model.GetIndex().GetMwmIdByCountryFile(CountryFile(countryFile)).IsAlive();
    };

    router.reset(new OsrmRouter(&m_model.GetIndex(), countryFileGetter));
    fetcher.reset(new OnlineAbsentCountriesFetcher(countryFileGetter, localFileChecker));
    m_routingSession.SetRoutingSettings(routing::GetCarRoutingSettings());
  }

  m_routingSession.SetRouter(move(router), move(fetcher));
  m_currentRouterType = type;
}

void Framework::RemoveRoute(bool deactivateFollowing)
{
  if (m_drapeEngine != nullptr)
    m_drapeEngine->RemoveRoute(deactivateFollowing);
}

void Framework::CloseRouting()
{
  if (m_routingSession.IsBuilt())
  {
    m_routingSession.EmitCloseRoutingEvent();
  }
  m_routingSession.Reset();
  RemoveRoute(true /* deactivateFollowing */);
}

void Framework::InsertRoute(Route const & route)
{
  if (m_drapeEngine == nullptr)
    return;

  if (route.GetPoly().GetSize() < 2)
  {
    LOG(LWARNING, ("Invalid track - only", route.GetPoly().GetSize(), "point(s)."));
    return;
  }

  vector<double> turns;
  if (m_currentRouterType == RouterType::Vehicle)
    route.GetTurnsDistances(turns);

  df::ColorConstant const routeColor = (m_currentRouterType == RouterType::Pedestrian) ?
                                        df::RoutePedestrian : df::Route;
  m_drapeEngine->AddRoute(route.GetPoly(), turns, routeColor);
}

void Framework::CheckLocationForRouting(GpsInfo const & info)
{
  if (!IsRoutingActive())
    return;

  RoutingSession::State state = m_routingSession.OnLocationPositionChanged(info, m_model.GetIndex());
  if (state == RoutingSession::RouteNeedRebuild)
  {
    auto readyCallback = [this] (Route const & route, IRouter::ResultCode code)
    {
      if (code == IRouter::NoError)
      {
        RemoveRoute(false /* deactivateFollowing */);
        InsertRoute(route);
      }
    };

    m_routingSession.RebuildRoute(MercatorBounds::FromLatLon(info.m_latitude, info.m_longitude),
                                  readyCallback, m_progressCallback, 0 /* timeoutSec */);
  }
}

void Framework::MatchLocationToRoute(location::GpsInfo & location, location::RouteMatchingInfo & routeMatchingInfo) const
{
  if (!IsRoutingActive())
    return;

  m_routingSession.MatchLocationToRoute(location, routeMatchingInfo);
}

void Framework::CallRouteBuilded(IRouter::ResultCode code, storage::TCountriesVec const & absentCountries)
{
  if (code == IRouter::Cancelled)
    return;
  m_routingCallback(code, absentCountries);
}

string Framework::GetRoutingErrorMessage(IRouter::ResultCode code)
{
  string messageID = "";
  switch (code)
  {
  case IRouter::NoCurrentPosition:
    messageID = "routing_failed_unknown_my_position";
    break;
  case IRouter::InconsistentMWMandRoute: // the same as RouteFileNotExist
  case IRouter::RouteFileNotExist:
    messageID = "routing_failed_has_no_routing_file";
    break;
  case IRouter::StartPointNotFound:
    messageID = "routing_failed_start_point_not_found";
    break;
  case IRouter::EndPointNotFound:
    messageID = "routing_failed_dst_point_not_found";
    break;
  case IRouter::PointsInDifferentMWM:
    messageID = "routing_failed_cross_mwm_building";
    break;
  case IRouter::RouteNotFound:
    messageID = "routing_failed_route_not_found";
    break;
  case IRouter::InternalError:
    messageID = "routing_failed_internal_error";
    break;
  default:
    ASSERT(false, ());
  }

  return m_stringsBundle.GetString(messageID);
}

RouterType Framework::GetBestRouter(m2::PointD const & startPoint, m2::PointD const & finalPoint)
{
  if (MercatorBounds::DistanceOnEarth(startPoint, finalPoint) < kKeepPedestrianDistanceMeters)
  {
    if (GetLastUsedRouter() == RouterType::Pedestrian)
      return RouterType::Pedestrian;

    // Return on a short distance the vehicle router flag only if we are already have routing files.
    auto countryFileGetter = [this](m2::PointD const & pt)
    {
      return m_infoGetter->GetRegionCountryId(pt);
    };
    if (!OsrmRouter::CheckRoutingAbility(startPoint, finalPoint, countryFileGetter,
                                         &m_model.GetIndex()))
    {
      return RouterType::Pedestrian;
    }
  }
  return RouterType::Vehicle;
}

RouterType Framework::GetLastUsedRouter() const
{
  string routerType;
  settings::Get(kRouterTypeKey, routerType);
  return (routerType == routing::ToString(RouterType::Pedestrian) ? RouterType::Pedestrian : RouterType::Vehicle);
}

void Framework::SetLastUsedRouter(RouterType type)
{
  settings::Set(kRouterTypeKey, routing::ToString(type));
}

void Framework::SetRouteStartPoint(m2::PointD const & pt, bool isValid)
{
  if (m_drapeEngine != nullptr)
    m_drapeEngine->SetRoutePoint(pt, true /* isStart */, isValid);
}

void Framework::SetRouteFinishPoint(m2::PointD const & pt, bool isValid)
{
  if (m_drapeEngine != nullptr)
    m_drapeEngine->SetRoutePoint(pt, false /* isStart */, isValid);
}

void Framework::Allow3dMode(bool allow3d, bool allow3dBuildings)
{
  CallDrapeFunction(bind(&df::DrapeEngine::Allow3dMode, _1, allow3d, allow3dBuildings, kRotationAngle, kAngleFOV));
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

void Framework::EnableChoosePositionMode(bool enable, bool enableBounds, bool applyPosition, m2::PointD const & position)
{
  if (m_drapeEngine != nullptr)
    m_drapeEngine->EnableChoosePositionMode(enable, enableBounds ? GetSelectedFeatureTriangles() : vector<m2::TriangleD>(),
                                            applyPosition, position);
}

vector<m2::TriangleD> Framework::GetSelectedFeatureTriangles() const
{
  vector<m2::TriangleD> triangles;
  if (m_selectedFeature.IsValid())
  {
    Index::FeaturesLoaderGuard const guard(m_model.GetIndex(), m_selectedFeature.m_mwmId);
    FeatureType ft;
    guard.GetFeatureByIndex(m_selectedFeature.m_index, ft);
    if (ftypes::IsBuildingChecker::Instance()(feature::TypesHolder(ft)))
    {
      triangles.reserve(10);
      ft.ForEachTriangle([&](m2::PointD const & p1, m2::PointD const & p2, m2::PointD const & p3)
      {
        triangles.push_back(m2::TriangleD(p1, p2, p3));
      }, scales::GetUpperScale());
    }
    m_selectedFeature = FeatureID();
  }
  return triangles;
}

void Framework::BlockTapEvents(bool block)
{
  CallDrapeFunction(bind(&df::DrapeEngine::BlockTapEvents, _1, block));
}

namespace feature
{
string GetPrintableTypes(FeatureType const & ft)
{
  return DebugPrint(feature::TypesHolder(ft));
}
uint32_t GetBestType(FeatureType const & ft)
{
  return feature::TypesHolder(ft).GetBestType();
}
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
      auto const feature = GetFeatureByID(fid);
      string name;
      feature->GetReadableName(name);
      feature::TypesHolder const types(*feature);
      search::Result::Metadata smd;
      results.AddResultNoChecks(search::Result(fid, feature::GetCenter(*feature), name, edit.second,
                                               DebugPrint(types), types.GetBestType(), smd));
    }
    params.m_onResults(results);
    params.m_onResults(search::Results::GetEndMarker(false));
    return true;
  }
  else if (params.m_query == "?eclear")
  {
    osm::Editor::Instance().ClearAllLocalEdits();
    return true;
  }
  return false;
}

namespace
{
osm::LocalizedStreet LocalizeStreet(Index const & index, FeatureID const & fid)
{
  osm::LocalizedStreet result;
  Index::FeaturesLoaderGuard g(index, fid.m_mwmId);
  FeatureType ft;
  g.GetFeatureByIndex(fid.m_index, ft);
  ft.GetPreferredNames(result.m_defaultName, result.m_localizedName);
  return result;
}

vector<osm::LocalizedStreet> TakeSomeStreetsAndLocalize(
    vector<search::ReverseGeocoder::Street> const & streets, Index const & index)

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

    results.push_back(LocalizeStreet(index, street.m_id));
    if (results.size() >= kMaxNumberOfNearbyStreetsToDisplay)
      break;
  }
  return results;
}

void SetStreet(search::ReverseGeocoder const & coder, Index const & index,
               FeatureType & ft, osm::EditableMapObject & emo)
{
  auto const & editor = osm::Editor::Instance();

  if (editor.GetFeatureStatus(emo.GetID()) == osm::Editor::FeatureStatus::Created)
  {
    string street;
    VERIFY(editor.GetEditedFeatureStreet(emo.GetID(), street), ("Feature is in editor."));
    emo.SetStreet({street, ""});
    return;
  }

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

  auto localizedStreets = TakeSomeStreetsAndLocalize(streetsPool, index);

  if (!street.empty())
  {
    auto it = find_if(begin(streetsPool), end(streetsPool),
                      [&street](search::ReverseGeocoder::Street const & s)
                      {
                        return s.m_name == street;
                      });

    if (it != end(streetsPool))
    {
      auto const localizedStreet = LocalizeStreet(index, it->m_id);
      emo.SetStreet(localizedStreet);

      // A street that a feature belongs to should alwas be in the first place in the list.
      auto localizedIt = find(begin(localizedStreets), end(localizedStreets), localizedStreet);
      if (localizedIt != end(localizedStreets))
        iter_swap(localizedIt, begin(localizedStreets));
      else
        localizedStreets.insert(begin(localizedStreets), localizedStreet);
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

void SetHostingBuildingAddress(FeatureID const & hostingBuildingFid, Index const & index,
                               search::ReverseGeocoder const & coder, osm::EditableMapObject & emo)
{
  if (!hostingBuildingFid.IsValid())
    return;

  FeatureType hostingBuildingFeature;

  Index::FeaturesLoaderGuard g(index, hostingBuildingFid.m_mwmId);
  g.GetFeatureByIndex(hostingBuildingFid.m_index, hostingBuildingFeature);

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

bool Framework::CreateMapObject(m2::PointD const & mercator, uint32_t const featureType,
                                osm::EditableMapObject & emo) const
{
  emo = {};
  MwmSet::MwmId const mwmId = m_model.GetIndex().GetMwmIdByCountryFile(
        platform::CountryFile(m_infoGetter->GetRegionCountryId(mercator)));
  if (!mwmId.IsAlive())
    return false;

  search::ReverseGeocoder const coder(m_model.GetIndex());
  vector<search::ReverseGeocoder::Street> streets;

  coder.GetNearbyStreets(mwmId, mercator, streets);
  emo.SetNearbyStreets(TakeSomeStreetsAndLocalize(streets, m_model.GetIndex()));
  return osm::Editor::Instance().CreatePoint(featureType, mercator, mwmId, emo);
}

bool Framework::GetEditableMapObject(FeatureID const & fid, osm::EditableMapObject & emo) const
{
  if (!fid.IsValid())
    return false;

  auto feature = GetFeatureByID(fid);
  FeatureType & ft = *feature;
  emo.SetFromFeatureType(ft);
  emo.SetHouseNumber(ft.GetHouseNumber());
  auto const & editor = osm::Editor::Instance();
  emo.SetEditableProperties(editor.GetEditableProperties(ft));

  auto const & index = m_model.GetIndex();
  search::ReverseGeocoder const coder(index);
  SetStreet(coder, index, ft, emo);

  if (!ftypes::IsBuildingChecker::Instance()(ft) &&
      (emo.GetHouseNumber().empty() || emo.GetStreet().m_defaultName.empty()))
  {
    SetHostingBuildingAddress(FindBuildingAtPoint(feature::GetCenter(ft)),
                              index, coder, emo);
  }

  return true;
}

osm::Editor::SaveResult Framework::SaveEditedMapObject(osm::EditableMapObject emo)
{
  if (!m_lastTapEvent)
  {
    // Automatically select newly created objects.
    m_lastTapEvent.reset(new df::TapInfo { m_currentModelView.GtoP(emo.GetMercator()), false, false, emo.GetID() });
  }

  auto & editor = osm::Editor::Instance();

  ms::LatLon issueLatLon;

  auto shouldNotify = false;
  // Notify if a poi address and it's hosting building address differ.
  do
  {
    auto const isCreatedFeature = editor.IsCreatedFeature(emo.GetID());

    search::ReverseGeocoder::Address hostingBuildingAddress;
    Index::FeaturesLoaderGuard g(m_model.GetIndex(), emo.GetID().m_mwmId);
    FeatureType originalFeature;

    if (!isCreatedFeature)
      g.GetOriginalFeatureByIndex(emo.GetID().m_index, originalFeature);
    else
      originalFeature.ReplaceBy(emo);

    // Handle only pois.
    if (ftypes::IsBuildingChecker::Instance()(originalFeature))
      break;

    auto const hostingBuildingFid = FindBuildingAtPoint(feature::GetCenter(originalFeature));
    // The is no building to take address from. Fallback to simple saving.
    if (!hostingBuildingFid.IsValid())
      break;

    FeatureType hostingBuildingFeature;
    g.GetFeatureByIndex(hostingBuildingFid.m_index, hostingBuildingFeature);
    issueLatLon = MercatorBounds::ToLatLon(feature::GetCenter(hostingBuildingFeature));

    search::ReverseGeocoder const coder(m_model.GetIndex());
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
    editor.CreateNote(issueLatLon, emo.GetID(), osm::Editor::NoteProblemType::General,
                      "The address on this POI is different from the building address."
                      " It is either a user's mistake, or an issue in the data. Please"
                      " check this and fix if needed. (This note was created automatically"
                      " without a user's input. Feel free to close it if it's wrong).");
  }

  return osm::Editor::Instance().SaveEditedFeature(emo);
}

void Framework::DeleteFeature(FeatureID const & fid) const
{
  // TODO(AlexZ): Use FeatureID in the editor interface.
  osm::Editor::Instance().DeleteFeature(*GetFeatureByID(fid));
}

osm::NewFeatureCategories Framework::GetEditorCategories() const
{
  return osm::Editor::Instance().GetNewFeatureCategories();
}
