#include "map/framework.hpp"

#include "map/ge0_parser.hpp"
#include "map/geourl_process.hpp"
#include "map/gps_tracker.hpp"
#include "map/storage_bridge.hpp"

#include "defines.hpp"

#include "routing/online_absent_fetcher.hpp"
#include "routing/osrm_router.hpp"
#include "routing/road_graph_router.hpp"
#include "routing/route.hpp"
#include "routing/routing_algorithm.hpp"

#include "search/intermediate_result.hpp"
#include "search/result.hpp"
#include "search/search_engine.hpp"
#include "search/search_query_factory.hpp"

#include "drape_frontend/color_constants.hpp"
#include "drape_frontend/gps_track_point.hpp"
#include "drape_frontend/gui/country_status_helper.hpp"
#include "drape_frontend/visual_params.hpp"
#include "drape_frontend/gui/country_status_helper.hpp"
#include "drape_frontend/watch/cpu_drawer.hpp"
#include "drape_frontend/watch/feature_processor.hpp"

#include "indexer/categories_holder.hpp"
#include "indexer/classificator_loader.hpp"
#include "indexer/drawing_rules.hpp"
#include "indexer/feature.hpp"
#include "indexer/map_style_reader.hpp"
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

}  // namespace

pair<MwmSet::MwmId, MwmSet::RegResult> Framework::RegisterMap(
    LocalCountryFile const & localFile)
{
  LOG(LINFO, ("Loading map:", localFile.GetCountryName()));
  return m_model.RegisterMap(localFile);
}

void Framework::OnLocationError(TLocationError /*error*/)
{
  CallDrapeFunction(bind(&df::DrapeEngine::CancelMyPosition, _1));
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
  CallDrapeFunction(bind(&df::DrapeEngine::MyPositionNextMode, _1));
}

void Framework::InvalidateMyPosition()
{
  CallDrapeFunction(bind(&df::DrapeEngine::InvalidateMyPosition, _1));
}

void Framework::SetMyPositionModeListener(location::TMyPositionModeChanged const & fn)
{
  m_myPositionListener = fn;
}

void Framework::OnUserPositionChanged(m2::PointD const & position)
{
  MyPositionMarkPoint * myPosition = UserMarkContainer::UserMarkForMyPostion();
  myPosition->SetUserPosition(position);

  if (IsRoutingActive())
    m_routingSession.SetUserCurrentPosition(position);
}

void Framework::CallDrapeFunction(TDrapeFunction const & fn)
{
  if (m_drapeEngine)
    fn(m_drapeEngine.get());
}

void Framework::StopLocationFollow()
{
  CallDrapeFunction(bind(&df::DrapeEngine::StopLocationFollow, _1));
}

Framework::Framework()
  : m_bmManager(*this)
  , m_fixedSearchResults(0)
{
  m_activeMaps.reset(new ActiveMapsLayout(*this));
  m_globalCntTree = storage::CountryTree(m_activeMaps);
  m_storageBridge = make_unique_dp<StorageBridge>(m_activeMaps, bind(&Framework::UpdateCountryInfo, this, _1, false));

  // Restore map style before classificator loading
  int mapStyle = MapStyleLight;
  if (!Settings::Get(kMapStyleKey, mapStyle))
    mapStyle = MapStyleClear;
  GetStyleReader().SetCurrentStyle(static_cast<MapStyle>(mapStyle));

  m_connectToGpsTrack = GpsTracker::Instance().IsEnabled();

  m_ParsedMapApi.SetBookmarkManager(&m_bmManager);

  // Init strings bundle.
  // @TODO. There are hardcoded strings below which are defined in strings.txt as well.
  // It's better to use strings form strings.txt intead of hardcoding them here.
  m_stringsBundle.SetDefaultString("country_status_added_to_queue", "^\nis added to the downloading queue");
  m_stringsBundle.SetDefaultString("country_status_downloading", "Downloading\n^\n^");
  m_stringsBundle.SetDefaultString("country_status_download", "Download map\n(^ ^)");
  m_stringsBundle.SetDefaultString("country_status_download_failed", "Downloading\n^\nhas failed");
  m_stringsBundle.SetDefaultString("country_status_download_without_routing", "Download map\nwithout routing (^ ^)");
  m_stringsBundle.SetDefaultString("try_again", "Try Again");
  m_stringsBundle.SetDefaultString("not_enough_free_space_on_sdcard", "Not enough space for downloading");

  m_stringsBundle.SetDefaultString("dropped_pin", "Dropped Pin");
  m_stringsBundle.SetDefaultString("my_places", "My Places");
  m_stringsBundle.SetDefaultString("my_position", "My Position");
  m_stringsBundle.SetDefaultString("routes", "Routes");

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
  m_storage.Init(bind(&Framework::UpdateLatestCountryFile, this, _1));
  LOG(LDEBUG, ("Storage initialized"));

  auto const routingStatisticsFn = [](map<string, string> const & statistics)
  {
    alohalytics::LogEvent("Routing_CalculatingRoute", statistics);
  };
//#ifdef DEBUG
//  auto const routingVisualizerFn = [this](m2::PointD const & pt)
//  {
//    GetPlatform().RunOnGuiThread([this,pt]()
//    {
//      m_bmManager.UserMarksGetController(UserMarkContainer::DEBUG_MARK).CreateUserMark(pt);
//      Invalidate();
//    });
//  };
//#else
  routing::RouterDelegate::TPointCheckCallback const routingVisualizerFn = nullptr;
//#endif
  m_routingSession.Init(routingStatisticsFn, routingVisualizerFn);

  SetRouterImpl(RouterType::Vehicle);

  LOG(LDEBUG, ("Routing engine initialized"));

  LOG(LINFO, ("System languages:", languages::GetPreferred()));
}

Framework::~Framework()
{
  m_drapeEngine.reset();

  m_storageBridge.reset();
  m_activeMaps.reset();
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

void Framework::DeleteCountry(storage::TIndex const & index, MapOptions opt)
{
  switch (opt)
  {
    case MapOptions::Nothing:
      return;
    case MapOptions::Map:  // fall through
    case MapOptions::MapWithCarRouting:
    {
      CountryFile const & countryFile = m_storage.GetCountryFile(index);
      // m_model will notify us when latest map file will be deleted via
      // OnMapDeregistered call.
      if (m_model.DeregisterMap(countryFile))
        InvalidateRect(GetCountryBounds(countryFile.GetNameWithoutExt()));

      // TODO (@ldragunov, @gorshenin): rewrite routing session to use MwmHandles. Thus,
      // it won' be needed to reset it after maps update.
      m_routingSession.Reset();
      return;
    }
    case MapOptions::CarRouting:
      m_routingSession.Reset();
      m_storage.DeleteCountry(index, opt);
      return;
  }
}

void Framework::DownloadCountry(TIndex const & index, MapOptions opt)
{
  m_storage.DownloadCountry(index, opt);
}

TStatus Framework::GetCountryStatus(TIndex const & index) const
{
  return m_storage.CountryStatusEx(index);
}

string Framework::GetCountryName(TIndex const & index) const
{
  string group, name;
  m_storage.GetGroupAndCountry(index, group, name);
  return (!group.empty() ? group + ", " + name : name);
}

m2::RectD Framework::GetCountryBounds(string const & file) const
{
  m2::RectD const bounds = m_infoGetter->CalcLimitRect(file);
  ASSERT(bounds.IsValid(), ());
  return bounds;
}

m2::RectD Framework::GetCountryBounds(TIndex const & index) const
{
  CountryFile const & file = m_storage.GetCountryFile(index);
  return GetCountryBounds(file.GetNameWithoutExt());
}

void Framework::ShowCountry(TIndex const & index)
{
  StopLocationFollow();

  ShowRect(GetCountryBounds(index));
}

void Framework::UpdateLatestCountryFile(LocalCountryFile const & localFile)
{
  // Soft reset to signal that mwm file may be out of date in routing caches.
  m_routingSession.Reset();

  if (!HasOptions(localFile.GetFiles(), MapOptions::Map))
    return;

  // Add downloaded map.
  auto p = m_model.RegisterMap(localFile);
  MwmSet::MwmId const & id = p.first;
  if (id.IsAlive())
    InvalidateRect(id.GetInfo()->m_limitRect);

  m_searchEngine->ClearViewportsCache();
}

void Framework::OnMapDeregistered(platform::LocalCountryFile const & localFile)
{
  m_storage.DeleteCustomCountryVersion(localFile);
}

void Framework::RegisterAllMaps()
{
  ASSERT(!m_storage.IsDownloadInProgress(),
         ("Registering maps while map downloading leads to removing downloading maps from "
          "ActiveMapsListener::m_items."));

  m_storage.RegisterAllLocalMaps();

  int minFormat = numeric_limits<int>::max();

  vector<shared_ptr<LocalCountryFile>> maps;
  m_storage.GetLocalMaps(maps);
  for (auto const & localFile : maps)
  {
    auto p = RegisterMap(*localFile);
    if (p.second != MwmSet::RegResult::Success)
      continue;

    MwmSet::MwmId const & id = p.first;
    ASSERT(id.IsAlive(), ());
    minFormat = min(minFormat, static_cast<int>(id.GetInfo()->m_version.format));
  }

  m_activeMaps->Init(maps);

  m_searchEngine->SupportOldFormat(minFormat < version::v3);
}

void Framework::DeregisterAllMaps()
{
  m_activeMaps->Clear();
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

void Framework::ShowBookmark(BookmarkAndCategory const & bnc)
{
  StopLocationFollow();

  // show ballon above
  Bookmark const * mark = static_cast<Bookmark const *>(GetBmCategory(bnc.first)->GetUserMark(bnc.second));

  double scale = mark->GetScale();
  if (scale == -1.0)
    scale = scales::GetUpperComfortScale();

  CallDrapeFunction(bind(&df::DrapeEngine::SetModelViewCenter, _1, mark->GetPivot(), scale, true));
  ActivateUserMark(mark, true);
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

void Framework::SaveState()
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
  Settings::Set("ScreenClipRect", rect);
}

void Framework::LoadState()
{
  m2::AnyRectD rect;
  if (Settings::Get("ScreenClipRect", rect) && df::GetWorldRect().IsRectInside(rect.GetGlobalRect()))
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
  CallDrapeFunction(bind(&df::DrapeEngine::SetModelViewCenter, _1, pt, -1, true));
}

m2::RectD Framework::GetCurrentViewport() const
{
  return m_currentModelView.ClipRect();
}

void Framework::ShowRect(double lat, double lon, double zoom)
{
  m2::PointD center(MercatorBounds::FromLatLon(lat, lon));
  CallDrapeFunction(bind(&df::DrapeEngine::SetModelViewCenter, _1, center, zoom, true));
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
  string const fName = m_infoGetter->GetRegionFile(pt);
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

  m_lastSearch = params;
  m_lastSearch.SetForceSearch(false);
  m_lastSearch.SetSearchMode(SearchParams::IN_VIEWPORT_ONLY | SearchParams::SEARCH_WORLD);
  m_lastSearch.m_callback = [this](Results const & results)
  {
    if (!results.IsEndMarker())
    {
      GetPlatform().RunOnGuiThread([this, results]()
      {
        if (IsISActive())
          FillSearchResultsMarks(results);
      });
    }
  };
}

void Framework::UpdateUserViewportChanged()
{
  if (IsISActive())
  {
    (void)GetCurrentPosition(m_lastSearch.m_lat, m_lastSearch.m_lon);

    m_searchEngine->Search(m_lastSearch, GetCurrentViewport());
  }
}

void Framework::ClearAllCaches()
{
  m_model.ClearCaches();
  m_infoGetter->ClearCaches();
  m_searchEngine->ClearAllCaches();
}

void Framework::SetDownloadCountryListener(TDownloadCountryListener const & listener)
{
  m_downloadCountryListener = listener;
}

void Framework::OnDownloadMapCallback(storage::TIndex const & countryIndex)
{
  if (m_downloadCountryListener != nullptr)
    m_downloadCountryListener(countryIndex, static_cast<int>(MapOptions::Map));
  else
    m_activeMaps->DownloadMap(countryIndex, MapOptions::Map);
}

void Framework::OnDownloadMapRoutingCallback(storage::TIndex const & countryIndex)
{
  if (m_downloadCountryListener != nullptr)
    m_downloadCountryListener(countryIndex, static_cast<int>(MapOptions::MapWithCarRouting));
  else
    m_activeMaps->DownloadMap(countryIndex, MapOptions::MapWithCarRouting);
}

void Framework::OnDownloadRetryCallback(storage::TIndex const & countryIndex)
{
  if (m_downloadCountryListener != nullptr)
    m_downloadCountryListener(countryIndex, -1);
  else
    m_activeMaps->RetryDownloading(countryIndex);
}

void Framework::OnUpdateCountryIndex(storage::TIndex const & currentIndex, m2::PointF const & pt)
{
  storage::TIndex newCountryIndex = GetCountryIndex(m2::PointD(pt));
  if (!newCountryIndex.IsValid())
  {
    m_drapeEngine->SetInvalidCountryInfo();
    return;
  }

  if (currentIndex != newCountryIndex)
    UpdateCountryInfo(newCountryIndex, true /* isCurrentCountry */);
}

void Framework::UpdateCountryInfo(storage::TIndex const & countryIndex, bool isCurrentCountry)
{
  ASSERT(m_activeMaps != nullptr, ());

  if (!m_drapeEngine)
    return;

  string const & fileName = m_storage.CountryByIndex(countryIndex).GetFile().GetNameWithoutExt();
  if (m_model.IsLoaded(fileName))
  {
    m_drapeEngine->SetInvalidCountryInfo();
    return;
  }

  gui::CountryInfo countryInfo;

  countryInfo.m_countryIndex = countryIndex;
  countryInfo.m_currentCountryName = m_activeMaps->GetFormatedCountryName(countryIndex);
  countryInfo.m_mapSize = m_activeMaps->GetRemoteCountrySizes(countryIndex).first;
  countryInfo.m_routingSize = m_activeMaps->GetRemoteCountrySizes(countryIndex).second;
  countryInfo.m_countryStatus = m_activeMaps->GetCountryStatus(countryIndex);
  if (countryInfo.m_countryStatus == storage::TStatus::EDownloading)
  {
    storage::LocalAndRemoteSizeT progress = m_activeMaps->GetDownloadableCountrySize(countryIndex);
    countryInfo.m_downloadProgress = progress.first * 100 / progress.second;
  }

  m_drapeEngine->SetCountryInfo(countryInfo, isCurrentCountry);
}

void Framework::MemoryWarning()
{
  LOG(LINFO, ("MemoryWarning"));
  ClearAllCaches();
  SharedBufferManager::instance().clearReserved();
}

void Framework::EnterBackground()
{
  const ms::LatLon ll = MercatorBounds::ToLatLon(GetViewportCenter());
  alohalytics::Stats::Instance().LogEvent("Framework::EnterBackground", {{"zoom", strings::to_string(GetDrawScale())},
                                          {"foregroundSeconds", strings::to_string(
                                           static_cast<int>(my::Timer::LocalTime() - m_startForegroundTime))}},
                                          alohalytics::Location::FromLatLon(ll.lat, ll.lon));
  // Do not clear caches for Android. This function is called when main activity is paused,
  // but at the same time search activity (for example) is enabled.
  // TODO(AlexZ): Use onStart/onStop on Android to correctly detect app background and remove #ifndef.
#ifndef OMIM_OS_ANDROID
  ClearAllCaches();
#endif

  if (m_drapeEngine != nullptr)
    m_drapeEngine->SetRenderingEnabled(false);
}

void Framework::EnterForeground()
{
  m_startForegroundTime = my::Timer::LocalTime();

  // Drape can be not initialized here in case of the first launch
  if (m_drapeEngine != nullptr)
    m_drapeEngine->SetRenderingEnabled(true);
}

void Framework::InitCountryInfoGetter()
{
  ASSERT(!m_infoGetter.get(), ("InitCountryInfoGetter() must be called only once."));
  Platform const & platform = GetPlatform();
  try
  {
    m_infoGetter.reset(new storage::CountryInfoGetter(platform.GetReader(PACKED_POLYGONS_FILE),
                                                      platform.GetReader(COUNTRIES_FILE)));
  }
  catch (RootException const & e)
  {
    LOG(LCRITICAL, ("Can't load needed resources for storage::CountryInfoGetter:", e.Msg()));
  }
}

void Framework::InitSearchEngine()
{
  ASSERT(!m_searchEngine.get(), ("InitSearchEngine() must be called only once."));
  ASSERT(m_infoGetter.get(), ());
  Platform const & platform = GetPlatform();

  try
  {
    m_searchEngine.reset(new search::Engine(
        const_cast<Index &>(m_model.GetIndex()), platform.GetReader(SEARCH_CATEGORIES_FILE_NAME),
        *m_infoGetter, languages::GetCurrentOrig(), make_unique<search::SearchQueryFactory>()));
  }
  catch (RootException const & e)
  {
    LOG(LCRITICAL, ("Can't load needed resources for search::Engine:", e.Msg()));
  }
}

TIndex Framework::GetCountryIndex(m2::PointD const & pt) const
{
  return m_storage.FindIndexByFile(m_infoGetter->GetRegionFile(pt));
}

string Framework::GetCountryName(m2::PointD const & pt) const
{
  storage::CountryInfo info;
  m_infoGetter->GetRegionInfo(pt, info);
  return info.m_name;
}

string Framework::GetCountryName(string const & id) const
{
  storage::CountryInfo info;
  m_infoGetter->GetRegionInfo(id, info);
  return info.m_name;
}

void Framework::PrepareSearch()
{
  m_searchEngine->PrepareSearch(GetCurrentViewport());
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

  return m_searchEngine->Search(rParams, GetCurrentViewport());
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

void Framework::LoadSearchResultMetadata(search::Result & res) const
{
  if (res.m_metadata.m_isInitialized)
    return;

  FeatureID const id = res.GetFeatureID();
  if (id.IsValid())
  {
    Index::FeaturesLoaderGuard loader(m_model.GetIndex(), id.m_mwmId);

    FeatureType ft;
    loader.GetFeatureByIndex(id.m_index, ft);

    search::ProcessMetadata(ft, res.m_metadata);
  }
  res.m_metadata.m_isInitialized = true;
}

void Framework::ShowSearchResult(search::Result const & res)
{
  CancelInteractiveSearch();

  UserMarkControllerGuard guard(m_bmManager, UserMarkType::SEARCH_MARK);
  guard.m_controller.SetIsDrawable(false);
  guard.m_controller.Clear();
  guard.m_controller.SetIsVisible(true);

  int scale;
  m2::PointD center;

  using namespace search;
  using namespace feature;

  alohalytics::TStringMap const stats = {{"pos", strings::to_string(res.GetPositionInResults())},
                                         {"result", res.ToStringForStats()}};
  alohalytics::LogEvent("searchShowResult", stats);

  switch (res.GetResultType())
  {
    case Result::RESULT_FEATURE:
    {
      FeatureID const id = res.GetFeatureID();
      Index::FeaturesLoaderGuard guard(m_model.GetIndex(), id.m_mwmId);

      FeatureType ft;
      guard.GetFeatureByIndex(id.m_index, ft);

      scale = GetFeatureViewportScale(TypesHolder(ft));
      center = GetCenter(ft, scale);
      break;
    }

    case Result::RESULT_LATLON:
    case Result::RESULT_ADDRESS:
      scale = scales::GetUpperComfortScale();
      center = res.GetFeatureCenter();
      break;

    default:
      return;
  }

  StopLocationFollow();
  if (m_currentModelView.isPerspective())
    CallDrapeFunction(bind(&df::DrapeEngine::SetModelViewCenter, _1, center, scale, true));
  else
    ShowRect(df::GetRectForDrawScale(scale, center));

  search::AddressInfo info;
  info.MakeFrom(res);

  SearchMarkPoint * mark = static_cast<SearchMarkPoint *>(guard.m_controller.CreateUserMark(center));
  mark->SetInfo(info);

  ActivateUserMark(mark, false);
}

size_t Framework::ShowAllSearchResults(search::Results const & results)
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
    using namespace search;

    Result const & r = results.GetResult(i);
    if (r.HasPoint())
    {
      AddressInfo info;
      info.MakeFrom(r);

      m2::PointD const pt = r.GetFeatureCenter();
      SearchMarkPoint * mark = static_cast<SearchMarkPoint *>(guard.m_controller.CreateUserMark(pt));
      mark->SetInfo(info);
    }
  }
}

void Framework::CancelInteractiveSearch()
{
  m_lastSearch.Clear();
  UserMarkControllerGuard(m_bmManager, UserMarkType::SEARCH_MARK).m_controller.Clear();
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
  using TReadIDsFn = df::MapDataProvider::TReadIDsFn;
  using TReadFeaturesFn = df::MapDataProvider::TReadFeaturesFn;
  using TUpdateCountryIndexFn = df::MapDataProvider::TUpdateCountryIndexFn;
  using TIsCountryLoadedFn = df::MapDataProvider::TIsCountryLoadedFn;
  using TDownloadFn = df::MapDataProvider::TDownloadFn;

  TReadIDsFn idReadFn = [this](df::MapDataProvider::TReadCallback<FeatureID> const & fn, m2::RectD const & r, int scale) -> void
  {
    m_model.ForEachFeatureID(r, fn, scale);
  };

  TReadFeaturesFn featureReadFn = [this](df::MapDataProvider::TReadCallback<FeatureType> const & fn, vector<FeatureID> const & ids) -> void
  {
    m_model.ReadFeatures(fn, ids);
  };

  TUpdateCountryIndexFn updateCountryIndex = [this](storage::TIndex const & currentIndex, m2::PointF const & pt)
  {
    GetPlatform().RunOnGuiThread(bind(&Framework::OnUpdateCountryIndex, this, currentIndex, pt));
  };

  TIsCountryLoadedFn isCountryLoadedFn = bind(&Framework::IsCountryLoaded, this, _1);
  auto isCountryLoadedByNameFn = bind(&Framework::IsCountryLoadedByName, this, _1);

  TDownloadFn downloadMapFn = [this](storage::TIndex const & countryIndex)
  {
    GetPlatform().RunOnGuiThread(bind(&Framework::OnDownloadMapCallback, this, countryIndex));
  };

  TDownloadFn downloadMapWithoutRoutingFn = [this](storage::TIndex const & countryIndex)
  {
    GetPlatform().RunOnGuiThread(bind(&Framework::OnDownloadMapRoutingCallback, this, countryIndex));
  };

  TDownloadFn downloadRetryFn = [this](storage::TIndex const & countryIndex)
  {
    GetPlatform().RunOnGuiThread(bind(&Framework::OnDownloadRetryCallback, this, countryIndex));
  };

  bool allow3d;
  bool allow3dBuildings;
  Load3dMode(allow3d, allow3dBuildings);

  df::DrapeEngine::Params p(contextFactory,
                            make_ref(&m_stringsBundle),
                            df::Viewport(0, 0, params.m_surfaceWidth, params.m_surfaceHeight),
                            df::MapDataProvider(idReadFn, featureReadFn, updateCountryIndex,
                                                isCountryLoadedFn, isCountryLoadedByNameFn,
                                                downloadMapFn, downloadMapWithoutRoutingFn,
                                                downloadRetryFn),
                            params.m_visualScale,
                            move(params.m_widgetsInitInfo),
                            make_pair(params.m_initialMyPositionState, params.m_hasMyPositionState),
                            allow3dBuildings);

  m_drapeEngine = make_unique_dp<df::DrapeEngine>(move(p));
  AddViewportListener([this](ScreenBase const & screen)
  {
    UpdateUserViewportChanged();
    m_currentModelView = screen;
  });
  m_drapeEngine->SetTapEventInfoListener(bind(&Framework::OnTapEvent, this, _1, _2, _3, _4));
  m_drapeEngine->SetUserPositionListener(bind(&Framework::OnUserPositionChanged, this, _1));
  OnSize(params.m_surfaceWidth, params.m_surfaceHeight);

  m_drapeEngine->SetMyPositionModeListener(m_myPositionListener);
  m_drapeEngine->InvalidateMyPosition();

  InvalidateUserMarks();

  // In case of the engine reinitialization simulate the last tap to show selection mark.
  if (m_lastTapEvent != nullptr)
  {
    UserMark const * mark = OnTapEventImpl(m_lastTapEvent->m_pxPoint, m_lastTapEvent->m_isLong,
                                           m_lastTapEvent->m_isMyPosition, m_lastTapEvent->m_feature);
    if (mark != nullptr)
      ActivateUserMark(mark, true);
  }

#ifdef OMIM_OS_ANDROID
  // In case of the engine reinitialization recover compass and location data
  // for correct my position state.
  if (m_lastCompassInfo != nullptr)
    OnCompassUpdate(*m_lastCompassInfo.release());
  if (m_lastGPSInfo != nullptr)
    OnLocationUpdate(*m_lastGPSInfo.release());
#endif

  Allow3dMode(allow3d, allow3dBuildings);

  // In case of the engine reinitialization recover route.
  if (m_routingSession.IsActive())
  {
    InsertRoute(m_routingSession.GetRoute());
    if (allow3d && m_routingSession.IsFollowing())
      m_drapeEngine->EnablePerspective(kRotationAngle, kAngleFOV);
  }

  if (m_connectToGpsTrack)
    GpsTracker::Instance().Connect(bind(&Framework::OnUpdateGpsTrackPointsCallback, this, _1, _2));
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
  Settings::Set(kMapStyleKey, static_cast<int>(mapStyle));
  GetStyleReader().SetCurrentStyle(mapStyle);

  alohalytics::TStringMap details {{"mapStyle", strings::to_string(static_cast<int>(mapStyle))}};
  alohalytics::Stats::Instance().LogEvent("MapStyle_Changed", details);
}

void Framework::SetMapStyle(MapStyle mapStyle)
{
  MarkMapStyle(mapStyle);
  CallDrapeFunction(bind(&df::DrapeEngine::UpdateMapStyle, _1));
  InvalidateUserMarks();
}

MapStyle Framework::GetMapStyle() const
{
  return GetStyleReader().GetCurrentStyle();
}

void Framework::SetupMeasurementSystem()
{
  GetPlatform().SetupMeasurementSystem();

  Settings::Units units = Settings::Metric;
  Settings::Get("Units", units);

  m_routingSession.SetTurnNotificationsUnits(units);
}

void Framework::SetWidgetLayout(gui::TWidgetsLayoutInfo && layout)
{
  ASSERT(m_drapeEngine != nullptr, ());
  m_drapeEngine->SetWidgetLayout(move(layout));
}

gui::TWidgetsSizeInfo const & Framework::GetWidgetSizes()
{
  ASSERT(m_drapeEngine != nullptr, ());
  return m_drapeEngine->GetWidgetSizes();
}

string Framework::GetCountryCode(m2::PointD const & pt) const
{
  storage::CountryInfo info;
  m_infoGetter->GetRegionInfo(pt, info);
  return info.m_flag;
}

bool Framework::ShowMapForURL(string const & url)
{
  m2::PointD point;
  m2::RectD rect;
  string name;
  UserMark const * apiMark = 0;

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
    // always hide current balloon here
    DeactivateUserMark();

    // set viewport and stop follow mode if any
    StopLocationFollow();
    ShowRect(rect);

    if (result != NO_NEED_CLICK)
    {
      if (apiMark)
      {
        LOG(LINFO, ("Show API mark:", static_cast<ApiMarkPoint const *>(apiMark)->GetName()));

        ActivateUserMark(apiMark, false);
      }
      else
      {
        PoiMarkPoint * mark = GetAddressMark(point);
        if (!name.empty())
          mark->SetName(name);
        ActivateUserMark(mark, false);
      }
    }

    return true;
  }

  return false;
}

bool Framework::GetVisiblePOI(m2::PointD const & glbPoint, search::AddressInfo & info, feature::Metadata & metadata) const
{
  ASSERT(m_drapeEngine != nullptr, ());
  FeatureID id = m_drapeEngine->GetVisiblePOI(glbPoint);
  if (!id.IsValid())
    return false;

  GetVisiblePOI(id, info, metadata);
  return true;
}

m2::PointD Framework::GetVisiblePOI(FeatureID const & id, search::AddressInfo & info, feature::Metadata & metadata) const
{
  ASSERT(id.IsValid(), ());
  Index::FeaturesLoaderGuard guard(m_model.GetIndex(), id.m_mwmId);

  FeatureType ft;
  guard.GetFeatureByIndex(id.m_index, ft);

  ft.ParseMetadata();
  metadata = ft.GetMetadata();

  ASSERT_NOT_EQUAL(ft.GetFeatureType(), feature::GEOM_LINE, ());
  m2::PointD const center = feature::GetCenter(ft);

  GetAddressInfo(ft, center, info);

  return m_currentModelView.isPerspective() ? GtoP3d(center) : GtoP(center);
}

namespace
{

/// POI - is a point or area feature.
class DoFindClosestPOI
{
  m2::PointD const & m_pt;
  double m_distMeters;
  FeatureID m_id;

public:
  DoFindClosestPOI(m2::PointD const & pt, double tresholdMeters)
    : m_pt(pt), m_distMeters(tresholdMeters)
  {
  }

  void operator() (FeatureType & ft)
  {
    if (ft.GetFeatureType() == feature::GEOM_LINE)
      return;

    double const dist = MercatorBounds::DistanceOnEarth(m_pt, feature::GetCenter(ft));
    if (dist < m_distMeters)
    {
      m_distMeters = dist;
      m_id = ft.GetID();
    }
  }

  void LoadMetadata(model::FeaturesFetcher const & model, feature::Metadata & metadata) const
  {
    if (!m_id.IsValid())
      return;

    Index::FeaturesLoaderGuard guard(model.GetIndex(), m_id.m_mwmId);

    FeatureType ft;
    guard.GetFeatureByIndex(m_id.m_index, ft);

    ft.ParseMetadata();
    metadata = ft.GetMetadata();
  }
};

}

void Framework::FindClosestPOIMetadata(m2::PointD const & pt, feature::Metadata & metadata) const
{
  m2::RectD rect(pt, pt);
  double const inf = MercatorBounds::GetCellID2PointAbsEpsilon();
  rect.Inflate(inf, inf);

  DoFindClosestPOI doFind(pt, 1.1 /* search radius in meters */);
  m_model.ForEachFeature(rect, doFind, scales::GetUpperScale() /* scale level for POI */);

  doFind.LoadMetadata(m_model, metadata);
}

BookmarkAndCategory Framework::FindBookmark(UserMark const * mark) const
{
  BookmarkAndCategory empty = MakeEmptyBookmarkAndCategory();
  BookmarkAndCategory result = empty;
  for (size_t i = 0; i < GetBmCategoriesCount(); ++i)
  {
    if (mark->GetContainer() == GetBmCategory(i))
    {
      result.first = i;
      break;
    }
  }

  ASSERT(result.first != empty.first, ());
  BookmarkCategory const * cat = GetBmCategory(result.first);
  for (size_t i = 0; i < cat->GetUserMarkCount(); ++i)
  {
    if (mark == cat->GetUserMark(i))
    {
      result.second = i;
      break;
    }
  }

  ASSERT(result != empty, ());
  return result;
}

PoiMarkPoint * Framework::GetAddressMark(m2::PointD const & globalPoint) const
{
  search::AddressInfo info;
  GetAddressInfoForGlobalPoint(globalPoint, info);
  PoiMarkPoint * mark = UserMarkContainer::UserMarkForPoi();
  mark->SetPtOrg(globalPoint);
  mark->SetInfo(info);
  return mark;
}

void Framework::ActivateUserMark(UserMark const * mark, bool needAnim)
{
  if (!m_activateUserMarkFn)
      return;

  if (mark)
  {
    m_activateUserMarkFn(mark->Copy());
    m2::PointD pt = mark->GetPivot();
    df::SelectionShape::ESelectedObject object = df::SelectionShape::OBJECT_USER_MARK;
    UserMark::Type type = mark->GetMarkType();
    if (type == UserMark::Type::MY_POSITION)
      object = df::SelectionShape::OBJECT_MY_POSITION;
    else if (type == UserMark::Type::POI)
      object = df::SelectionShape::OBJECT_POI;

    CallDrapeFunction(bind(&df::DrapeEngine::SelectObject, _1, object, pt, needAnim));
  }
  else
  {
    m_activateUserMarkFn(nullptr);
    CallDrapeFunction(bind(&df::DrapeEngine::DeselectObject, _1));
  }
}

void Framework::DeactivateUserMark()
{
  CallDrapeFunction(bind(&df::DrapeEngine::DeselectObject, _1));
}

bool Framework::HasActiveUserMark()
{
  if (m_drapeEngine == nullptr)
    return false;

  return m_drapeEngine->GetSelectedObject() != df::SelectionShape::OBJECT_EMPTY;
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

void Framework::OnTapEvent(m2::PointD pxPoint, bool isLong, bool isMyPosition, FeatureID const & feature)
{
  // Back up last tap event to recover selection in case of Drape reinitialization.
  if (!m_lastTapEvent)
    m_lastTapEvent = make_unique<TapEventData>();
  m_lastTapEvent->m_pxPoint = pxPoint;
  m_lastTapEvent->m_isLong = isLong;
  m_lastTapEvent->m_isMyPosition = isMyPosition;
  m_lastTapEvent->m_feature = feature;

  UserMark const * mark = OnTapEventImpl(pxPoint, isLong, isMyPosition, feature);

  {
    alohalytics::TStringMap details {{"isLongPress", isLong ? "1" : "0"}};
    if (mark)
      mark->FillLogEvent(details);
    alohalytics::Stats::Instance().LogEvent("$GetUserMark", details);
  }

  ActivateUserMark(mark, true);
}

void Framework::ResetLastTapEvent()
{
  m_lastTapEvent.reset();
}

void Framework::InvalidateRendering()
{
  if (m_drapeEngine != nullptr)
    m_drapeEngine->Invalidate();
}

UserMark const * Framework::OnTapEventImpl(m2::PointD pxPoint, bool isLong, bool isMyPosition, FeatureID const & feature)
{
  m2::PointD const pxPoint2d = m_currentModelView.P3dtoP(pxPoint);

  if (isMyPosition)
  {
    search::AddressInfo info;
    info.m_name = m_stringsBundle.GetString("my_position");
    MyPositionMarkPoint * myPostition = UserMarkContainer::UserMarkForMyPostion();
    myPostition->SetInfo(info);

    return myPostition;
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

  if (mark != nullptr)
    return mark;

  bool needMark = false;
  m2::PointD pxPivot;
  search::AddressInfo info;
  feature::Metadata metadata;

  if (feature.IsValid())
  {
    pxPivot = GetVisiblePOI(feature, info, metadata);
    needMark = true;
  }
  else if (isLong)
  {
    GetAddressInfoForPixelPoint(pxPoint2d, info);
    pxPivot = pxPoint;
    needMark = true;
  }

  if (needMark)
  {
    PoiMarkPoint * poiMark = UserMarkContainer::UserMarkForPoi();
    poiMark->SetPtOrg(m_currentModelView.PtoG(m_currentModelView.P3dtoP(pxPivot)));
    poiMark->SetInfo(info);
    poiMark->SetMetadata(move(metadata));
    return poiMark;
  }

  return nullptr;
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

string Framework::GenerateApiBackUrl(ApiMarkPoint const & point)
{
  string res = m_ParsedMapApi.GetGlobalBackUrl();
  if (!res.empty())
  {
    double lat, lon;
    point.GetLatLon(lat, lon);
    res += "pin?ll=" + strings::to_string(lat) + "," + strings::to_string(lon);
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
  if (Settings::Get("DataVersion", storedVersion))
  {
    return storedVersion < m_storage.GetCurrentDataVersion();
  }
  // no key in the settings, assume new version
  return true;
}

void Framework::UpdateSavedDataVersion()
{
  Settings::Set("DataVersion", m_storage.GetCurrentDataVersion());
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
    CallRouteBuilded(IRouter::NoCurrentPosition, vector<storage::TIndex>(), vector<storage::TIndex>());
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
    vector<storage::TIndex> absentCountries;
    vector<storage::TIndex> absentRoutingIndexes;
    if (code == IRouter::NoError)
    {
      double const kRouteScaleMultiplier = 1.5;

      InsertRoute(route);
      m2::RectD routeRect = route.GetPoly().GetLimitRect();
      routeRect.Scale(kRouteScaleMultiplier);
      ShowRect(routeRect, -1);
    }
    else
    {
      for (string const & name : route.GetAbsentCountries())
      {
        storage::TIndex fileIndex = m_storage.FindIndexByFile(name);
        if (m_storage.GetLatestLocalFile(fileIndex))
          absentRoutingIndexes.push_back(fileIndex);
        else
          absentCountries.push_back(fileIndex);
      }

      if (code != IRouter::NeedMoreMaps)
        RemoveRoute(true /* deactivateFollowing */);
    }
    CallRouteBuilded(code, absentCountries, absentRoutingIndexes);
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
    return m_infoGetter->GetRegionFile(p);
  };

  if (type == RouterType::Pedestrian)
  {
    router = CreatePedestrianAStarBidirectionalRouter(m_model.GetIndex(), countryFileGetter);
    m_routingSession.SetRoutingSettings(routing::GetPedestrianRoutingSettings());
  }
  else
  {
    auto localFileGetter = [this](string const & countryFile) -> shared_ptr<LocalCountryFile>
    {
      return m_storage.GetLatestLocalFile(CountryFile(countryFile));
    };

    router.reset(new OsrmRouter(&m_model.GetIndex(), countryFileGetter));
    fetcher.reset(new OnlineAbsentCountriesFetcher(countryFileGetter, localFileGetter));
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
  if (m_routingSession.IsActive())
  {
    auto const lastGoodPoint = MercatorBounds::ToLatLon(
        m_routingSession.GetRoute().GetFollowedPolyline().GetCurrentIter().m_pt);
    alohalytics::Stats::Instance().LogEvent(
        "RouteTracking_RouteClosing",
        {{"percent", strings::to_string(m_routingSession.GetCompletionPercent())}},
        alohalytics::Location::FromLatLon(lastGoodPoint.lat, lastGoodPoint.lon));
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

void Framework::CallRouteBuilded(IRouter::ResultCode code, vector<storage::TIndex> const & absentCountries, vector<storage::TIndex> const & absentRoutingFiles)
{
  if (code == IRouter::Cancelled)
    return;
  m_routingCallback(code, absentCountries, absentRoutingFiles);
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
      return m_infoGetter->GetRegionFile(pt);
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
  Settings::Get(kRouterTypeKey, routerType);
  return (routerType == routing::ToString(RouterType::Pedestrian) ? RouterType::Pedestrian : RouterType::Vehicle);
}

void Framework::SetLastUsedRouter(RouterType type)
{
  Settings::Set(kRouterTypeKey, routing::ToString(type));
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
  Settings::Set(kAllow3dKey, allow3d);
  Settings::Set(kAllow3dBuildingsKey, allow3dBuildings);
}

void Framework::Load3dMode(bool & allow3d, bool & allow3dBuildings)
{
  if (!Settings::Get(kAllow3dKey, allow3d))
    allow3d = true;

  if (!Settings::Get(kAllow3dBuildingsKey, allow3dBuildings))
    allow3dBuildings = true;
}
