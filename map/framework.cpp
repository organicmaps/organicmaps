#include "framework.hpp"
#include "benchmark_provider.hpp"
#include "benchmark_engine.hpp"
#include "geourl_process.hpp"
#include "navigator_utils.hpp"
#include "ge0_parser.hpp"

#include "render/cpu_drawer.hpp"
#include "render/gpu_drawer.hpp"

#ifndef USE_DRAPE
  #include "render/feature_processor.hpp"
  #include "render/drawer.hpp"
#else
  #include "../drape_frontend/visual_params.hpp"
#endif // USE_DRAPE

#include "defines.hpp"

#include "routing/online_absent_fetcher.hpp"
#include "routing/osrm_router.hpp"
#include "routing/road_graph_router.hpp"
#include "routing/route.hpp"
#include "routing/routing_algorithm.hpp"

#include "search/search_engine.hpp"
#include "search/search_query_factory.hpp"
#include "search/result.hpp"

#include "indexer/categories_holder.hpp"
#include "indexer/classificator_loader.hpp"
#include "indexer/drawing_rules.hpp"
#include "indexer/feature.hpp"
#include "indexer/map_style_reader.hpp"
#include "indexer/mwm_version.hpp"
#include "indexer/scales.hpp"

/// @todo Probably it's better to join this functionality.
//@{
#include "indexer/feature_algo.hpp"
#include "indexer/feature_utils.hpp"
//@}

#include "anim/controller.hpp"

#include "gui/controller.hpp"

#include "platform/local_country_file_utils.hpp"
#include "platform/measurement_utils.hpp"
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

#include "graphics/depth_constants.hpp"

#include "base/math.hpp"
#include "base/timer.hpp"
#include "base/scope_guard.hpp"

#include "std/algorithm.hpp"
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
}

pair<MwmSet::MwmHandle, MwmSet::RegResult> Framework::RegisterMap(
    LocalCountryFile const & localFile)
{
  string const countryFileName = localFile.GetCountryName();
  LOG(LINFO, ("Loading map:", countryFileName));
  return m_model.RegisterMap(localFile);
}

void Framework::OnLocationError(TLocationError /*error*/) {}

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
  location::RouteMatchingInfo routeMatchingInfo;
  CheckLocationForRouting(rInfo);
  bool hasDistanceFromBegin = false;
  double distanceFromBegin = 0.0;
  MatchLocationToRoute(rInfo, routeMatchingInfo, hasDistanceFromBegin, distanceFromBegin);

  shared_ptr<State> const & state = GetLocationState();
  state->OnLocationUpdate(rInfo, m_routingSession.IsNavigable(), routeMatchingInfo);

  if (state->IsModeChangeViewport())
    UpdateUserViewportChanged();

  m_bmManager.UpdateRouteDistanceFromBegin(hasDistanceFromBegin ? distanceFromBegin : 0.0);
}

void Framework::OnCompassUpdate(CompassInfo const & info)
{
#ifdef FIXED_LOCATION
  CompassInfo rInfo(info);
  m_fixedPos.GetNorth(rInfo.m_bearing);
#else
  CompassInfo const & rInfo = info;
#endif

  GetLocationState()->OnCompassUpdate(rInfo);
}

void Framework::StopLocationFollow()
{
  GetLocationState()->StopLocationFollow();
}

InformationDisplay & Framework::GetInformationDisplay()
{
  return m_informationDisplay;
}

CountryStatusDisplay * Framework::GetCountryStatusDisplay() const
{
  return m_informationDisplay.countryStatusDisplay().get();
}

void Framework::SetWidgetPivot(InformationDisplay::WidgetType widget, m2::PointD const & pivot)
{
  m_informationDisplay.SetWidgetPivot(widget, pivot);
}

m2::PointD Framework::GetWidgetSize(InformationDisplay::WidgetType widget) const
{
  return m_informationDisplay.GetWidgetSize(widget);
}

Framework::Framework()
  : m_navigator(m_scales),
    m_animator(this),
    m_queryMaxScaleMode(false),
    m_width(0),
    m_height(0),
    m_countryTree(*this),
    m_guiController(new gui::Controller),
    m_animController(new anim::Controller),
    m_informationDisplay(this),
    m_benchmarkEngine(0),
    m_bmManager(*this),
    m_balloonManager(*this),
    m_fixedSearchResults(0),
    m_locationChangedSlotID(-1)
{
  // Checking whether we should enable benchmark.
  bool isBenchmarkingEnabled = false;
  (void)Settings::Get("IsBenchmarking", isBenchmarkingEnabled);
  if (isBenchmarkingEnabled)
    m_benchmarkEngine = new BenchmarkEngine(this);

  m_ParsedMapApi.SetController(&m_bmManager.UserMarksGetController(UserMarkContainer::API_MARK));

  // Init strings bundle.
  // @TODO. There are hardcoded strings below which are defined in strings.txt as well.
  // It's better to use strings form strings.txt intead of hardcoding them here.
  m_stringsBundle.SetDefaultString("country_status_added_to_queue", "^\nis added to the downloading queue");
  m_stringsBundle.SetDefaultString("country_status_downloading", "Downloading\n^\n^%");
  m_stringsBundle.SetDefaultString("country_status_download", "Download map\n^ ^");
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

  m_guiController->SetStringsBundle(&m_stringsBundle);

  // Init information display.
  m_informationDisplay.setController(m_guiController.get());

#ifdef DRAW_TOUCH_POINTS
  m_informationDisplay.enableDebugPoints(true);
#endif

  m_model.InitClassificator();
  m_model.SetOnMapDeregisteredCallback(bind(&Framework::OnMapDeregistered, this, _1));
  LOG(LDEBUG, ("Classificator initialized"));

  // To avoid possible races - init search engine once in constructor.
  (void)GetSearchEngine();
  LOG(LDEBUG, ("Search engine initialized"));

#ifndef OMIM_OS_ANDROID
  RegisterAllMaps();
#endif
  LOG(LDEBUG, ("Maps initialized"));

  // Init storage with needed callback.
  m_storage.Init(bind(&Framework::UpdateLatestCountryFile, this, _1));
  LOG(LDEBUG, ("Storage initialized"));

  auto const routingStatisticsFn = [](map<string, string> const & statistics)
  {
    alohalytics::LogEvent("Routing_CalculatingRoute", statistics);
  };
#ifdef DEBUG
  auto const routingVisualizerFn = [this](m2::PointD const & pt)
  {
    GetPlatform().RunOnGuiThread([this,pt]()
    {
      m_bmManager.UserMarksGetController(UserMarkContainer::DEBUG_MARK).CreateUserMark(pt);
      Invalidate();
    });
  };
#else
  routing::RouterDelegate::TPointCheckCallback const routingVisualizerFn = nullptr;
#endif
  m_routingSession.Init(routingStatisticsFn, routingVisualizerFn);

  SetRouterImpl(RouterType::Vehicle);

  LOG(LDEBUG, ("Routing engine initialized"));

  LOG(LINFO, ("System languages:", languages::GetPreferred()));
}

Framework::~Framework()
{
  delete m_benchmarkEngine;
  m_model.SetOnMapDeregisteredCallback(nullptr);
}

void Framework::DrawSingleFrame(m2::PointD const & center, int zoomModifier,
                                uint32_t pxWidth, uint32_t pxHeight, FrameImage & image,
                                SingleFrameSymbols const & symbols)
{
  ASSERT(IsSingleFrameRendererInited(), ());
  Navigator frameNavigator = m_navigator;
  frameNavigator.OnSize(0, 0, pxWidth, pxHeight);
  frameNavigator.SetAngle(0);

  m2::RectD rect = m_scales.GetRectForDrawScale(scales::GetUpperComfortScale() - 1, center);
  if (symbols.m_showSearchResult && !rect.IsPointInside(symbols.m_searchResult))
  {
    double const kScaleFactor = 1.3;
    m2::PointD oldCenter = rect.Center();
    rect.Add(symbols.m_searchResult);
    double const centersDiff = 2 * (rect.Center() - oldCenter).Length();

    m2::RectD resultRect;
    resultRect.SetSizes(rect.SizeX() + centersDiff, rect.SizeY() + centersDiff);
    resultRect.SetCenter(center);
    resultRect.Scale(kScaleFactor);
    rect = resultRect;
    ASSERT(rect.IsPointInside(symbols.m_searchResult), ());
  }


  int baseZoom = m_scales.GetDrawTileScale(rect);
  int resultZoom = baseZoom + zoomModifier;
  int const minZoom = symbols.m_bottomZoom == -1 ? resultZoom : symbols.m_bottomZoom;
  resultZoom = my::clamp(resultZoom, minZoom, scales::GetUpperScale());
  rect = m_scales.GetRectForDrawScale(resultZoom, rect.Center());

  CheckMinGlobalRect(rect);
  CheckMinMaxVisibleScale(rect);
  frameNavigator.SetFromRect(m2::AnyRectD(rect));

  m_cpuDrawer->BeginFrame(pxWidth, pxHeight);

  ScreenBase const & s = frameNavigator.Screen();
  shared_ptr<PaintEvent> event = make_shared<PaintEvent>(m_cpuDrawer.get());
  DrawModel(event, s, m2::RectD(0, 0, pxWidth, pxHeight), m_scales.GetTileScaleBase(s), false);

  m_cpuDrawer->Flush();
  m_cpuDrawer->DrawMyPosition(frameNavigator.GtoP(center));

  if (symbols.m_showSearchResult)
  {
    if (!frameNavigator.Screen().PixelRect().IsPointInside(frameNavigator.GtoP(symbols.m_searchResult)))
      m_cpuDrawer->DrawSearchArrow(ang::AngleTo(rect.Center(), symbols.m_searchResult));
    else
      m_cpuDrawer->DrawSearchResult(frameNavigator.GtoP(symbols.m_searchResult));
  }

  m_cpuDrawer->EndFrame(image);
}

void Framework::InitSingleFrameRenderer(graphics::EDensity density)
{
  ASSERT(!IsSingleFrameRendererInited(), ());
  if (m_cpuDrawer == nullptr)
  {
    CPUDrawer::Params params(GetGlyphCacheParams(density));
    params.m_visualScale = graphics::visualScale(density);
    params.m_density = density;

    m_cpuDrawer.reset(new CPUDrawer(params));
  }
}

void Framework::ReleaseSingleFrameRenderer()
{
  if (IsSingleFrameRendererInited())
    m_cpuDrawer.reset();
}

bool Framework::IsSingleFrameRendererInited() const
{
  return m_cpuDrawer != nullptr;
}

double Framework::GetVisualScale() const
{
  return m_scales.GetVisualScale();
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
      {
        InvalidateRect(GetCountryBounds(countryFile.GetNameWithoutExt()), true /* doForceUpdate */);
      }
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
  m2::RectD const r = GetSearchEngine()->GetCountryBounds(file);
  ASSERT ( r.IsValid(), () );
  return r;
}

m2::RectD Framework::GetCountryBounds(TIndex const & index) const
{
  CountryFile const & file = m_storage.GetCountryFile(index);
  return GetCountryBounds(file.GetNameWithoutExt());
}

void Framework::ShowCountry(TIndex const & index)
{
  StopLocationFollow();

  ShowRectEx(GetCountryBounds(index));
}

void Framework::UpdateLatestCountryFile(LocalCountryFile const & localFile)
{
  // TODO (@ldragunov, @gorshenin): rewrite routing session to use MwmHandles. Thus,
  // it won' be needed to reset it after maps update.
  m_routingSession.Reset();

  if (!HasOptions(localFile.GetFiles(), MapOptions::Map))
    return;

  // Add downloaded map.
  auto result = m_model.RegisterMap(localFile);
  MwmSet::MwmHandle const & handle = result.first;
  if (handle.IsAlive())
    InvalidateRect(handle.GetInfo()->m_limitRect, true /* doForceUpdate */);
  GetSearchEngine()->ClearViewportsCache();
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

  platform::CleanupMapsDirectory();
  m_storage.RegisterAllLocalMaps();

  int minFormat = numeric_limits<int>::max();
  vector<CountryFile> maps;
  m_storage.GetLocalMaps(maps);

  for (CountryFile const & countryFile : maps)
  {
    shared_ptr<LocalCountryFile> localFile = m_storage.GetLatestLocalFile(countryFile);
    if (!localFile)
      continue;
    auto p = RegisterMap(*localFile);
    if (p.second != MwmSet::RegResult::Success)
      continue;
    MwmSet::MwmHandle const & handle = p.first;
    ASSERT(handle.IsAlive(), ());
    minFormat = min(minFormat, static_cast<int>(handle.GetInfo()->m_version.format));
  }

  m_countryTree.Init(maps);

  GetSearchEngine()->SupportOldFormat(minFormat < version::v3);
}

void Framework::DeregisterAllMaps()
{
  m_countryTree.Clear();
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
  Bookmark const * bmk = m_bmManager.GetBmCategory(bnc.first)->GetBookmark(bnc.second);

  double scale = bmk->GetScale();
  if (scale == -1.0)
    scale = scales::GetUpperComfortScale();

  ShowRectExVisibleScale(m_scales.GetRectForDrawScale(scale, bmk->GetOrg()));
  Bookmark * mark = GetBmCategory(bnc.first)->GetBookmark(bnc.second);
  ActivateUserMark(mark);
  m_balloonManager.OnShowMark(mark);
}

void Framework::ShowTrack(Track const & track)
{
  StopLocationFollow();
  ShowRectEx(track.GetLimitRect());
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

}

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
#ifndef USE_DRAPE
  m_bmManager.PrepareToShutdown();
  SetRenderPolicy(0);
#else
  m_drapeEngine.Destroy();
#endif // USE_DRAPE
}

void Framework::SetMaxWorldRect()
{
  m_navigator.SetFromRect(m2::AnyRectD(m_model.GetWorldRect()));
}

bool Framework::NeedRedraw() const
{
  // Checking this here allows to avoid many dummy "IsInitialized" flags in client code.
#ifndef USE_DRAPE
  return (m_renderPolicy && m_renderPolicy->NeedRedraw());
#else
  return false;
#endif // USE_DRAPE
}

void Framework::SetNeedRedraw(bool flag)
{
#ifndef USE_DRAPE
  m_renderPolicy->GetWindowHandle()->setNeedRedraw(flag);
  //if (!flag)
  //  m_doForceUpdate = false;
#endif // USE_DRAPE
}

void Framework::Invalidate(bool doForceUpdate)
{
  InvalidateRect(MercatorBounds::FullRect(), doForceUpdate);
}

void Framework::InvalidateRect(m2::RectD const & rect, bool doForceUpdate)
{
#ifndef USE_DRAPE
  if (m_renderPolicy)
  {
    ASSERT ( rect.IsValid(), () );
    m_renderPolicy->SetForceUpdate(doForceUpdate);
    m_renderPolicy->SetInvalidRect(m2::AnyRectD(rect));
    m_renderPolicy->GetWindowHandle()->invalidate();
  }
#else
  if (!m_drapeEngine.IsNull())
    m_drapeEngine->UpdateCoverage(m_navigator.Screen());
#endif // USE_DRAPE
}

void Framework::SaveState()
{
  Settings::Set("ScreenClipRect", m_navigator.Screen().GlobalRect());
}

bool Framework::LoadState()
{
  m2::AnyRectD rect;
  if (!Settings::Get("ScreenClipRect", rect))
    return false;

  // additional check for valid rect
  m2::RectD r = rect.GetGlobalRect();
  if (!m_scales.GetWorldRect().IsRectInside(r))
    return false;

  CheckMinMaxVisibleScale(r);

  double const dx = r.SizeX();
  double const dy = r.SizeY();

  m2::AnyRectD safeRect(r.Center(), rect.Angle(), m2::RectD(-dx/2, -dy/2, dx/2, dy/2));
  m_navigator.SetFromRect(safeRect);

#ifdef USE_DRAPE
  if (!m_drapeEngine.IsNull())
    m_drapeEngine->UpdateCoverage(m_navigator.Screen());
#endif
  return true;
}
//@}


void Framework::OnSize(int w, int h)
{
  if (w < 2) w = 2;
  if (h < 2) h = 2;

  m2::RectD oldPixelRect = m_navigator.Screen().PixelRect();

#ifndef USE_DRAPE
  m_navigator.OnSize(0, 0, w, h);
  if (m_renderPolicy)
  {
    // if gui controller not initialized, than we work in mode "Without gui"
    // and no need to set gui layout. We will not render it.
    if (m_guiController->GetCacheScreen())
      m_informationDisplay.SetWidgetPivotsByDefault(w, h);
    m_renderPolicy->OnSize(w, h);
  }
#else
  if (!m_drapeEngine.IsNull())
  {
    double vs = df::VisualParams::Instance().GetVisualScale();
    m_navigator.OnSize(0, 0, vs * w, vs * h);
    //m_navigator.OnSize(0, 0, w, h);
    m_drapeEngine->Resize(w, h);
    m_drapeEngine->UpdateCoverage(m_navigator.Screen());
  }
#endif // USE_DRAPE

  m_width = w;
  m_height = h;
  GetLocationState()->OnSize(oldPixelRect);
}

bool Framework::SetUpdatesEnabled(bool doEnable)
{
#ifndef USE_DRAPE
  if (m_renderPolicy)
    return m_renderPolicy->GetWindowHandle()->setUpdatesEnabled(doEnable);
  else
#endif // USE_DRAPE
    return false;
}

int Framework::GetDrawScale() const
{
  return m_navigator.GetDrawScale();
}

#ifndef USE_DRAPE
RenderPolicy::TRenderFn Framework::DrawModelFn()
{
  bool const isTiling = m_renderPolicy->IsTiling();
  return bind(&Framework::DrawModel, this, _1, _2, _3, _4, isTiling);
}

void Framework::DrawModel(shared_ptr<PaintEvent> const & e,
                          ScreenBase const & screen,
                          m2::RectD const & renderRect,
                          int baseScale, bool isTilingQuery)
{
  m2::RectD selectRect;
  m2::RectD clipRect;

  double const inflationSize = m_scales.GetClipRectInflation();
  screen.PtoG(m2::Inflate(m2::RectD(renderRect), inflationSize, inflationSize), clipRect);
  screen.PtoG(m2::RectD(renderRect), selectRect);

  int drawScale = m_scales.GetDrawTileScale(baseScale);
  fwork::FeatureProcessor doDraw(clipRect, screen, e, drawScale);
  if (m_queryMaxScaleMode)
    drawScale = scales::GetUpperScale();

  try
  {
    int const upperScale = scales::GetUpperScale();
    if (isTilingQuery && drawScale <= upperScale)
      m_model.ForEachFeature_TileDrawing(selectRect, doDraw, drawScale);
    else
      m_model.ForEachFeature(selectRect, doDraw, min(upperScale, drawScale));
  }
  catch (redraw_operation_cancelled const &)
  {}

  e->setIsEmptyDrawing(doDraw.IsEmptyDrawing());

  if (m_navigator.Update(ElapsedSeconds()))
    Invalidate();
}
#endif // USE_DRAPE

bool Framework::IsCountryLoaded(m2::PointD const & pt) const
{
  // Correct, but slow version (check country polygon).
  string const fName = GetSearchEngine()->GetCountryFile(pt);
  if (fName.empty())
    return true;

  return m_model.IsLoaded(fName);
}

#ifndef USE_DRAPE
void Framework::BeginPaint(shared_ptr<PaintEvent> const & e)
{
  if (m_renderPolicy)
    m_renderPolicy->BeginFrame(e, m_navigator.Screen());
}

void Framework::EndPaint(shared_ptr<PaintEvent> const & e)
{
  if (m_renderPolicy)
    m_renderPolicy->EndFrame(e, m_navigator.Screen());
}

void Framework::DrawAdditionalInfo(shared_ptr<PaintEvent> const & e)
{
  // m_informationDisplay is set and drawn after the m_renderPolicy
  ASSERT ( m_renderPolicy, () );

  graphics::Screen * pScreen = GPUDrawer::GetScreen(e->drawer());

  pScreen->beginFrame();

  int const drawScale = GetDrawScale();
  bool const isEmptyModel = m_renderPolicy->IsEmptyModel();

  if (isEmptyModel)
    m_informationDisplay.setEmptyCountryIndex(GetCountryIndex(GetViewportCenter()));
  else
    m_informationDisplay.setEmptyCountryIndex(storage::TIndex());

  bool const isCompassEnabled = my::Abs(ang::GetShortestDistance(m_navigator.Screen().GetAngle(), 0.0)) > my::DegToRad(3.0);
  bool const isCompasActionEnabled = m_informationDisplay.isCompassArrowEnabled() && m_navigator.InAction();

  m_informationDisplay.enableCompassArrow(isCompassEnabled || isCompasActionEnabled);
  m_informationDisplay.setCompassArrowAngle(m_navigator.Screen().GetAngle());

  m_informationDisplay.enableRuler(!m_isFullScreenMode && (drawScale > 4 && !m_informationDisplay.isCopyrightActive()));

  m_informationDisplay.setDebugInfo(0, drawScale);
  pScreen->endFrame();

  m_bmManager.DrawItems(e->drawer());
  m_guiController->UpdateElements();
  m_guiController->DrawFrame(pScreen);
}

void Framework::DoPaint(shared_ptr<PaintEvent> const & e)
{
  if (m_renderPolicy)
  {
    m_renderPolicy->DrawFrame(e, m_navigator.Screen());

    // Don't render additional elements if guiController wasn't initialized.
    if (m_guiController->GetCacheScreen() != NULL)
      DrawAdditionalInfo(e);
  }
}
#endif // USE_DRAPE

m2::PointD const & Framework::GetViewportCenter() const
{
  return m_navigator.Screen().GetOrg();
}

void Framework::SetViewportCenter(m2::PointD const & pt)
{
  m_navigator.CenterViewport(pt);
  Invalidate();
}

shared_ptr<MoveScreenTask> Framework::SetViewportCenterAnimated(m2::PointD const & endPt)
{
  anim::Controller::Guard guard(GetAnimController());
  m2::PointD const & startPt = GetViewportCenter();
  return m_animator.MoveScreen(startPt, endPt, m_navigator.ComputeMoveSpeed(startPt, endPt));
}

void Framework::CheckMinGlobalRect(m2::RectD & rect) const
{
  m2::RectD const minRect = m_scales.GetRectForDrawScale(scales::GetUpperStyleScale(), rect.Center());
  if (minRect.IsRectInside(rect))
    rect = minRect;
}

void Framework::CheckMinMaxVisibleScale(m2::RectD & rect, int maxScale/* = -1*/) const
{
  CheckMinGlobalRect(rect);

  m2::PointD const c = rect.Center();
  int const worldS = scales::GetUpperWorldScale();

  int scale = m_scales.GetDrawTileScale(rect);
  if (scale > worldS && !IsCountryLoaded(c))
  {
    // country is not loaded - limit on world scale
    rect = m_scales.GetRectForDrawScale(worldS, c);
    scale = worldS;
  }

  if (maxScale != -1 && scale > maxScale)
  {
    // limit on passed maximal scale
    rect = m_scales.GetRectForDrawScale(maxScale, c);
  }
}

void Framework::ShowRect(double lat, double lon, double zoom)
{
  m2::PointD center(MercatorBounds::FromLatLon(lat, lon));
  ShowRect(center, zoom);
}

void Framework::ShowRect(m2::PointD const & pt, double zoom)
{
  ShowRectEx(m_scales.GetRectForDrawScale(zoom, pt));
}

void Framework::ShowRect(m2::RectD rect)
{
  CheckMinGlobalRect(rect);

  m_navigator.SetFromRect(m2::AnyRectD(rect));
  Invalidate();
}

void Framework::ShowRectEx(m2::RectD rect)
{
  CheckMinGlobalRect(rect);

  ShowRectFixed(rect);
}

void Framework::ShowRectExVisibleScale(m2::RectD rect, int maxScale/* = -1*/)
{
  CheckMinMaxVisibleScale(rect, maxScale);

  ShowRectFixed(rect);
}

void Framework::ShowRectFixed(m2::RectD const & rect)
{
  ShowRectFixedAR(navi::ToRotated(rect, m_navigator));
}

void Framework::ShowRectFixedAR(m2::AnyRectD const & rect)
{
  navi::SetRectFixedAR(rect, m_scales, m_navigator);
  Invalidate();
}

void Framework::UpdateUserViewportChanged()
{
  if (IsISActive())
  {
    (void)GetCurrentPosition(m_lastSearch.m_lat, m_lastSearch.m_lon);
    m_lastSearch.m_callback = bind(&Framework::OnSearchResultsCallback, this, _1);
    m_lastSearch.SetSearchMode(search::SearchParams::IN_VIEWPORT_ONLY);
    m_lastSearch.SetForceSearch(false);

    (void)GetSearchEngine()->Search(m_lastSearch, GetCurrentViewport());
  }
}

void Framework::OnSearchResultsCallback(search::Results const & results)
{
  if (!results.IsEndMarker() && results.GetCount() > 0)
  {
    // Got here from search thread. Need to switch into GUI thread to modify search mark container.
    // Do copy the results structure to pass into GUI thread.
    GetPlatform().RunOnGuiThread(bind(&Framework::OnSearchResultsCallbackUI, this, results));
  }
}

void Framework::OnSearchResultsCallbackUI(search::Results const & results)
{
  if (IsISActive())
  {
    FillSearchResultsMarks(results);

    Invalidate();
  }
}

void Framework::ClearAllCaches()
{
  m_model.ClearCaches();
  GetSearchEngine()->ClearAllCaches();
}

void Framework::MemoryWarning()
{
  LOG(LINFO, ("MemoryWarning"));
  ClearAllCaches();
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
}

void Framework::EnterForeground()
{
  m_startForegroundTime = my::Timer::LocalTime();
}

void Framework::ShowAll()
{
  SetMaxWorldRect();
  Invalidate();
}

/// @name Drag implementation.
//@{
m2::PointD Framework::GetPixelCenter() const
{
  return m_navigator.Screen().PixelRect().Center();
}

void Framework::StartDrag(DragEvent const & e)
{
  m_navigator.StartDrag(m_navigator.ShiftPoint(e.Pos()), ElapsedSeconds());
  m_informationDisplay.locationState()->DragStarted();

#ifndef USE_DRAPE
  if (m_renderPolicy)
    m_renderPolicy->StartDrag();
#else
  if (!m_drapeEngine.IsNull())
    m_drapeEngine->UpdateCoverage(m_navigator.Screen());
#endif // USE_DRAPE
}

void Framework::DoDrag(DragEvent const & e)
{
  m_navigator.DoDrag(m_navigator.ShiftPoint(e.Pos()), ElapsedSeconds());

#ifndef USE_DRAPE
  if (m_renderPolicy)
    m_renderPolicy->DoDrag();
#else
  if (!m_drapeEngine.IsNull())
    m_drapeEngine->UpdateCoverage(m_navigator.Screen());
#endif // USE_DRAPE
}

void Framework::StopDrag(DragEvent const & e)
{
  m_navigator.StopDrag(m_navigator.ShiftPoint(e.Pos()), ElapsedSeconds(), true);
  m_informationDisplay.locationState()->DragEnded();

#ifndef USE_DRAPE
  if (m_renderPolicy)
  {
    m_renderPolicy->StopDrag();
    UpdateUserViewportChanged();
  }
#else
  if (!m_drapeEngine.IsNull())
    m_drapeEngine->UpdateCoverage(m_navigator.Screen());
#endif // USE_DRAPE
}

void Framework::StartRotate(RotateEvent const & e)
{
  m_navigator.StartRotate(e.Angle(), ElapsedSeconds());
#ifndef USE_DRAPE
  m_renderPolicy->StartRotate(e.Angle(), ElapsedSeconds());
#else
  if (!m_drapeEngine.IsNull())
    m_drapeEngine->UpdateCoverage(m_navigator.Screen());
#endif // USE_DRAPE
  GetLocationState()->ScaleStarted();
}

void Framework::DoRotate(RotateEvent const & e)
{
  m_navigator.DoRotate(e.Angle(), ElapsedSeconds());
#ifndef USE_DRAPE
  m_renderPolicy->DoRotate(e.Angle(), ElapsedSeconds());
#else
  if (!m_drapeEngine.IsNull())
    m_drapeEngine->UpdateCoverage(m_navigator.Screen());
#endif
}

void Framework::StopRotate(RotateEvent const & e)
{
  m_navigator.StopRotate(e.Angle(), ElapsedSeconds());
  shared_ptr<State> const & state = GetLocationState();
  state->Rotated();
  state->ScaleEnded();
#ifndef USE_DRAPE
  m_renderPolicy->StopRotate(e.Angle(), ElapsedSeconds());
#else
  if (!m_drapeEngine.IsNull())
    m_drapeEngine->UpdateCoverage(m_navigator.Screen());
#endif
  UpdateUserViewportChanged();
}

void Framework::Move(double azDir, double factor)
{
  m_navigator.Move(azDir, factor);

  Invalidate();
}
//@}

/// @name Scaling.
//@{
void Framework::ScaleToPoint(ScaleToPointEvent const & e, bool anim)
{
  m2::PointD pt = m_navigator.ShiftPoint(e.Pt());
  GetLocationState()->CorrectScalePoint(pt);

  if (anim)
    m_animController->AddTask(m_navigator.ScaleToPointAnim(pt, e.ScaleFactor(), 0.25));
  else
    m_navigator.ScaleToPoint(pt, e.ScaleFactor(), 0);

  Invalidate();
  UpdateUserViewportChanged();
}

void Framework::ScaleDefault(bool enlarge)
{
  Scale(enlarge ? 1.5 : 2.0/3.0);
}

void Framework::Scale(double scale)
{
  m2::PointD center = GetPixelCenter();
  GetLocationState()->CorrectScalePoint(center);
  m_animController->AddTask(m_navigator.ScaleToPointAnim(center, scale, 0.25));

  Invalidate();
  UpdateUserViewportChanged();
}

void Framework::CalcScalePoints(ScaleEvent const & e, m2::PointD & pt1, m2::PointD & pt2) const
{
  pt1 = m_navigator.ShiftPoint(e.Pt1());
  pt2 = m_navigator.ShiftPoint(e.Pt2());

  m_informationDisplay.locationState()->CorrectScalePoint(pt1, pt2);
}

void Framework::StartScale(ScaleEvent const & e)
{
  m2::PointD pt1, pt2;
  CalcScalePoints(e, pt1, pt2);

  GetLocationState()->ScaleStarted();
  m_navigator.StartScale(pt1, pt2, ElapsedSeconds());
#ifndef USE_DRAPE
  if (m_renderPolicy)
    m_renderPolicy->StartScale();
#else
  if (!m_drapeEngine.IsNull())
    m_drapeEngine->UpdateCoverage(m_navigator.Screen());
#endif // USE_DRAPE
}

void Framework::DoScale(ScaleEvent const & e)
{
  m2::PointD pt1, pt2;
  CalcScalePoints(e, pt1, pt2);

  m_navigator.DoScale(pt1, pt2, ElapsedSeconds());
#ifndef USE_DRAPE
  if (m_renderPolicy)
    m_renderPolicy->DoScale();
#else
  if (!m_drapeEngine.IsNull())
    m_drapeEngine->UpdateCoverage(m_navigator.Screen());
#endif // USE_DRAPE

  if (m_navigator.IsRotatingDuringScale())
    GetLocationState()->Rotated();
}

void Framework::StopScale(ScaleEvent const & e)
{
  m2::PointD pt1, pt2;
  CalcScalePoints(e, pt1, pt2);

  m_navigator.StopScale(pt1, pt2, ElapsedSeconds());

#ifndef USE_DRAPE
  if (m_renderPolicy)
  {
    m_renderPolicy->StopScale();
    UpdateUserViewportChanged();
  }
#else
  if (!m_drapeEngine.IsNull())
    m_drapeEngine->UpdateCoverage(m_navigator.Screen());
#endif // USE_DRAPE

  GetLocationState()->ScaleEnded();
}
//@}

search::Engine * Framework::GetSearchEngine() const
{
  if (!m_pSearchEngine)
  {
    Platform & pl = GetPlatform();

    try
    {
      m_pSearchEngine.reset(new search::Engine(
          &m_model.GetIndex(), pl.GetReader(SEARCH_CATEGORIES_FILE_NAME),
          pl.GetReader(PACKED_POLYGONS_FILE), pl.GetReader(COUNTRIES_FILE),
          languages::GetCurrentOrig(), make_unique<search::SearchQueryFactory>()));
    }
    catch (RootException const & e)
    {
      LOG(LCRITICAL, ("Can't load needed resources for search::Engine: ", e.Msg()));
    }
  }

  return m_pSearchEngine.get();
}

TIndex Framework::GetCountryIndex(m2::PointD const & pt) const
{
  return m_storage.FindIndexByFile(GetSearchEngine()->GetCountryFile(pt));
}

string Framework::GetCountryName(m2::PointD const & pt) const
{
  return GetSearchEngine()->GetCountryName(pt);
}

string Framework::GetCountryName(string const & id) const
{
  return GetSearchEngine()->GetCountryName(id);
}

void Framework::PrepareSearch()
{
  GetSearchEngine()->PrepareSearch(GetCurrentViewport());
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

  return GetSearchEngine()->Search(rParams, GetCurrentViewport());
}

bool Framework::GetCurrentPosition(double & lat, double & lon) const
{
  shared_ptr<State> const & locationState = m_informationDisplay.locationState();

  if (locationState->IsModeHasPosition())
  {
    m2::PointD const pos = locationState->Position();
    lat = MercatorBounds::YToLat(pos.y);
    lon = MercatorBounds::XToLon(pos.x);
    return true;
  }
  else
    return false;
}

void Framework::ShowSearchResult(search::Result const & res)
{
  UserMarkContainer::Type const type = UserMarkContainer::SEARCH_MARK;
  m_bmManager.UserMarksSetVisible(type, true);
  m_bmManager.UserMarksClear(type);
  m_bmManager.UserMarksSetDrawable(type, false);

  m_lastSearch.Clear();
  m_fixedSearchResults = 0;

  int scale;
  m2::PointD center;

  using namespace search;
  using namespace feature;

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
  ShowRectExVisibleScale(m_scales.GetRectForDrawScale(scale, center));

  search::AddressInfo info;
  info.MakeFrom(res);

  SearchMarkPoint * mark = static_cast<SearchMarkPoint *>(m_bmManager.UserMarksAddMark(type, center));
  mark->SetInfo(info);

  m_balloonManager.OnShowMark(mark);
}

size_t Framework::ShowAllSearchResults()
{
  using namespace search;

  Results results;
  GetSearchEngine()->GetResults(results);

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

  shared_ptr<State> state = GetLocationState();
  state->SetFixedZoom();
  // Setup viewport according to results.
  m2::AnyRectD viewport = m_navigator.Screen().GlobalRect();
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
    if (!viewport.IsPointInside(pt))
    {
      viewport.SetSizesToIncludePoint(pt);

      ShowRectFixedAR(viewport);
      //StopLocationFollow();
    }
    else
      minInd = -1;
  }

  if (minInd == -1)
    Invalidate();

  return count;
}

void Framework::FillSearchResultsMarks(search::Results const & results)
{
  UserMarkContainer::Type const type = UserMarkContainer::SEARCH_MARK;
  m_bmManager.UserMarksSetVisible(type, true);
  m_bmManager.UserMarksSetDrawable(type, true);
  m_bmManager.UserMarksClear(type, m_fixedSearchResults);

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
      SearchMarkPoint * mark = static_cast<SearchMarkPoint *>(m_bmManager.UserMarksAddMark(type, pt));
      mark->SetInfo(info);
    }
  }
}

void Framework::CancelInteractiveSearch()
{
  m_lastSearch.Clear();
  m_bmManager.UserMarksClear(UserMarkContainer::SEARCH_MARK);

  m_fixedSearchResults = 0;

  Invalidate();
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

    azimut = ang::AngleTo(MercatorBounds::FromLatLon(lat, lon), point) + north;

    double const pi2 = 2.0*math::pi;
    if (azimut < 0.0)
      azimut += pi2;
    else if (azimut > pi2)
      azimut -= pi2;
  }

  // This constant and return value is using for arrow/flag choice.
  return (d < 25000.0);
}

#ifndef USE_DRAPE
void Framework::SetRenderPolicy(RenderPolicy * renderPolicy)
{
  m_bmManager.ResetScreen();
  m_guiController->ResetRenderParams();
  m_renderPolicy.reset();
  m_renderPolicy.reset(renderPolicy);

  if (m_renderPolicy)
  {
    m_renderPolicy->SetAnimController(m_animController.get());

    m_renderPolicy->SetRenderFn(DrawModelFn());

    m_scales.SetParams(m_renderPolicy->VisualScale(), m_renderPolicy->TileSize());

    if (m_benchmarkEngine)
      m_benchmarkEngine->Start();
  }
}

void Framework::InitGuiSubsystem()
{
  if (m_renderPolicy)
  {
    gui::Controller::RenderParams rp(m_renderPolicy->Density(),
                                     bind(&WindowHandle::invalidate,
                                          m_renderPolicy->GetWindowHandle().get()),
                                     m_renderPolicy->GetGlyphCache(),
                                     m_renderPolicy->GetCacheScreen().get());

    m_guiController->SetRenderParams(rp);
    m_informationDisplay.setVisualScale(m_renderPolicy->VisualScale());
    m_balloonManager.RenderPolicyCreated(m_renderPolicy->Density());

    if (m_width != 0 && m_height != 0)
      OnSize(m_width, m_height);

    // init Bookmark manager
    //@{
    graphics::Screen::Params pr;
    pr.m_resourceManager = m_renderPolicy->GetResourceManager();
    pr.m_threadSlot = m_renderPolicy->GetResourceManager()->guiThreadSlot();
    pr.m_renderContext = m_renderPolicy->GetRenderContext();

    pr.m_storageType = graphics::EMediumStorage;
    pr.m_textureType = graphics::ESmallTexture;

    m_bmManager.SetScreen(m_renderPolicy->GetCacheScreen().get());
    //@}

    // Do full invalidate instead of any "pending" stuff.
    Invalidate();
  }
}

RenderPolicy * Framework::GetRenderPolicy() const
{
  return m_renderPolicy.get();
}
#else
void Framework::CreateDrapeEngine(dp::RefPointer<dp::OGLContextFactory> contextFactory, float vs, int w, int h)
{
  typedef df::MapDataProvider::TReadIDsFn TReadIDsFn;
  typedef df::MapDataProvider::TReadFeaturesFn TReadFeaturesFn;
  typedef df::MapDataProvider::TReadIdCallback TReadIdCallback;
  typedef df::MapDataProvider::TReadFeatureCallback TReadFeatureCallback;

  TReadIDsFn idReadFn = [this](TReadIdCallback const & fn, m2::RectD const & r, int scale)
  {
    m_model.ForEachFeatureID(r, fn, scale);
  };

  TReadFeaturesFn featureReadFn = [this](TReadFeatureCallback const & fn, vector<FeatureID> const & ids)
  {
    m_model.ReadFeatures(fn, ids);
  };

  m_drapeEngine.Reset(new df::DrapeEngine(contextFactory, df::Viewport(vs, 0, 0, w, h), df::MapDataProvider(idReadFn, featureReadFn)));
  OnSize(w, h);
}
#endif // USE_DRAPE

void Framework::SetMapStyle(MapStyle mapStyle)
{
  GetStyleReader().SetCurrentStyle(mapStyle);
  drule::LoadRules();
}

MapStyle Framework::GetMapStyle() const
{
  return GetStyleReader().GetCurrentStyle();
}

void Framework::SetupMeasurementSystem()
{
  Settings::Units units = Settings::Metric;
  Settings::Get("Units", units);
  // @TODO(vbykoianko) Rewrites code to use enum class LengthUnits only.
  m_routingSession.SetTurnNotificationsUnits(units == Settings::Foot ?
                                             routing::turns::sound::LengthUnits::Feet :
                                             routing::turns::sound::LengthUnits::Meters);

  m_informationDisplay.measurementSystemChanged();
  Invalidate();
}

string Framework::GetCountryCode(m2::PointD const & pt) const
{
  return GetSearchEngine()->GetCountryCode(pt);
}

gui::Controller * Framework::GetGuiController() const
{
  return m_guiController.get();
}

anim::Controller * Framework::GetAnimController() const
{
  return m_animController.get();
}

bool Framework::ShowMapForURL(string const & url)
{
  m2::PointD point;
  m2::RectD rect;
  string name;
  UserMark const * apiMark = 0;

  enum ResultT { FAILED, NEED_CLICK, NO_NEED_CLICK };
  ResultT result = FAILED;

  // always hide current balloon here
  m_balloonManager.Hide();

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
      rect = m_scales.GetRectForDrawScale(zoom, point);
      name = pt.m_name;
      result = NEED_CLICK;
    }
  }
  else if (StartsWith(url, "mapswithme://") || StartsWith(url, "mwm://"))
  {
    m_bmManager.UserMarksClear(UserMarkContainer::API_MARK);

    if (m_ParsedMapApi.SetUriAndParse(url))
    {
      if (!m_ParsedMapApi.GetViewportRect(m_scales, rect))
        rect = ScalesProcessor::GetWorldRect();

      if ((apiMark = m_ParsedMapApi.GetSinglePoint()))
        result = NEED_CLICK;
      else
        result = NO_NEED_CLICK;
    }
  }
  else  // Actually, we can parse any geo url scheme with correct coordinates.
  {
    Info info;
    ParseGeoURL(url, info);
    if (info.IsValid())
    {
      point = MercatorBounds::FromLatLon(info.m_lat, info.m_lon);
      rect = m_scales.GetRectForDrawScale(info.m_zoom, point);
      result = NEED_CLICK;
    }
  }

  if (result != FAILED)
  {
    // set viewport and stop follow mode if any
    StopLocationFollow();
    ShowRectExVisibleScale(rect);

    if (result != NO_NEED_CLICK)
    {
      if (apiMark)
      {
        LOG(LINFO, ("Show API mark:", static_cast<ApiMarkPoint const *>(apiMark)->GetName()));

        m_balloonManager.OnShowMark(apiMark);
      }
      else
      {
        PoiMarkPoint * mark = GetAddressMark(point);
        if (!name.empty())
          mark->SetName(name);
        m_balloonManager.OnShowMark(mark);
      }
    }
    else
    {
      m_balloonManager.RemovePin();
      m_balloonManager.Dismiss();
    }

    return true;
  }
  else
    return false;
}

void Framework::SetViewPortASync(m2::RectD const & r)
{
  m2::AnyRectD const aRect(r);
  m_animator.ChangeViewport(aRect, aRect, 0.0);
}

void Framework::UpdateSelectedMyPosition(m2::PointD const & pt)
{
  MyPositionMarkPoint * myPositionMark = UserMarkContainer::UserMarkForMyPostion();
  myPositionMark->SetPtOrg(pt);
  ActivateUserMark(myPositionMark, false);
}

void Framework::DisconnectMyPositionUpdate()
{
  if (m_locationChangedSlotID != -1)
  {
    GetLocationState()->RemovePositionChangedListener(m_locationChangedSlotID);
    m_locationChangedSlotID = -1;
  }
}

m2::RectD Framework::GetCurrentViewport() const
{
  return m_navigator.Screen().ClipRect();
}

bool Framework::IsBenchmarking() const
{
  return m_benchmarkEngine != 0;
}

#ifndef USE_DRAPE
namespace
{

typedef shared_ptr<graphics::OverlayElement> OEPointerT;

OEPointerT GetClosestToPivot(list<OEPointerT> const & l, m2::PointD const & pxPoint)
{
  double dist = numeric_limits<double>::max();
  OEPointerT res;

  for (list<OEPointerT>::const_iterator it = l.begin(); it != l.end(); ++it)
  {
    double const curDist = pxPoint.SquareLength((*it)->pivot());
    if (curDist < dist)
    {
      dist = curDist;
      res = *it;
    }
  }

  return res;
}

}
#endif // USE_DRAPE

bool Framework::GetVisiblePOI(m2::PointD const & pxPoint, m2::PointD & pxPivot,
                              search::AddressInfo & info, feature::Metadata & metadata) const
{
#ifndef USE_DRAPE
  ASSERT(m_renderPolicy, ());
  graphics::OverlayElement::UserInfo ui;

  {
    // It seems like we don't need to lock frame here.
    // Overlay locking and storing items as shared_ptr is enough here.
    //m_renderPolicy->FrameLock();

    m2::PointD const pt = m_navigator.ShiftPoint(pxPoint);
    double const halfSize = TOUCH_PIXEL_RADIUS * GetVisualScale();

    list<OEPointerT> candidates;
    m2::RectD const rect(pt.x - halfSize, pt.y - halfSize,
                         pt.x + halfSize, pt.y + halfSize);

    graphics::Overlay * frameOverlay = m_renderPolicy->FrameOverlay();
    frameOverlay->lock();
    frameOverlay->selectOverlayElements(rect, candidates);
    frameOverlay->unlock();

    OEPointerT elem = GetClosestToPivot(candidates, pt);

    if (elem)
      ui = elem->userInfo();

    //m_renderPolicy->FrameUnlock();
  }

  if (ui.IsValid())
  {
    Index::FeaturesLoaderGuard guard(m_model.GetIndex(), ui.m_featureID.m_mwmId);

    FeatureType ft;
    guard.GetFeatureByIndex(ui.m_featureID.m_index, ft);

    ft.ParseMetadata();
    metadata = ft.GetMetadata();

    // @TODO experiment with other pivots
    ASSERT_NOT_EQUAL(ft.GetFeatureType(), feature::GEOM_LINE, ());
    m2::PointD const center = feature::GetCenter(ft);

    GetAddressInfo(ft, center, info);

    pxPivot = GtoP(center);
    return true;
  }
#endif // USE_DRAPE

  return false;
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

Animator & Framework::GetAnimator()
{
  return m_animator;
}

Navigator & Framework::GetNavigator()
{
  return m_navigator;
}

shared_ptr<State> const & Framework::GetLocationState() const
{
  return m_informationDisplay.locationState();
}

void Framework::ActivateUserMark(UserMark const * mark, bool needAnim)
{
  if (mark != UserMarkContainer::UserMarkForMyPostion())
    DisconnectMyPositionUpdate();
  m_bmManager.ActivateMark(mark, needAnim);
}

bool Framework::HasActiveUserMark() const
{
  return m_bmManager.UserMarkHasActive();
}

UserMark const * Framework::GetUserMark(m2::PointD const & pxPoint, bool isLongPress)
{
  // The main idea is to calculate POI rank based on the frequency users are clicking them.
  UserMark const * mark = GetUserMarkWithoutLogging(pxPoint, isLongPress);

  alohalytics::TStringMap details {{"isLongPress", isLongPress ? "1" : "0"}};
  if (mark)
    mark->FillLogEvent(details);
  alohalytics::Stats::Instance().LogEvent("$GetUserMark", details);

  return mark;
}

UserMark const * Framework::GetUserMarkWithoutLogging(m2::PointD const & pxPoint, bool isLongPress)
{
  DisconnectMyPositionUpdate();
  m2::AnyRectD rect;
  m_navigator.GetTouchRect(pxPoint, TOUCH_PIXEL_RADIUS * GetVisualScale(), rect);

  shared_ptr<State> const & locationState = GetLocationState();
  if (locationState->IsModeHasPosition())
  {
    m2::PointD const & glPivot = locationState->Position();
    if (rect.IsPointInside(glPivot))
    {
      search::AddressInfo info;
      info.m_name = m_stringsBundle.GetString("my_position");
      MyPositionMarkPoint * myPostition = UserMarkContainer::UserMarkForMyPostion();
      m_locationChangedSlotID = locationState->AddPositionChangedListener(bind(&Framework::UpdateSelectedMyPosition, this, _1));
      myPostition->SetPtOrg(glPivot);
      myPostition->SetInfo(info);
      return myPostition;
    }
  }

  m2::AnyRectD bmSearchRect;
  double const pxWidth  =  TOUCH_PIXEL_RADIUS * GetVisualScale();
  double const pxHeight = (TOUCH_PIXEL_RADIUS + BM_TOUCH_PIXEL_INCREASE) * GetVisualScale();
  m_navigator.GetTouchRect(pxPoint + m2::PointD(0, BM_TOUCH_PIXEL_INCREASE), pxWidth, pxHeight, bmSearchRect);

  UserMark const * mark = m_bmManager.FindNearestUserMark(
        [&rect, &bmSearchRect](UserMarkContainer::Type type) -> m2::AnyRectD const &
        {
          return (type == UserMarkContainer::BOOKMARK_MARK ? bmSearchRect : rect);
        });

  if (mark == NULL)
  {
    bool needMark = false;
    m2::PointD pxPivot;
    search::AddressInfo info;
    feature::Metadata metadata;
    if (GetVisiblePOI(pxPoint, pxPivot, info, metadata))
      needMark = true;
    else if (isLongPress)
    {
      GetAddressInfoForPixelPoint(pxPoint, info);
      pxPivot = pxPoint;
      needMark = true;
    }

    if (needMark)
    {
      PoiMarkPoint * poiMark = UserMarkContainer::UserMarkForPoi();
      poiMark->SetPtOrg(m_navigator.PtoG(pxPivot));
      poiMark->SetInfo(info);
      poiMark->SetMetadata(move(metadata));
      mark = poiMark;
    }
  }

  return mark;
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
  for (size_t i = 0; i < cat->GetBookmarksCount(); ++i)
  {
    if (mark == cat->GetBookmark(i))
    {
      result.second = i;
      break;
    }
  }

  ASSERT(result != empty, ());
  return result;
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
  double lat = MercatorBounds::YToLat(bmk->GetOrg().y);
  double lon = MercatorBounds::XToLon(bmk->GetOrg().x);
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

void Framework::BuildRoute(m2::PointD const & destination, uint32_t timeoutSec)
{
  shared_ptr<State> const & state = GetLocationState();
  if (!state->IsModeHasPosition())
  {
    CallRouteBuilded(IRouter::NoCurrentPosition, vector<storage::TIndex>(),
                     vector<storage::TIndex>());
    return;
  }

  if (IsRoutingActive())
    CloseRouting();

  SetLastUsedRouter(m_currentRouterType);

  auto readyCallback = [this](Route const & route, IRouter::ResultCode code)
  {
    vector<storage::TIndex> absentCountries;
    vector<storage::TIndex> absentRoutingIndexes;
    if (code == IRouter::NoError)
    {
      InsertRoute(route);
      GetLocationState()->RouteBuilded();
      ShowRectExVisibleScale(route.GetPoly().GetLimitRect());
    }
    else
    {
      for (string const & name : route.GetAbsentCountries())
      {
        storage::TIndex fileIndex = m_storage.FindIndexByFile(name);
        if (m_storage.GetLatestLocalFile(fileIndex) && code != IRouter::FileTooOld)
          absentRoutingIndexes.push_back(fileIndex);
        else
          absentCountries.push_back(fileIndex);
      }

      if (code != IRouter::NeedMoreMaps)
        RemoveRoute();
    }
    CallRouteBuilded(code, absentCountries, absentRoutingIndexes);
  };

  m_routingSession.BuildRoute(state->Position(), destination,
                              [readyCallback](Route const & route, IRouter::ResultCode code)
                              {
                                GetPlatform().RunOnGuiThread(bind(readyCallback, route, code));
                              },
                              m_progressCallback, timeoutSec);
}

void Framework::SetRouter(RouterType type)
{
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
  if (type == RouterType::Pedestrian)
  {
    router = CreatePedestrianAStarBidirectionalRouter(m_model.GetIndex());
    m_routingSession.SetRoutingSettings(routing::GetPedestrianRoutingSettings());
  }
  else
  {
    auto countryFileGetter = [this](m2::PointD const & p) -> string
    {
      // TODO (@gorshenin): fix search engine to return CountryFile
      // instances instead of plain strings.
      return GetSearchEngine()->GetCountryFile(p);
    };
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

void Framework::RemoveRoute()
{
  m_bmManager.UserMarksClear(UserMarkContainer::DEBUG_MARK);

  m_bmManager.ResetRouteTrack();
}

void Framework::CloseRouting()
{
  GetLocationState()->StopRoutingMode();
  m_routingSession.Reset();
  RemoveRoute();
  Invalidate();
}

void Framework::InsertRoute(Route const & route)
{
  if (route.GetPoly().GetSize() < 2)
  {
    LOG(LWARNING, ("Invalid track - only", route.GetPoly().GetSize(), "point(s)."));
    return;
  }

  vector<double> turns;
  if (m_currentRouterType == RouterType::Vehicle)
  {
    turns::TTurnsGeom const & turnsGeom = route.GetTurnsGeometry();
    if (!turnsGeom.empty())
    {
      turns.reserve(turnsGeom.size());
      for (size_t i = 0; i < turnsGeom.size(); i++)
        turns.push_back(turnsGeom[i].m_mercatorDistance);
    }
  }

  /// @todo Consider a style parameter for the route color.
  graphics::Color routeColor;
  if (m_currentRouterType == RouterType::Pedestrian)
    routeColor = graphics::Color(5, 105, 175, 204);
  else
    routeColor = graphics::Color(30, 150, 240, 204);

  m_bmManager.SetRouteTrack(route.GetPoly(), turns, routeColor);

  m_informationDisplay.ResetRouteMatchingInfo();
  Invalidate();
}

void Framework::CheckLocationForRouting(GpsInfo const & info)
{
  if (!IsRoutingActive())
    return;

  m2::PointD const & position = GetLocationState()->Position();
  if (m_routingSession.OnLocationPositionChanged(position, info) == RoutingSession::RouteNeedRebuild)
  {
    auto readyCallback = [this](Route const & route, IRouter::ResultCode code)
    {
      if (code == IRouter::NoError)
        GetPlatform().RunOnGuiThread(bind(&Framework::InsertRoute, this, route));
    };

    m_routingSession.RebuildRoute(position, readyCallback, m_progressCallback, 0 /* timeoutSec */);
  }
}

void Framework::MatchLocationToRoute(location::GpsInfo & location, location::RouteMatchingInfo & routeMatchingInfo,
                                     bool & hasDistanceFromBegin, double & distanceFromBegin) const
{
  if (!IsRoutingActive())
    return;
  m_routingSession.MatchLocationToRoute(location, routeMatchingInfo);
  hasDistanceFromBegin = m_routingSession.GetMercatorDistanceFromBegin(distanceFromBegin);
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
    string routerType;
    Settings::Get(kRouterTypeKey, routerType);
    if (routerType == routing::ToString(RouterType::Pedestrian))
      return RouterType::Pedestrian;
    else
    {
      // Return on a short distance the vehicle router flag only if we are already have routing files.
      auto countryFileGetter = [this](m2::PointD const & p)
      {
        return GetSearchEngine()->GetCountryFile(p);
      };
      if (!OsrmRouter::CheckRoutingAbility(startPoint, finalPoint, countryFileGetter,
                                           &m_model.GetIndex()))
      {
        return RouterType::Pedestrian;
      }
    }
  }
  return RouterType::Vehicle;
}

void Framework::SetLastUsedRouter(RouterType type)
{
  Settings::Set(kRouterTypeKey, routing::ToString(type));
}
