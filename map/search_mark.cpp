#include "map/search_mark.hpp"
#include "map/bookmark_manager.hpp"

#include "drape_frontend/drape_engine.hpp"

#include "platform/platform.hpp"

#include <algorithm>

namespace
{
std::vector<std::string> const kSymbols =
{
  "search-result",  // Default.
  "search-booking", // Booking.
  "search-adv",     // LocalAds.
  "search-cian",    // TODO: delete me after Cian project is finished.

  "non-found-search-result", // NotFound.
};
}  // namespace

SearchMarkPoint::SearchMarkPoint(m2::PointD const & ptOrg, UserMarkContainer * container)
  : UserMark(ptOrg, container)
{}

std::string SearchMarkPoint::GetSymbolName() const
{
  if (m_isPreparing)
  {
    //TODO: set symbol for preparing state.
    return "non-found-search-result";
  }

  if (m_type >= SearchMarkType::Count)
  {
    ASSERT(false, ("Unknown search mark symbol."));
    return kSymbols[static_cast<size_t>(SearchMarkType::Default)];
  }
  return kSymbols[static_cast<size_t>(m_type)];
}

UserMark::Type SearchMarkPoint::GetMarkType() const
{
  return UserMark::Type::SEARCH;
}

void SearchMarkPoint::SetFoundFeature(FeatureID const & feature)
{
  SetAttributeValue(m_featureID, feature);
}

void SearchMarkPoint::SetMatchedName(std::string const & name)
{
  SetAttributeValue(m_matchedName, name);
}

void SearchMarkPoint::SetMarkType(SearchMarkType type)
{
  SetAttributeValue(m_type, type);
}

void SearchMarkPoint::SetPreparing(bool isPreparing)
{
  SetAttributeValue(m_isPreparing, isPreparing);
}

SearchMarks::SearchMarks()
  : m_bmManager(nullptr)
{}

void SearchMarks::SetDrapeEngine(ref_ptr<df::DrapeEngine> engine)
{
  m_drapeEngine.Set(engine);
  if (engine == nullptr)
    return;

  m_drapeEngine.SafeCall(&df::DrapeEngine::RequestSymbolsSize, kSymbols,
                         [this](std::vector<m2::PointF> const & sizes)
  {
    GetPlatform().RunTask(Platform::Thread::Gui, [this, sizes](){ m_searchMarksSizes = sizes; });
  });
}

void SearchMarks::SetBookmarkManager(BookmarkManager * bmManager)
{
  m_bmManager = bmManager;
}

double SearchMarks::GetMaxDimension(ScreenBase const & modelView) const
{
  double dimension = 0.0;
  for (size_t i = 0; i < static_cast<size_t>(SearchMarkType::Count); ++i)
  {
    m2::PointD const markSize = GetSize(static_cast<SearchMarkType>(i), modelView);
    dimension = std::max(dimension, std::max(markSize.x, markSize.y));
  }
  return dimension;
}

m2::PointD SearchMarks::GetSize(SearchMarkType searchMarkType, ScreenBase const & modelView) const
{
  if (m_searchMarksSizes.empty())
    return {};

  auto const index = static_cast<size_t>(searchMarkType);
  ASSERT_LESS(index, m_searchMarksSizes.size(), ());
  m2::PointF const pixelSize = m_searchMarksSizes[index];

  double const pixelToMercator = modelView.GetScale();
  return {pixelToMercator * pixelSize.x, pixelToMercator * pixelSize.y};
}

void SearchMarks::SetPreparingState(std::vector<FeatureID> const & features, bool isPreparing)
{
  if (m_bmManager == nullptr)
    return;

  ASSERT(std::is_sorted(features.begin(), features.end()), ());

  UserMarkNotificationGuard guard(*m_bmManager, UserMark::Type::SEARCH);
  size_t const count = guard.m_controller.GetUserMarkCount();
  for (size_t i = 0; i < count; ++i)
  {
    auto mark = static_cast<SearchMarkPoint *>(guard.m_controller.GetUserMarkForEdit(i));
    if (std::binary_search(features.begin(), features.end(), mark->GetFeatureID()))
      mark->SetPreparing(isPreparing);
  }
}
