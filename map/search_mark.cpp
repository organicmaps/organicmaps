#include "map/search_mark.hpp"

#include <algorithm>

std::vector<m2::PointF> SearchMarkPoint::m_searchMarksSizes;

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

  auto const & symbols = GetAllSymbolsNames();
  auto const index = static_cast<size_t>(m_type);
  if (index >= static_cast<size_t>(SearchMarkType::Count))
  {
    ASSERT(false, ("Unknown search mark symbol."));
    return symbols[static_cast<size_t>(SearchMarkType::Default)];
  }
  return symbols[index];
}

UserMark::Type SearchMarkPoint::GetMarkType() const
{
  return UserMark::Type::SEARCH;
}

void SearchMarkPoint::SetFoundFeature(FeatureID const & feature)
{
  if (m_featureID == feature)
    return;

  SetDirty();
  m_featureID = feature;
}

void SearchMarkPoint::SetMatchedName(std::string const & name)
{
  if (m_matchedName == name)
    return;

  SetDirty();
  m_matchedName = name;
}

void SearchMarkPoint::SetMarkType(SearchMarkType type)
{
  if (m_type == type)
    return;

  SetDirty();
  m_type = type;
}

void SearchMarkPoint::SetPreparing(bool isPreparing)
{
  if (m_isPreparing == isPreparing)
    return;

  SetDirty();
  m_isPreparing = isPreparing;
}

// static
std::vector<std::string> const & SearchMarkPoint::GetAllSymbolsNames()
{
  static std::vector<std::string> const kSymbols =
  {
    "search-result",  // Default.
    "search-booking", // Booking.
    "search-adv",     // LocalAds.
    "search-cian",    // TODO: delete me after Cian project is finished.

    "non-found-search-result", // NotFound.
  };

  return kSymbols;
}

// static
void SearchMarkPoint::SetSearchMarksSizes(std::vector<m2::PointF> const & sizes)
{
  m_searchMarksSizes = sizes;
}

// static
double SearchMarkPoint::GetMaxSearchMarkDimension(ScreenBase const & modelView)
{
  double dimension = 0.0;
  for (size_t i = 0; i < static_cast<size_t>(SearchMarkType::Count); ++i)
  {
    m2::PointD const markSize = GetSearchMarkSize(static_cast<SearchMarkType>(i), modelView);
    dimension = std::max(dimension, std::max(markSize.x, markSize.y));
  }
  return dimension;
}

// static
m2::PointD SearchMarkPoint::GetSearchMarkSize(SearchMarkType searchMarkType,
                                              ScreenBase const & modelView)
{
  if (m_searchMarksSizes.empty())
    return {};

  auto const index = static_cast<size_t>(searchMarkType);
  ASSERT_LESS(index, m_searchMarksSizes.size(), ());
  m2::PointF const pixelSize = m_searchMarksSizes[index];

  double const pixelToMercator = modelView.GetScale();
  return {pixelToMercator * pixelSize.x, pixelToMercator * pixelSize.y};
}
