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
  "search-result",              // Default.
  "coloredmark-default-l",      // Booking.
  "search-adv",                 // Local Ads.
  "searchbookingadv-default-l", // Local Ads + Booking.
  "coloredmark-default-l",      // UGC.
  "search-football",            // FC 2018.

  "non-found-search-result",    // NotFound.
};

std::vector<std::string> const kPreparingSymbols =
{
  "search-result",              // Default.
  "coloredmark-inactive",       // Booking.
  "search-adv",                 // Local Ads.
  "searchbookingadv-default-l", // Local Ads + Booking.
  "coloredmark-inactive",       // UGC.
  "search-football",            // FC 2018.

  "non-found-search-result",    // NotFound.
};

std::string const kSaleBadgeName = "searchbooking-sale-1";

float const kRatingThreshold = 6.0f;
float const kMetricThreshold = 0.38f;

inline bool HasNoRating(float rating)
{
  return fabs(rating) < 1e-5;
}

float CalculateAggregateMetric(float rating, int pricing)
{
  float const p1 = my::clamp(rating, 0.0f, 10.0f) / 10.0f;
  float const p2 = (3 - my::clamp(pricing, 0, 3)) / 3.0f;
  float const s = p1 + p2;
  if (fabs(s) < 1e-5)
    return 0.0f;
  return 2.0f * p1 * p2 / s;
}

std::string GetBookingBadgeName(int pricing)
{
  if (pricing == 0)
    return {};

  return std::string("searchbooking-ext-") + strings::to_string(pricing);
}

bool NeedShowBookingBadge(float rating, int pricing)
{
  if (pricing == 0)
    return false;
  auto const metric = CalculateAggregateMetric(rating, pricing);
  return metric >= kMetricThreshold;
}

std::string GetBookingSmallIcon(SearchMarkType type)
{
  if (type == SearchMarkType::Booking)
    return "coloredmark-default-s";
  if (type == SearchMarkType::LocalAdsBooking)
    return "search-adv";
  return {};
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
  }

  auto symbol = make_unique_dp<SymbolNameZoomInfo>();
  if (IsMarkWithRating())
  {
    symbol->insert(std::make_pair(1 /* zoomLevel */, m_rating < kRatingThreshold ?
                                                     GetBookingSmallIcon(m_type) : name));
    symbol->insert(std::make_pair(17 /* zoomLevel */, name));
  }
  else
  {
    symbol->insert(std::make_pair(1 /* zoomLevel */, name));
  }
  return symbol;
}

drape_ptr<df::UserPointMark::SymbolNameZoomInfo> SearchMarkPoint::GetBadgeNames() const
{
  if (!IsBookingSpecialMark())
    return nullptr;

  auto const badgeName = m_hasSale ? kSaleBadgeName : GetBookingBadgeName(m_pricing);
  if (badgeName.empty() || !SearchMarks::GetSize(badgeName))
    return nullptr;

  auto symbol = make_unique_dp<SymbolNameZoomInfo>();
  if (NeedShowBookingBadge(m_rating, m_pricing) && m_rating >= kRatingThreshold)
    symbol->insert(std::make_pair(10 /* zoomLevel */, badgeName));
  else
    symbol->insert(std::make_pair(17 /* zoomLevel */, badgeName));
  return symbol;
}

drape_ptr<df::UserPointMark::SymbolOffsets> SearchMarkPoint::GetSymbolOffsets() const
{
  if (!IsBookingSpecialMark())
    return nullptr;

  auto const badgeName = m_hasSale ? kSaleBadgeName : GetBookingBadgeName(m_pricing);
  if (badgeName.empty() || !SearchMarks::GetSize(badgeName))
    return nullptr;

  auto const name = kSymbols[static_cast<size_t>(SearchMarkType::Booking)];
  auto const iconSz = SearchMarks::GetSize(name).get_value_or({});

  SymbolOffsets offsets(scales::UPPER_STYLE_SCALE);
  for (size_t i = 0; i < offsets.size(); i++)
  {
    auto const badgeSz = SearchMarks::GetSize(badgeName).get_value_or({});
    offsets[i] = {0.5f * static_cast<float>(badgeSz.x - iconSz.x), 0.0};
  }

  return make_unique_dp<SymbolOffsets>(offsets);
}

df::ColorConstant SearchMarkPoint::GetColorConstant() const
{
  if (!IsMarkWithRating())
    return {};

  if (m_type == SearchMarkType::LocalAdsBooking)
    return "RatingLocalAds";

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
  if (!IsMarkWithRating() || fabs(m_rating) < 1e-5)
    return nullptr;

  auto titles = make_unique_dp<TitlesInfo>();
  titles->push_back(m_titleDecl);
  return titles;
}

int SearchMarkPoint::GetMinTitleZoom() const
{
  if (IsMarkWithRating() && m_rating < kRatingThreshold)
    return 17;
  return 1;
}

df::DepthLayer SearchMarkPoint::GetDepthLayer() const
{
  return df::DepthLayer::SearchMarkLayer;
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

void SearchMarkPoint::SetSale(bool hasSale)
{
  SetAttributeValue(m_hasSale, hasSale);
}

bool SearchMarkPoint::IsBookingSpecialMark() const
{
  return (m_type == SearchMarkType::Booking || m_type == SearchMarkType::LocalAdsBooking) &&
         !m_isPreparing;
}

bool SearchMarkPoint::IsUGCMark() const
{
  return m_type == SearchMarkType::UGC;
}

bool SearchMarkPoint::IsMarkWithRating() const
{
  return IsBookingSpecialMark() || IsUGCMark();
}

// static
std::map<std::string, m2::PointF> SearchMarks::m_searchMarksSizes;

SearchMarks::SearchMarks()
  : m_bmManager(nullptr)
{}

void SearchMarks::SetDrapeEngine(ref_ptr<df::DrapeEngine> engine)
{
  m_drapeEngine.Set(engine);
  if (engine == nullptr)
    return;

  std::vector<std::string> symbols;
  symbols.insert(symbols.end(), kSymbols.begin(), kSymbols.end());
  for (int pricing = 1; pricing <= 3; pricing++)
    symbols.push_back(GetBookingBadgeName(pricing));
  symbols.push_back(kSaleBadgeName);

  m_drapeEngine.SafeCall(&df::DrapeEngine::RequestSymbolsSize, symbols,
                         [](std::map<std::string, m2::PointF> && sizes)
  {
    GetPlatform().RunTask(Platform::Thread::Gui, [sizes = std::move(sizes)]() mutable
    {
      m_searchMarksSizes = std::move(sizes);
    });
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

// static
m2::PointD SearchMarks::GetSize(SearchMarkType searchMarkType, ScreenBase const & modelView)
{
  if (m_searchMarksSizes.empty())
    return {};

  auto const index = static_cast<size_t>(searchMarkType);
  ASSERT_LESS(index, kSymbols.size(), ());
  auto const pixelSize = GetSize(kSymbols[index]).get_value_or({});
  double const pixelToMercator = modelView.GetScale();
  return {pixelToMercator * pixelSize.x, pixelToMercator * pixelSize.y};
}

// static
boost::optional<m2::PointD> SearchMarks::GetSize(std::string const & symbolName)
{
  auto const it = m_searchMarksSizes.find(symbolName);
  if (it == m_searchMarksSizes.end())
    return {};
  return m2::PointD(it->second);
}

void SearchMarks::SetPreparingState(std::vector<FeatureID> const & features, bool isPreparing)
{
  FilterAndProcessMarks(features, [isPreparing](SearchMarkPoint * mark)
  {
    mark->SetPreparing(isPreparing);
  });
}

void SearchMarks::SetSales(std::vector<FeatureID> const & features, bool hasSale)
{
  FilterAndProcessMarks(features, [hasSale](SearchMarkPoint * mark)
  {
    mark->SetSale(hasSale);
  });
}

void SearchMarks::FilterAndProcessMarks(std::vector<FeatureID> const & features,
                                        std::function<void(SearchMarkPoint *)> && processor)
{
  if (m_bmManager == nullptr || processor == nullptr)
    return;

  ASSERT(std::is_sorted(features.begin(), features.end()), ());

  auto editSession = m_bmManager->GetEditSession();
  for (auto markId : m_bmManager->GetUserMarkIds(UserMark::Type::SEARCH))
  {
    auto * mark = editSession.GetMarkForEdit<SearchMarkPoint>(markId);
    if (std::binary_search(features.begin(), features.end(), mark->GetFeatureID()))
      processor(mark);
  }
}
