#include "map/framework.hpp"
#ifndef USE_DRAPE
  #include "feature_processor.hpp"
  #include "drawer.hpp"
#else
  #include "../drape_frontend/visual_params.hpp"
#endif // USE_DRAPE

#include "map/benchmark_provider.hpp"
#include "map/benchmark_engine.hpp"
#include "map/geourl_process.hpp"
#include "map/dialog_settings.hpp"
#include "map/ge0_parser.hpp"

#include "defines.hpp"

#include "routing/route.hpp"
#include "routing/osrm_router.hpp"
#include "routing/astar_router.hpp"

#include "search/search_engine.hpp"
#include "search/result.hpp"

#include "indexer/categories_holder.hpp"
#include "indexer/feature.hpp"
#include "indexer/mwm_version.hpp"
#include "indexer/scales.hpp"
#include "indexer/classificator_loader.hpp"

/// @todo Probably it's better to join this functionality.
//@{
#include "indexer/feature_algo.hpp"
#include "indexer/feature_utils.hpp"
//@}

#include "anim/controller.hpp"

#include "gui/controller.hpp"

#include "platform/measurement_utils.hpp"
#include "platform/settings.hpp"
#include "platform/preferred_languages.hpp"
#include "platform/platform.hpp"

#include "coding/internal/file_data.hpp"
#include "coding/zip_reader.hpp"
#include "coding/url_encode.hpp"
#include "coding/file_name_utils.hpp"

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
}

pair<MwmSet::MwmLock, bool> Framework::RegisterMap(string const & file)
{
  LOG(LINFO, ("Loading map:", file));

  pair<MwmSet::MwmLock, bool> p = m_model.RegisterMap(file);
  if (!p.second)
    return p;
  MwmSet::MwmLock const & lock = p.first;
  ASSERT(lock.IsLocked(), ());

  MwmInfo const & info = lock.GetInfo();

  if (info.m_version.format == version::v1)
  {
    // Now we do force delete of old (April 2011) maps.
    LOG(LINFO, ("Deleting old map:", file));

    DeregisterMap(file);
    VERIFY(my::DeleteFileX(GetPlatform().WritablePathForFile(file)), ());
    return make_pair(MwmSet::MwmLock(), false);
  }

  return p;
}

void Framework::DeregisterMap(string const & file) { m_model.DeregisterMap(file); }

void Framework::OnLocationError(TLocationError /*error*/)
{
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
  location::RouteMatchingInfo routeMatchingInfo;
  CheckLocationForRouting(rInfo);
  MatchLocationToRoute(rInfo, routeMatchingInfo);

  shared_ptr<State> const & state = GetLocationState();
  state->OnLocationUpdate(rInfo, m_routingSession.IsNavigable(), routeMatchingInfo);

  if (state->IsModeChangeViewport())
    UpdateUserViewportChanged();
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

void Framework::GetMaps(vector<string> & maps) const
{
  Platform & pl = GetPlatform();

  pl.GetFilesByExt(pl.ResourcesDir(), DATA_FILE_EXTENSION, maps);
  pl.GetFilesByExt(pl.WritableDir(), DATA_FILE_EXTENSION, maps);

  // Remove duplicate maps if they're both present in Resources and in Writable dirs.
  sort(maps.begin(), maps.end());
  maps.erase(unique(maps.begin(), maps.end()), maps.end());

#ifdef OMIM_OS_ANDROID
  // On Android World and WorldCoasts can be stored in alternative /Android/obb/ path.
  char const * arrCheck[] = { WORLD_FILE_NAME DATA_FILE_EXTENSION,
                              WORLD_COASTS_FILE_NAME DATA_FILE_EXTENSION };

  for (size_t i = 0; i < ARRAY_SIZE(arrCheck); ++i)
  {
    auto const it = lower_bound(maps.begin(), maps.end(), arrCheck[i]);
    if (it == maps.end() || *it != arrCheck[i])
      maps.insert(it, arrCheck[i]);
  }
#endif
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
  m_stringsBundle.SetDefaultString("country_status_added_to_queue", "^\nis added to the downloading queue");
  m_stringsBundle.SetDefaultString("country_status_downloading", "Downloading\n^\n^%");
  m_stringsBundle.SetDefaultString("country_status_download", "Download map\n^ MB");
  m_stringsBundle.SetDefaultString("country_status_download_routing", "Download Map + Routing\n^ MB");
  m_stringsBundle.SetDefaultString("country_status_download_failed", "Downloading\n^\nhas failed");
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
  LOG(LDEBUG, ("Classificator initialized"));

  // To avoid possible races - init search engine once in constructor.
  (void)GetSearchEngine();
  LOG(LDEBUG, ("Search engine initialized"));

#ifndef OMIM_OS_ANDROID
  RegisterAllMaps();
#endif
  LOG(LDEBUG, ("Maps initialized"));

  // Init storage with needed callback.
  m_storage.Init(bind(&Framework::UpdateAfterDownload, this, _1, _2));
  LOG(LDEBUG, ("Storage initialized"));

#ifdef USE_PEDESTRIAN_ROUTER
  SetRouter(RouterType::Pedestrian);
#else
  SetRouter(RouterType::Vehicle);
#endif
  LOG(LDEBUG, ("Routing engine initialized"));

  LOG(LINFO, ("System languages:", languages::GetPreferred()));
}

Framework::~Framework()
{
  delete m_benchmarkEngine;
}

double Framework::GetVisualScale() const
{
  return m_scales.GetVisualScale();
}

void Framework::DeleteCountry(TIndex const & index, TMapOptions opt)
{
  if (opt & TMapOptions::EMapOnly)
    opt = TMapOptions::EMapWithCarRouting;

  if (!m_storage.DeleteFromDownloader(index))
  {
    CountryFile const & file = m_storage.CountryByIndex(index).GetFile();

    if (opt & TMapOptions::EMapOnly)
    {
      if (m_model.DeleteMap(file.GetFileWithExt(TMapOptions::EMapOnly)))
        InvalidateRect(GetCountryBounds(file.GetFileWithoutExt()), true);
    }

    if (opt & TMapOptions::ECarRouting)
      m_routingSession.DeleteIndexFile(file.GetFileWithExt(TMapOptions::ECarRouting));

    m_storage.NotifyStatusChanged(index);

    DeleteCountryIndexes(m_storage.CountryFileNameWithoutExt(index));
  }
}

void Framework::DeleteCountryIndexes(string const & mwmName)
{
  m_routingSession.Reset();

  Platform::FilesList files;
  Platform const & pl = GetPlatform();
  string const path = pl.WritablePathForCountryIndexes(mwmName);

  /// @todo We need correct regexp for any file (not including "." and "..").
  pl.GetFilesByRegExp(path, mwmName + "\\..*", files);
  for (auto const & file : files)
    (void) my::DeleteFileX(path + file);
}

void Framework::DownloadCountry(TIndex const & index, TMapOptions opt)
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
  return GetCountryBounds(m_storage.CountryFileNameWithoutExt(index));
}

void Framework::ShowCountry(TIndex const & index)
{
  StopLocationFollow();

  ShowRectEx(GetCountryBounds(index));
}

void Framework::UpdateAfterDownload(string const & fileName, TMapOptions opt)
{
  if (opt & TMapOptions::EMapOnly)
  {
    // Delete old (splitted) map files, if any.
    char const * arr[] = { "Japan", "Brazil" };
    for (size_t i = 0; i < ARRAY_SIZE(arr); ++i)
      if (fileName.find(arr[i]) == 0)
      {
        if (m_model.DeleteMap(string(arr[i]) + DATA_FILE_EXTENSION))
          Invalidate(true);

        // Routing index doesn't exist for this map files.
      }

    // Add downloaded map.
    pair<MwmSet::MwmLock, Index::UpdateStatus> const p = m_model.UpdateMap(fileName);
    if (p.second == Index::UPDATE_STATUS_OK)
    {
      MwmSet::MwmLock const & lock = p.first;
      ASSERT(lock.IsLocked(), ());
      InvalidateRect(lock.GetInfo().m_limitRect, true);
    }

    GetSearchEngine()->ClearViewportsCache();
  }

  // Replace routing file.
  if (opt & TMapOptions::ECarRouting)
  {
    string routingName = fileName + ROUTING_FILE_EXTENSION;
    m_routingSession.DeleteIndexFile(routingName);

    routingName = GetPlatform().WritableDir() + routingName;
    VERIFY(my::RenameFileX(routingName + READY_FILE_EXTENSION, routingName), ());
  }

  string countryName(fileName);
  my::GetNameWithoutExt(countryName);
  DeleteCountryIndexes(countryName);
}

void Framework::RegisterAllMaps()
{
  //ASSERT(m_model.IsEmpty(), ());

  int minVersion = numeric_limits<int>::max();

  vector<string> maps;
  GetMaps(maps);
  for_each(maps.begin(), maps.end(), [&](string const & file)
  {
    pair<MwmSet::MwmLock, bool> const p = RegisterMap(file);
    if (p.second)
    {
      MwmSet::MwmLock const & lock = p.first;
      ASSERT(lock.IsLocked(), ());
      minVersion = min(minVersion, static_cast<int>(lock.GetInfo().m_version.format));
    }
  });

  m_countryTree.Init(maps);

  GetSearchEngine()->SupportOldFormat(minVersion < version::v3);
}

void Framework::DeregisterAllMaps()
{
  m_countryTree.Clear();
  m_model.DeregisterAllMaps();
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
  m_navigator.SaveState();
}

bool Framework::LoadState()
{
  bool r = m_navigator.LoadState();
#ifdef USE_DRAPE
  if (!m_drapeEngine.IsNull())
    m_drapeEngine->UpdateCoverage(m_navigator.Screen());
#endif
  return r;
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
    m_informationDisplay.setDisplayRect(m2::RectI(0, 0, w, h));
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

  Drawer * pDrawer = e->drawer();
  graphics::Screen * pScreen = pDrawer->screen();

  pScreen->beginFrame();

  bool const isEmptyModel = m_renderPolicy->IsEmptyModel();

  if (isEmptyModel)
    m_informationDisplay.setEmptyCountryIndex(GetCountryIndex(GetViewportCenter()));
  else
    m_informationDisplay.setEmptyCountryIndex(storage::TIndex());

  bool const isCompassEnabled = my::Abs(ang::GetShortestDistance(m_navigator.Screen().GetAngle(), 0.0)) > my::DegToRad(3.0);
  bool const isCompasActionEnabled = m_informationDisplay.isCompassArrowEnabled() && m_navigator.InAction();

  m_informationDisplay.enableCompassArrow(isCompassEnabled || isCompasActionEnabled);
  m_informationDisplay.setCompassArrowAngle(m_navigator.Screen().GetAngle());

  int const drawScale = GetDrawScale();
  m_informationDisplay.setDebugInfo(0, drawScale);

  m_informationDisplay.enableRuler(drawScale > 4 && !m_informationDisplay.isCopyrightActive());

  pScreen->endFrame();

  m_bmManager.DrawItems(e);
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

m2::AnyRectD Framework::ToRotated(m2::RectD const & rect) const
{
  double const dx = rect.SizeX();
  double const dy = rect.SizeY();

  return m2::AnyRectD(rect.Center(),
                      m_navigator.Screen().GetAngle(),
                      m2::RectD(-dx/2, -dy/2, dx/2, dy/2));
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
  ShowRectEx(m_scales.GetRectForDrawScale(zoom, center));
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
  ShowRectFixedAR(ToRotated(rect));
}

void Framework::ShowRectFixedAR(m2::AnyRectD const & rect)
{
  double const halfSize = m_scales.GetTileSize() / 2.0;
  m2::RectD etalonRect(-halfSize, -halfSize, halfSize, halfSize);

  m2::PointD const pxCenter = m_navigator.Screen().PixelRect().Center();
  etalonRect.Offset(pxCenter);

  m_navigator.SetFromRects(rect, etalonRect);

  Invalidate();
}

void Framework::UpdateUserViewportChanged()
{
  if (IsISActive())
  {
    (void)GetCurrentPosition(m_lastSearch.m_lat, m_lastSearch.m_lon);
    m_lastSearch.m_callback = bind(&Framework::OnSearchResultsCallback, this, _1);
    m_lastSearch.SetSearchMode(search::SearchParams::IN_VIEWPORT);
    m_lastSearch.SetForceSearch(false);

    (void)GetSearchEngine()->Search(m_lastSearch, GetCurrentViewport(), true);
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
  // Do not clear caches for Android. This function is called when main activity is paused,
  // but at the same time search activity (for example) is enabled.
#ifndef OMIM_OS_ANDROID
  ClearAllCaches();
#endif

  dlg_settings::EnterBackground(my::Timer::LocalTime() - m_StartForegroundTime);
}

void Framework::EnterForeground()
{
  m_StartForegroundTime = my::Timer::LocalTime();
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
                              &m_model.GetIndex(),
                              pl.GetReader(SEARCH_CATEGORIES_FILE_NAME),
                              pl.GetReader(PACKED_POLYGONS_FILE),
                              pl.GetReader(COUNTRIES_FILE),
                              languages::GetCurrentOrig()));
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

void Framework::PrepareSearch(bool hasPt, double lat, double lon)
{
  GetSearchEngine()->PrepareSearch(GetCurrentViewport(), hasPt, lat, lon);
}

bool Framework::Search(search::SearchParams const & params)
{
  if (params.m_query == ROUTING_SECRET_UNLOCKING_WORD)
  {
    LOG(LINFO, ("Pedestrian routing mode enabled"));
    SetRouter(RouterType::Pedestrian);
    return false;
  }
  if (params.m_query == ROUTING_SECRET_LOCKING_WORD)
  {
    LOG(LINFO, ("Vehicle routing mode enabled"));
    SetRouter(RouterType::Vehicle);
    return false;
  }
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
      Index::FeaturesLoaderGuard guard(m_model.GetIndex(), id.m_mwm);

      FeatureType ft;
      guard.GetFeature(id.m_offset, ft);

      scale = GetFeatureViewportScale(TypesHolder(ft));
      center = GetCenter(ft, scale);
      break;
    }

    case Result::RESULT_LATLON:
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
  drule::SetCurrentMapStyle(mapStyle);
  drule::LoadRules();
}

MapStyle Framework::GetMapStyle() const
{
  return drule::GetCurrentMapStyle();
}

void Framework::SetupMeasurementSystem()
{
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
                              search::AddressInfo & info, feature::FeatureMetadata & metadata) const
{
#ifndef USE_DRAPE
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
    Index::FeaturesLoaderGuard guard(m_model.GetIndex(), ui.m_mwmID);

    FeatureType ft;
    guard.GetFeature(ui.m_offset, ft);

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

  void LoadMetadata(model::FeaturesFetcher const & model, feature::FeatureMetadata & metadata) const
  {
    if (!m_id.IsValid())
      return;

    Index::FeaturesLoaderGuard guard(model.GetIndex(), m_id.m_mwm);

    FeatureType ft;
    guard.GetFeature(m_id.m_offset, ft);

    ft.ParseMetadata();
    metadata = ft.GetMetadata();
  }
};

}

void Framework::FindClosestPOIMetadata(m2::PointD const & pt, feature::FeatureMetadata & metadata) const
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

namespace
{
  class MainTouchRectHolder : public BookmarkManager::TouchRectHolder
  {
  public:
    MainTouchRectHolder(m2::AnyRectD const & defaultRect, m2::AnyRectD const & bmRect)
      : m_defRect(defaultRect)
      , m_bmRect(bmRect)
    {
    }

    m2::AnyRectD const & GetTouchArea(UserMarkContainer::Type type) const
    {
      switch (type)
      {
      case UserMarkContainer::BOOKMARK_MARK:
        return m_bmRect;
      default:
        return m_defRect;
      }
    }

  private:
    m2::AnyRectD const & m_defRect;
    m2::AnyRectD const & m_bmRect;
  };
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
  MainTouchRectHolder holder(rect, bmSearchRect);
  UserMark const * mark = m_bmManager.FindNearestUserMark(holder);

  if (mark == NULL)
  {
    bool needMark = false;
    m2::PointD pxPivot;
    search::AddressInfo info;
    feature::FeatureMetadata metadata;
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
      poiMark->SetMetadata(metadata);
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

bool Framework::IsRoutingActive() const
{
  return m_routingSession.IsActive();
}

bool Framework::IsRouteBuilt() const
{
  return m_routingSession.IsBuilt();
}

void Framework::BuildRoute(m2::PointD const & destination)
{
  shared_ptr<State> const & state = GetLocationState();
  if (!state->IsModeHasPosition())
  {
    CallRouteBuilded(IRouter::NoCurrentPosition, vector<storage::TIndex>());
    return;
  }

  if (IsRoutingActive())
    CloseRouting();

  m_routingSession.BuildRoute(state->Position(), destination,
    [this] (Route const & route, IRouter::ResultCode code)
    {
      vector<storage::TIndex> absentFiles;
      if (code == IRouter::NoError)
      {
        if(route.GetPoly().GetSize() < 2)
        {
          LOG(LWARNING, ("Invalide track for drawing. Have only ",route.GetPoly().GetSize(), "points"));
          return;
        }
        InsertRoute(route);
        GetLocationState()->RouteBuilded();
        ShowRectExVisibleScale(route.GetPoly().GetLimitRect());
      }
      else
      {
        for(string const & name : route.GetAbsentCountries())
          absentFiles.push_back(m_storage.FindIndexByFile(name));

        RemoveRoute();
      }
      CallRouteBuilded(code, absentFiles);
    });
}

void Framework::SetRouteBuildingListener(TRouteBuildingCallback const & callback)
{
  m_routingCallback = callback;
}

void Framework::FollowRoute()
{
  GetLocationState()->StartRouteFollow();
}

void Framework::SetRouter(RouterType type)
{
  if (type == RouterType::Pedestrian)
    m_routingSession.SetRouter(unique_ptr<IRouter>(new AStarRouter(&m_model.GetIndex())));
  else
    m_routingSession.SetRouter(unique_ptr<IRouter>(new OsrmRouter(&m_model.GetIndex(), [this] (m2::PointD const & pt)
    {
      return GetSearchEngine()->GetCountryFile(pt);
    })));
}

void Framework::RemoveRoute()
{
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
  float const visScale = GetVisualScale();

  RouteTrack track(route.GetPoly());
  track.SetName(route.GetName());
  track.SetTurnsGeometry(route.GetTurnsGeometry());

  Track::TrackOutline outlines[]
  {
    { 10.0f * visScale, graphics::Color(110, 180, 240, 255) }
  };

  track.AddOutline(outlines, ARRAY_SIZE(outlines));
  track.AddClosingSymbol(false, "route_to", graphics::EPosCenter, graphics::routingFinishDepth);

  m_bmManager.SetRouteTrack(track);
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
    m_routingSession.RebuildRoute(position, [this] (Route const & route, IRouter::ResultCode code)
    {
      if (code == IRouter::NoError)
        InsertRoute(route);
    });
  }
}

void Framework::MatchLocationToRoute(location::GpsInfo & location, location::RouteMatchingInfo & routeMatchingInfo) const
{
  if (!IsRoutingActive())
    return;
  m_routingSession.MatchLocationToRoute(location, routeMatchingInfo);
}

void Framework::CallRouteBuilded(IRouter::ResultCode code, vector<storage::TIndex> const & absentFiles)
{
  if (code == IRouter::Cancelled)
    return;
  m_routingCallback(code, absentFiles);
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

void Framework::GetRouteFollowingInfo(location::FollowingInfo & info) const
{
  m_routingSession.GetRouteFollowingInfo(info);
}
