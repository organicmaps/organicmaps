#include "map/guides_manager.hpp"

#include "map/bookmark_catalog.hpp"

#include "partners_api/utm.hpp"

#include "drape_frontend/drape_engine.hpp"
#include "drape_frontend/visual_params.hpp"

#include "platform/platform.hpp"
#include "platform/preferred_languages.hpp"

#include "geometry/intersection_score.hpp"

#include "private.h"

#include <chrono>
#include <utility>

using namespace std::placeholders;

namespace
{
auto constexpr kRequestAttemptsCount = 3;
auto constexpr kErrorTimeout = std::chrono::seconds(5);
// This constant is empirically calculated based on geometry::IntersectionScore.
// When screen scales are different more than 11.15 percent
// it is equal less than 80 percents screen rectangles intersection.
auto constexpr kScaleEps = 0.1115;

auto constexpr kMinViewportsIntersectionScore = 0.9;
auto constexpr kRequestingRectIncrease = 0.6;

using GalleryItem = GuidesManager::GuidesGallery::Item;
using SortedGuides = std::vector<std::pair<m2::PointD, size_t>>;

SortedGuides SortGuidesByPositions(std::vector<guides_on_map::GuidesNode> const & guides,
                                   ScreenBase const & screen)
{
  SortedGuides sortedGuides;
  sortedGuides.reserve(guides.size());
  for (size_t i = 0; i < guides.size(); ++i)
    sortedGuides.emplace_back(screen.GtoP(guides[i].m_point), i);

  std::sort(sortedGuides.begin(), sortedGuides.end(),
            [](auto const & lhs, auto const & rhs)
            {
              if (base::AlmostEqualAbs(lhs.first.y, rhs.first.y, 1e-2))
                return lhs.first.x < rhs.first.x;
              return lhs.first.y < rhs.first.y;
            });
  return sortedGuides;
}
}  // namespace

GuidesManager::GuidesManager(CloseGalleryFn && closeGalleryFn)
  : m_closeGallery(std::move(closeGalleryFn))
  , m_statistics("guides")
{
  CHECK(m_closeGallery != nullptr, ());
}

GuidesManager::GuidesState GuidesManager::GetState() const
{
  return m_state;
}

void GuidesManager::SetStateListener(GuidesStateChangedFn const & onStateChanged)
{
  m_onStateChanged = onStateChanged;
  if (m_onStateChanged != nullptr)
    m_onStateChanged(m_state);
}

void GuidesManager::UpdateViewport(ScreenBase const & screen)
{
  m_lastShownViewport = screen.GlobalRect();
  auto const zoom = df::GetDrawTileScale(screen);

  if (m_state == GuidesState::Disabled || m_state == GuidesState::FatalNetworkError)
  {
    m_screen = screen;
    m_zoom = zoom;
    return;
  }

  if (screen.GlobalRect().GetLocalRect().IsEmptyInterior())
    return;

  m_closeGallery();

  if (IsRequestParamsInitialized())
  {
    auto const scaleStronglyChanged =
      fabs(m_screen.GetScale() - screen.GetScale()) / m_screen.GetScale() > kScaleEps;

    if (!scaleStronglyChanged)
    {
      m2::AnyRectD::Corners currentCorners;
      m_screen.GlobalRect().GetGlobalPoints(currentCorners);

      m2::AnyRectD::Corners screenCorners;
      screen.GlobalRect().GetGlobalPoints(screenCorners);

      auto const score = geometry::GetIntersectionScoreForPoints(currentCorners, screenCorners);

      // If more than |kMinViewportsIntersectionScore| of viewport rect
      // intersects with last requested rect then return.
      if (score > kMinViewportsIntersectionScore)
        return;
    }
  }

  m_screen = screen;
  m_zoom = zoom;

  RequestGuides();
}

void GuidesManager::Reconnect()
{
  if (m_state != GuidesState::FatalNetworkError)
    return;

  ChangeState(GuidesState::Enabled);
  RequestGuides();
}

void GuidesManager::SetEnabled(bool enabled, bool silentMode /* = false */,
                               bool suggestZoom /* = true */)
{
  auto const newState = enabled ? GuidesState::Enabled : GuidesState::Disabled;
  if (newState == m_state)
    return;
  m_drapeEngine.SafeCall(&df::DrapeEngine::EnableGuides, enabled);

  Clear();
  m_shownGuides.clear();
  ChangeState(newState);

  if (!enabled)
    return;

  m_silentMode = silentMode;

  if (!GetPlatform().IsConnected())
  {
    if (m_silentMode)
      ChangeState(GuidesState::Disabled);
    else
      ChangeState(GuidesState::FatalNetworkError);
    return;
  }

  RequestGuides(suggestZoom);
}

bool GuidesManager::IsEnabled() const
{
  return m_state != GuidesState::Disabled;
}

void GuidesManager::ChangeState(GuidesState newState, bool force /* = false */, bool needNotify /* = true */)
{
  if (m_state == newState && !force)
    return;
  m_state = newState;
  if (m_onStateChanged != nullptr && needNotify)
    m_onStateChanged(newState);

  if (m_shownGuides.empty())
    TrackStatistics();
}

void GuidesManager::RequestGuides(bool suggestZoom)
{
  if (!IsRequestParamsInitialized())
    return;

  auto screenRect = m_screen.GlobalRect();

  auto rect = screenRect.GetGlobalRect();

  screenRect.Inflate(rect.SizeX() * (kRequestingRectIncrease / 2),
                     rect.SizeY() * (kRequestingRectIncrease / 2));
  m2::AnyRectD::Corners corners;
  screenRect.GetGlobalPoints(corners);

  for (auto & p : corners)
    mercator::ClampPoint(p);

  auto const requestNumber = ++m_requestCounter;
  auto const pushResult = m_api.GetGuidesOnMap(
      corners, m_zoom, suggestZoom, kRequestingRectIncrease * 100,
      std::bind(&GuidesManager::OnRequestSucceed, this, _1, suggestZoom, requestNumber),
      std::bind(&GuidesManager::OnRequestError, this));

  if (pushResult.m_id != base::TaskLoop::kNoId)
  {
    if (m_previousRequestsId != base::TaskLoop::kNoId)
      GetPlatform().CancelTask(Platform::Thread::Network, m_previousRequestsId);

    m_previousRequestsId = pushResult.m_id;
  }
}

void GuidesManager::OnRequestSucceed(guides_on_map::GuidesOnMap const & guides, bool suggestZoom,
                                     uint64_t requestNumber)
{
  if (m_state == GuidesState::Disabled)
    return;

  m_errorRequestsCount = 0;
  if (m_retryAfterErrorRequestId != base::TaskLoop::kNoId)
  {
    GetPlatform().CancelTask(Platform::Thread::Background, m_retryAfterErrorRequestId);
    m_retryAfterErrorRequestId = base::TaskLoop::kNoId;
  }

  if (requestNumber != m_requestCounter)
    return;

  m_guides = guides;

  if (!m_guides.m_nodes.empty())
  {
    ChangeState(GuidesState::HasData);
  }
  else
  {
    if (suggestZoom &&
        m_guides.m_suggestedZoom != guides_on_map::GuidesOnMap::kIncorrectZoom &&
        m_zoom > m_guides.m_suggestedZoom)
    {
      m_drapeEngine.SafeCall(&df::DrapeEngine::SetModelViewCenter,
                             m_lastShownViewport.GetGlobalRect().Center(), m_guides.m_suggestedZoom,
                             true /* isAnim */, false /* trackVisibleViewport */);
    }
    else
    {
      if (!m_silentMode)
        ChangeState(GuidesState::NoData, false /* force */, !m_silentMode /* needNotify */);
    }
  }

  UpdateGuidesMarks();

  if (m_activeGuide.empty())
  {
    m_closeGallery();
  }
  else
  {
    if (m_onGalleryChanged)
      m_onGalleryChanged(true /* reload */);
  }
}

void GuidesManager::OnRequestError()
{
  if (m_state == GuidesState::Disabled || m_state == GuidesState::FatalNetworkError ||
      m_retryAfterErrorRequestId != base::TaskLoop::kNoId)
  {
    return;
  }

  if (++m_errorRequestsCount >= kRequestAttemptsCount)
  {
    Clear();
    if (m_silentMode)
      ChangeState(GuidesState::Disabled);
    else
      ChangeState(GuidesState::FatalNetworkError);
    return;
  }

  ChangeState(GuidesState::NetworkError, true /* force */, !m_silentMode /* needNotify */);

  auto const pushResult =
      GetPlatform().RunDelayedTask(Platform::Thread::Background, kErrorTimeout, [this]() {
        GetPlatform().RunTask(Platform::Thread::Gui, [this]() {
          if (m_state != GuidesState::NetworkError)
            return;

          m_retryAfterErrorRequestId = base::TaskLoop::kNoId;
          RequestGuides();
        });
      });

  m_retryAfterErrorRequestId = pushResult.m_id;
}

void GuidesManager::Clear()
{
  m_activeGuide.clear();
  m_guides = {};
  m_errorRequestsCount = 0;

  UpdateGuidesMarks();
  m_closeGallery();
}

GuidesManager::GuidesGallery GuidesManager::GetGallery() const
{
  GuidesGallery gallery;
  for (auto const & guide : m_guides.m_nodes)
  {
    if (guide.m_outdoorCount + guide.m_sightsCount != 1 ||
        !m_lastShownViewport.IsPointInside(guide.m_point))
    {
      continue;
    }

    if (guide.m_guideInfo.m_id == m_activeGuide)
      gallery.m_items.emplace_front(MakeGalleryItem(guide));
    else
      gallery.m_items.emplace_back(MakeGalleryItem(guide));
  }

  return gallery;
}

std::string GuidesManager::GetActiveGuide() const
{
  return m_activeGuide;
}

void GuidesManager::SetActiveGuide(std::string const & guideId)
{
  if (m_activeGuide == guideId)
    return;

  m_activeGuide = guideId;
  UpdateActiveGuide();
}

void GuidesManager::ResetActiveGuide()
{
  if (m_state == GuidesState::Disabled)
    return;

  if (m_activeGuide.empty())
    return;

  m_activeGuide.clear();
  auto es = m_bmManager->GetEditSession();
  es.ClearGroup(UserMark::Type::GUIDE_SELECTION);
}

uint64_t GuidesManager::GetShownGuidesCount() const
{
  return m_shownGuides.size();
}

void GuidesManager::SetGalleryListener(GuidesGalleryChangedFn const & onGalleryChanged)
{
  m_onGalleryChanged = onGalleryChanged;
}

void GuidesManager::SetBookmarkManager(BookmarkManager * bmManager)
{
  CHECK(bmManager != nullptr, ());
  m_bmManager = bmManager;
  m_bmManager->SetCategoriesChangedCallback(
      [this]()
      {
        GetPlatform().RunTask(Platform::Thread::Gui, [this](){ UpdateDownloadedStatus(); });
      });
}

void GuidesManager::SetDrapeEngine(ref_ptr<df::DrapeEngine> engine)
{
  m_drapeEngine.Set(engine);
}

void GuidesManager::SetApiDelegate(std::unique_ptr<guides_on_map::Api::Delegate> apiDelegate)
{
  m_api.SetDelegate(std::move(apiDelegate));
}

bool GuidesManager::IsGuideDownloaded(std::string const & guideId) const
{
  return m_bmManager->GetCatalog().HasDownloaded(guideId);
}

void GuidesManager::UpdateDownloadedStatus()
{
  if (m_state == GuidesState::Disabled)
    return;

  bool changed = false;
  auto es = m_bmManager->GetEditSession();
  for (auto markId : m_bmManager->GetUserMarkIds(UserMark::Type::GUIDE))
  {
    auto * mark = es.GetMarkForEdit<GuideMark>(markId);
    auto const isDownloaded = IsGuideDownloaded(mark->GetGuideId());
    if (isDownloaded != mark->GetIsDownloaded())
    {
      changed = true;
      mark->SetIsDownloaded(isDownloaded);
    }
  }

  if (changed && m_onGalleryChanged)
    m_onGalleryChanged(true /* reload */);
}

void GuidesManager::UpdateGuidesMarks()
{
  auto es = m_bmManager->GetEditSession();
  es.ClearGroup(UserMark::Type::GUIDE_CLUSTER);
  es.ClearGroup(UserMark::Type::GUIDE);

  auto const sortedGuides = SortGuidesByPositions(m_guides.m_nodes, m_screen);

  float depth = 0.0f;
  for (auto const & guidePos : sortedGuides)
  {
    auto const & guide = m_guides.m_nodes[guidePos.second];
    if (guide.m_sightsCount + guide.m_outdoorCount > 1)
    {
      GuidesClusterMark * mark = es.CreateUserMark<GuidesClusterMark>(guide.m_point);
      mark->SetGuidesCount(guide.m_sightsCount, guide.m_outdoorCount);
      mark->SetDepth(depth);
    }
    else
    {
      GuideMark * mark = es.CreateUserMark<GuideMark>(guide.m_point);
      mark->SetGuideType(guide.m_sightsCount > 0 ? GuideMark::Type::City
                                                 : GuideMark::Type::Outdoor);
      mark->SetGuideId(guide.m_guideInfo.m_id);
      mark->SetIsDownloaded(IsGuideDownloaded(guide.m_guideInfo.m_id));
      mark->SetDepth(depth);
      m_shownGuides.insert(guide.m_guideInfo.m_id);
    }
    depth += 1.0f;
  }
  UpdateActiveGuide();
}

void GuidesManager::OnClusterSelected(GuidesClusterMark const & mark, ScreenBase const & screen)
{
  m_drapeEngine.SafeCall(&df::DrapeEngine::StopLocationFollow);
  m_drapeEngine.SafeCall(&df::DrapeEngine::ScaleAndSetCenter, mark.GetPivot(),
                         2.0 /* scaleFactor */, true /* isAnim */,
                         false /* trackVisibleViewport */);
  m_statistics.LogItemSelected(LayersStatistics::LayerItemType::Cluster);
}

void GuidesManager::OnGuideSelected()
{
  if (m_onGalleryChanged)
    m_onGalleryChanged(false /* reload */);
}

void GuidesManager::LogGuideSelectedStatistic()
{
  m_statistics.LogItemSelected(LayersStatistics::LayerItemType::Point);
}

void GuidesManager::UpdateActiveGuide()
{
  auto es = m_bmManager->GetEditSession();
  es.ClearGroup(UserMark::Type::GUIDE_SELECTION);
  auto const ids = m_bmManager->GetUserMarkIds(UserMark::Type::GUIDE);
  for (auto markId : ids)
  {
    GuideMark const * mark = m_bmManager->GetMark<GuideMark>(markId);
    if (mark->GetGuideId() == m_activeGuide)
    {
      es.CreateUserMark<GuideSelectionMark>(mark->GetPivot());
      return;
    }
  }
  m_activeGuide.clear();
}

bool GuidesManager::IsRequestParamsInitialized() const
{
  return m_screen.GlobalRect().GetLocalRect().IsEmptyInterior() || m_zoom != 0;
}

void GuidesManager::TrackStatistics() const
{
  auto const initType = m_silentMode ? LayersStatistics::InitType::Auto
                                     : LayersStatistics::InitType::User;
  if (m_state == GuidesState::HasData)
    m_statistics.LogActivate(LayersStatistics::Status::Success, {} /* mwmVersions */, initType);
  else if (m_state == GuidesState::NoData)
    m_statistics.LogActivate(LayersStatistics::Status::Unavailable, {} /* mwmVersions */, initType);
  else if (m_state == GuidesState::NetworkError || m_state == GuidesState::FatalNetworkError)
    m_statistics.LogActivate(LayersStatistics::Status::Error, {} /* mwmVersions */, initType);
}

GalleryItem GuidesManager::MakeGalleryItem(guides_on_map::GuidesNode const & guide) const
{
  auto const & info = guide.m_guideInfo;

  GuidesGallery::Item item;
  item.m_guideId = info.m_id;

  auto url = url::Join(BOOKMARKS_CATALOG_FRONT_URL, languages::GetCurrentNorm(),
                       "v3/mobilefront/route", info.m_id);
  url = InjectUTM(url, UTM::GuidesOnMapGallery);
  url = InjectUTMTerm(url, std::to_string(m_shownGuides.size()));

  item.m_url = std::move(url);
  item.m_imageUrl = info.m_imageUrl;
  item.m_title = info.m_name;
  item.m_downloaded = IsGuideDownloaded(info.m_id);

  if (guide.m_sightsCount == 1)
  {
    item.m_type = GuidesGallery::Item::Type::City;
    item.m_cityParams.m_bookmarksCount = info.m_bookmarksCount;
    item.m_cityParams.m_trackIsAvailable = info.m_hasTrack;
  }
  else
  {
    item.m_type = GuidesGallery::Item::Type::Outdoor;
    item.m_outdoorsParams.m_duration = info.m_tourDuration;
    item.m_outdoorsParams.m_distance = info.m_tracksLength;
    item.m_outdoorsParams.m_ascent = info.m_ascent;
    item.m_outdoorsParams.m_tag = info.m_tag;
  }

  return item;
}

std::string DebugPrint(GuidesManager::GuidesState state)
{
  switch (state)
  {
  case GuidesManager::GuidesState::Disabled: return "Disabled";
  case GuidesManager::GuidesState::Enabled: return "Enabled";
  case GuidesManager::GuidesState::HasData: return "HasData";
  case GuidesManager::GuidesState::NoData: return "NoData";
  case GuidesManager::GuidesState::NetworkError: return "NetworkError";
  case GuidesManager::GuidesState::FatalNetworkError: return "FatalNetworkError";
  }
  UNREACHABLE();
}
