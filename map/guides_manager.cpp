#include "map/guides_manager.hpp"

#include "map/bookmark_catalog.hpp"

#include "partners_api/utm.hpp"

#include "drape_frontend/drape_engine.hpp"
#include "drape_frontend/visual_params.hpp"

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
}  // namespace

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
  auto const zoom = df::GetDrawTileScale(screen);

  if (m_state == GuidesState::Disabled || m_state == GuidesState::FatalNetworkError)
  {
    m_screen = screen;
    m_zoom = zoom;
    return;
  }

  if (screen.GlobalRect().GetLocalRect().IsEmptyInterior())
    return;

  if (!m_screen.GlobalRect().GetLocalRect().IsEmptyInterior())
  {
    auto const requestedCenter = screen.PtoP3d(screen.GtoP(m_screen.GetOrg()));
    auto const viewportCenter = screen.PtoP3d(screen.GtoP(screen.GetOrg()));
    auto const orgDist = requestedCenter.Length(viewportCenter);
    auto const viewportLength = std::min(screen.PixelRectIn3d().SizeX(),
                                         screen.PixelRectIn3d().SizeY());
    // Return when center moves more than 20 percents of smaller side
    if (orgDist < viewportLength * 0.2)
      return;

    auto const scaleStronglyChanged =
      fabs(m_screen.GetScale() - screen.GetScale()) / m_screen.GetScale() > kScaleEps;

    if (!scaleStronglyChanged)
    {
      m2::AnyRectD::Corners currentCorners;
      m_screen.GlobalRect().GetGlobalPoints(currentCorners);

      m2::AnyRectD::Corners screenCorners;
      screen.GlobalRect().GetGlobalPoints(screenCorners);

      auto const score = geometry::GetIntersectionScoreForPoints(currentCorners, screenCorners);

      // If more than 80% of viewport rect intersects with last requested rect then return.
      if (score > 0.8)
        return;
    }
  }

  m_screen = screen;
  m_zoom = zoom;

  RequestGuides(m_screen.GlobalRect(), m_zoom);
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
  RequestGuides(m_screen.GlobalRect(), m_zoom);
}

void GuidesManager::SetEnabled(bool enabled)
{
  auto const newState = enabled ? GuidesState::Enabled : GuidesState::Disabled;
  if (newState == m_state)
    return;

  Clear();
  ChangeState(newState);
  m_shownGuides.clear();

  if (!enabled)
    return;

  RequestGuides(m_screen.GlobalRect(), m_zoom);
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
}

void GuidesManager::RequestGuides(m2::AnyRectD const & rect, int zoom)
{
  if (rect.GetLocalRect().IsEmptyInterior())
    return;

  auto const requestNumber = ++m_requestCounter;
  m_api.GetGuidesOnMap(
      rect, zoom,
      [this](guides_on_map::GuidesOnMap const & guides) {
        if (m_state == GuidesState::Disabled)
          return;

        m_guides = guides;
        m_errorRequestsCount = 0;

        if (!m_guides.empty())
          ChangeState(GuidesState::HasData);
        else
          ChangeState(GuidesState::NoData);

        UpdateGuidesMarks();

        if (m_onGalleryChanged)
          m_onGalleryChanged(true /* reload */);
      },
      [this, requestNumber, zoom]() mutable {
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
          RequestGuides(m_screen.GlobalRect(), zoom);
      });
}

void GuidesManager::Clear()
{
  m_activeGuide.clear();
  m_guides.clear();
  m_errorRequestsCount = 0;

  UpdateGuidesMarks();
}

GuidesManager::GuidesGallery GuidesManager::GetGallery() const
{
  // Dummy gallery hardcode for debug only.
  GuidesGallery gallery;
  {
    GuidesGallery::Item item;
    item.m_guideId = "048f4c49-ee80-463f-8513-e57ade2303ee";
    item.m_url = "https://routes.maps.me/en/v3/mobilefront/route/048f4c49-ee80-463f-8513-e57ade2303ee";
    item.m_imageUrl = "https://storage.maps.me/bookmarks_catalogue/"
                      "002dc2ae-7b5c-4d3c-88bc-7c7ba109d0e8.jpg?t=1584470956.009026";
    item.m_title = "Moscow by The Village";
    item.m_type = GuidesGallery::Item::Type::City;
    item.m_downloaded = false;
    item.m_cityParams.m_bookmarksCount = 32;
    item.m_cityParams.m_trackIsAvailable = false;

    gallery.m_items.emplace_back(std::move(item));
  }

  {
    GuidesGallery::Item item;
    item.m_guideId = "e2d448eb-7fa4-4fab-93e7-ef0fea91cfff";
    item.m_url = "https://routes.maps.me/en/v3/mobilefront/route/e2d448eb-7fa4-4fab-93e7-ef0fea91cfff";
    item.m_imageUrl = "https://storage.maps.me/bookmarks_catalogue/"
                      "002dc2ae-7b5c-4d3c-88bc-7c7ba109d0e8.jpg?t=1584470956.009026";
    item.m_title = "Riga City Tour";
    item.m_type = GuidesGallery::Item::Type::City;
    item.m_downloaded = true;
    item.m_cityParams.m_bookmarksCount = 31;
    item.m_cityParams.m_trackIsAvailable = true;

    gallery.m_items.emplace_back(std::move(item));
  }

  {
    GuidesGallery::Item item;
    item.m_guideId = "d26a6662-20a3-432c-a357-c9cb3cce6d57";
    item.m_url = "https://routes.maps.me/en/v3/mobilefront/route/d26a6662-20a3-432c-a357-c9cb3cce6d57";
    item.m_imageUrl = "https://img.oastatic.com/img2/1966324/834x417s/t.jpg";
    item.m_title = "Klassik trifft Romantik";
    item.m_type = GuidesGallery::Item::Type::Outdoor;
    item.m_downloaded = false;
    item.m_outdoorsParams.m_tag = "Hiking / Trekking";
    item.m_outdoorsParams.m_ascent = 400;
    item.m_outdoorsParams.m_distance = 24100;
    item.m_outdoorsParams.m_duration = 749246;

    gallery.m_items.emplace_back(std::move(item));
  }

  for (auto const & guide : m_guides)
  {
    if (guide.m_outdoorCount + guide.m_sightsCount != 1)
      continue;

    auto const & info = guide.m_guideInfo;

    GuidesGallery::Item item;
    item.m_guideId = info.m_id;

    auto url = url::Join(BOOKMARKS_CATALOG_FRONT_URL, languages::GetCurrentNorm(), "v3/mobilefront",
                         info.m_id);
    InjectUTM(url, UTM::GuidesOnMapGallery);
    InjectUTMTerm(url, std::to_string(m_shownGuides.size()));

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

  // Dummy, for debug only.
  while (gallery.m_items.size() < 10)
  {
    std::copy(gallery.m_items.begin(), gallery.m_items.end(), std::back_inserter(gallery.m_items));
  }

  return gallery;
}

std::string GuidesManager::GetActiveGuide() const { return m_activeGuide; }

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
  for (auto & guide : m_guides)
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
}

void GuidesManager::OnGuideSelected(GuideMark const & mark)
{
  auto es = m_bmManager->GetEditSession();
  es.ClearGroup(UserMark::Type::GUIDE_SELECTION);
  es.CreateUserMark<GuideSelectionMark>(mark.GetPivot());

  m_activeGuide = mark.GetGuideId();
  if (m_onGalleryChanged)
    m_onGalleryChanged(false /* reload */);
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
