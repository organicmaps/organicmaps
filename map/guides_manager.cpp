#include "map/guides_manager.hpp"

#include "map/bookmark_catalog.hpp"

#include "partners_api/utm.hpp"

#include "drape_frontend/drape_engine.hpp"
#include "drape_frontend/visual_params.hpp"

#include "platform/platform.hpp"
#include "platform/preferred_languages.hpp"

#include "geometry/intersection_score.hpp"

#include "private.h"

#include <utility>

namespace
{
auto constexpr kRequestAttemptsCount = 3;
// This constant is empirically calculated based on geometry::IntersectionScore.
// When screen scales are different more than 11.15 percent
// it is equal less than 80 percents screen rectangles intersection.
auto constexpr kScaleEps = 0.1115;

auto constexpr kMinViewportsIntersectionScore = 0.9;
auto constexpr kRequestingRectSidesIncrease = 0.3;
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
  m_lastShownViewport = m_screen.GlobalRect();
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

void GuidesManager::Invalidate()
{
  // TODO: Implement.
}

void GuidesManager::Reconnect()
{
  if (m_state != GuidesState::FatalNetworkError)
    return;

  ChangeState(GuidesState::Enabled);
  RequestGuides();
}

void GuidesManager::SetEnabled(bool enabled)
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

  if (!GetPlatform().IsConnected())
  {
    ChangeState(GuidesState::FatalNetworkError);
    return;
  }

  RequestGuides(true /* suggestZoom */);
}

bool GuidesManager::IsEnabled() const
{
  return m_state != GuidesState::Disabled;
}

void GuidesManager::ChangeState(GuidesState newState)
{
  if (m_state == newState)
    return;
  m_state = newState;
  if (m_onStateChanged != nullptr)
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

  screenRect.Inflate(rect.SizeX() * kRequestingRectSidesIncrease,
                     rect.SizeY() * kRequestingRectSidesIncrease);
  m2::AnyRectD::Corners corners;
  screenRect.GetGlobalPoints(corners);

  for (auto & p : corners)
    mercator::ClampPoint(p);

  auto const requestNumber = ++m_requestCounter;
  auto const id = m_api.GetGuidesOnMap(
      corners, m_zoom, suggestZoom,
      [this, suggestZoom, requestNumber](guides_on_map::GuidesOnMap const & guides) {
        if (m_state == GuidesState::Disabled || requestNumber != m_requestCounter)
          return;

        m_guides = guides;
        m_errorRequestsCount = 0;

        if (!m_guides.m_nodes.empty())
        {
          ChangeState(GuidesState::HasData);
        }
        else
        {
          if (suggestZoom && m_zoom > m_guides.m_suggestedZoom)
          {
            m_drapeEngine.SafeCall(&df::DrapeEngine::SetModelViewCenter,
                                   m_lastShownViewport.GetGlobalRect().Center(),
                                   m_guides.m_suggestedZoom,
                                   true /* isAnim */, false /* trackVisibleViewport */);
          }
          else
          {
            ChangeState(GuidesState::NoData);
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
      },
      [this, requestNumber]() mutable {
        if (m_state == GuidesState::Disabled || m_state == GuidesState::FatalNetworkError)
          return;

        if (++m_errorRequestsCount >= kRequestAttemptsCount)
        {
          Clear();
          ChangeState(GuidesState::FatalNetworkError);
        }
        else
        {
          ChangeState(GuidesState::NetworkError);
        }

        // Re-request only when no additional requests enqueued.
        if (requestNumber == m_requestCounter)
          RequestGuides();
      });

  if (id != base::TaskLoop::kIncorrectId)
  {
    if (m_previousRequestsId != base::TaskLoop::kIncorrectId)
      GetPlatform().CancelTask(Platform::Thread::Network, m_previousRequestsId);

    m_previousRequestsId = id;
  }
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
    if (guide.m_outdoorCount + guide.m_sightsCount != 1)
      continue;

    if (!m_lastShownViewport.IsPointInside(guide.m_point))
      continue;

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
      item.m_cityParams.m_bookmarksCount = guide.m_guideInfo.m_bookmarksCount;
      item.m_cityParams.m_trackIsAvailable = guide.m_guideInfo.m_hasTrack;
    }
    else
    {
      item.m_type = GuidesGallery::Item::Type::Outdoor;
      item.m_outdoorsParams.m_duration = guide.m_guideInfo.m_tourDuration;
      item.m_outdoorsParams.m_distance = guide.m_guideInfo.m_tracksLength;
      item.m_outdoorsParams.m_ascent = guide.m_guideInfo.m_ascent;
      item.m_outdoorsParams.m_tag = guide.m_guideInfo.m_tag;
    }

    gallery.m_items.emplace_back(std::move(item));
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
  m_bmManager = bmManager;
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

void GuidesManager::UpdateGuidesMarks()
{
  auto es = m_bmManager->GetEditSession();
  es.ClearGroup(UserMark::GUIDE_CLUSTER);
  es.ClearGroup(UserMark::GUIDE);
  for (auto & guide : m_guides.m_nodes)
  {
    if (guide.m_sightsCount + guide.m_outdoorCount > 1)
    {
      GuidesClusterMark * mark = es.CreateUserMark<GuidesClusterMark>(guide.m_point);
      mark->SetGuidesCount(guide.m_sightsCount, guide.m_outdoorCount);
      mark->SetIndex(++m_nextMarkIndex);
    }
    else
    {
      GuideMark * mark = es.CreateUserMark<GuideMark>(guide.m_point);
      mark->SetGuideType(guide.m_sightsCount > 0 ? GuideMark::Type::City
                                                 : GuideMark::Type::Outdoor);
      mark->SetGuideId(guide.m_guideInfo.m_id);
      mark->SetIsDownloaded(IsGuideDownloaded(guide.m_guideInfo.m_id));
      mark->SetIndex(++m_nextMarkIndex);
      m_shownGuides.insert(guide.m_guideInfo.m_id);
    }
  }
  UpdateActiveGuide();
}

void GuidesManager::OnClusterSelected(GuidesClusterMark const & mark, ScreenBase const & screen)
{
  m_drapeEngine.SafeCall(&df::DrapeEngine::Scale, 2.0, screen.GtoP(mark.GetPivot()),
                         true /* isAnim */);
  m_statistics.LogItemSelected(LayersStatistics::LayerItemType::Cluster);
}

void GuidesManager::OnGuideSelected()
{
  if (m_onGalleryChanged)
    m_onGalleryChanged(false /* reload */);

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
  if (m_state == GuidesState::HasData)
    m_statistics.LogActivate(LayersStatistics::Status::Success);
  else if (m_state == GuidesState::NoData)
    m_statistics.LogActivate(LayersStatistics::Status::Unavailable);
  else if (m_state == GuidesState::NetworkError || m_state == GuidesState::FatalNetworkError)
    m_statistics.LogActivate(LayersStatistics::Status::Error);
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
