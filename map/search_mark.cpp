#include "map/search_mark.hpp"
#include "map/bookmark_manager.hpp"
#include "map/place_page_info.hpp"

#include "drape_frontend/drape_engine.hpp"
#include "drape_frontend/visual_params.hpp"

#include "platform/platform.hpp"

#include <algorithm>

namespace
{
std::vector<std::string> const kSymbols =
{
  "search-result",           // Default.
  "searchbooking-default-l", // Booking.
  "search-adv",              // LocalAds.

  "non-found-search-result", // NotFound.
};

std::vector<std::string> const kPreparingSymbols =
{
  "search-result",           // Default.
  "searchbooking-inactive",  // Booking.
  "search-adv",              // LocalAds.

  "non-found-search-result", // NotFound.
};

std::string const kBookingSmallIcon = "searchbooking-default-s";
float const kRatingThreshold = 6.0f;

inline bool HasNoRating(float rating)
{
  return fabs(rating) < 1e-5;
}
}  // namespace

SearchMarkPoint::SearchMarkPoint(m2::PointD const & ptOrg)
  : UserMark(ptOrg, UserMark::Type::SEARCH)
{
  m_titleDecl.m_anchor = dp::Center;
  m_titleDecl.m_primaryTextFont.m_color = df::GetColorConstant("RatingText");
  m_titleDecl.m_primaryTextFont.m_size =
      static_cast<float>(12.0 / df::VisualParams::Instance().GetFontScale());
}

drape_ptr<df::UserPointMark::SymbolNameZoomInfo> SearchMarkPoint::GetSymbolNames() const
{
  std::string name;
  if (m_type >= SearchMarkType::Count)
  {
    ASSERT(false, ("Unknown search mark symbol."));
    name = kSymbols[static_cast<size_t>(SearchMarkType::Default)];
  }
  else if (m_isPreparing)
  {
    name = kPreparingSymbols[static_cast<size_t>(m_type)];
  }
  else
  {
    name = kSymbols[static_cast<size_t>(m_type)];
    if (m_type == SearchMarkType::Booking && m_rating < kRatingThreshold)
      name = kBookingSmallIcon;
  }
  auto symbol = make_unique_dp<SymbolNameZoomInfo>();
  symbol->insert(std::make_pair(1 /* zoomLevel */, name));
  return symbol;
}

df::ColorConstant SearchMarkPoint::GetColorConstant() const
{
  if (m_type != SearchMarkType::Booking || m_isPreparing)
    return {};

  if (HasNoRating(m_rating))
    return "RatingNone";
  if (m_rating < 2.0f)
    return "RatingHorrible";
  if (m_rating < 4.0f)
    return "RatingBad";
  if (m_rating < 6.0f)
    return "RatingNormal";
  if (m_rating < 8.0f)
    return "RatingGood";
  return "RatingExcellent";
}

drape_ptr<df::UserPointMark::TitlesInfo> SearchMarkPoint::GetTitleDecl() const
{
  if (m_type != SearchMarkType::Booking || m_isPreparing || m_rating < kRatingThreshold)
    return nullptr;

  auto titles = make_unique_dp<TitlesInfo>();
  titles->push_back(m_titleDecl);
  return titles;
}

df::RenderState::DepthLayer SearchMarkPoint::GetDepthLayer() const
{
  return df::RenderState::SearchMarkLayer;
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

void SearchMarkPoint::SetRating(float rating)
{
  SetAttributeValue(m_rating, rating);
  m_titleDecl.m_primaryText = place_page::rating::GetRatingFormatted(rating);
}

void SearchMarkPoint::SetPricing(int pricing)
{
  SetAttributeValue(m_pricing, pricing);
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

  auto editSession = m_bmManager->GetEditSession();
  for (auto markId : m_bmManager->GetUserMarkIds(UserMark::Type::SEARCH))
  {
    auto * mark = editSession.GetMarkForEdit<SearchMarkPoint>(markId);
    if (std::binary_search(features.begin(), features.end(), mark->GetFeatureID()))
      mark->SetPreparing(isPreparing);
  }
}
