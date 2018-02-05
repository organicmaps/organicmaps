#pragma once

#include "map/discovery/discovery_client_params.hpp"

#include "search/result.hpp"

#include "geometry/point2d.hpp"
#include "geometry/rect2d.hpp"

#include <cstddef>
#include <functional>
#include <string>

namespace search
{
struct DiscoverySearchParams
{
  enum class SortingType
  {
    None,
    HotelRating
  };

  struct HotelRatingComparator
  {
    bool operator()(Result const & lhs, Result const & rhs) const
    {
      return lhs.m_metadata.m_hotelRating > rhs.m_metadata.m_hotelRating;
    }
  };

  using OnResults = std::function<void(Results const & results)>;

  std::string m_query;
  size_t m_itemsCount = 0;
  m2::PointD m_position;
  m2::RectD m_viewport;
  SortingType m_sortingType = SortingType::None;
  OnResults m_onResults = nullptr;
};
}  // namespace search
