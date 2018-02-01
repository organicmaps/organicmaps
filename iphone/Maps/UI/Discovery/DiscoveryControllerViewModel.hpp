#pragma once

#include "map/discovery/discovery_client_params.hpp"

#include "partners_api/locals_api.hpp"
#include "partners_api/viator_api.hpp"

#include "search/result.hpp"

#include "geometry/point2d.hpp"

#include "base/assert.hpp"

#include <vector>

namespace discovery
{
class DiscoveryControllerViewModel
{
public:
  void SetSearchResults(search::Results const & res, m2::PointD const & viewportCenter,
                        ItemType const type)
  {
    switch (type)
    {
    case ItemType::Attractions:
      m_attractions.m_viewportCenter = viewportCenter;
      m_attractions.m_results = res;
      break;
    case ItemType::Cafes:
      m_cafes.m_viewportCenter = viewportCenter;
      m_cafes.m_results = res;
      break;
    case ItemType::Hotels:
      m_hotels.m_viewportCenter = viewportCenter;
      m_hotels.m_results = res;
      break;
    default: break;
    }
  }

  void SetViator(std::vector<viator::Product> const & viator) { m_viator = viator; }
  void SetExperts(std::vector<locals::LocalExpert> const & experts) { m_experts = experts; }

  size_t GetItemsCount(ItemType const type) const
  {
    switch (type)
    {
    case ItemType::Viator: return m_viator.size();
    case ItemType::Attractions: return m_attractions.m_results.GetCount();
    case ItemType::Cafes: return m_cafes.m_results.GetCount();
    case ItemType::Hotels: return m_hotels.m_results.GetCount();
    case ItemType::LocalExperts: return m_experts.size();
    }
  }

  viator::Product const & GetViatorAt(size_t const index) const
  {
    CHECK_LESS(index, m_viator.size(), ("Incorrect viator index:", index));
    return m_viator[index];
  }

  search::Result const & GetAttractionAt(size_t const index) const
  {
    CHECK_LESS(index, m_attractions.m_results.GetCount(), ("Incorrect attractions index:", index));
    return m_attractions.m_results[index];
  }

  m2::PointD const & GetAttractionReferencePoint() const { return m_attractions.m_viewportCenter; }

  search::Result const & GetCafeAt(size_t const index) const
  {
    CHECK_LESS(index, m_cafes.m_results.GetCount(), ("Incorrect cafes index:", index));
    return m_cafes.m_results[index];
  }

  m2::PointD const & GetCafeReferencePoint() const { return m_cafes.m_viewportCenter; }

  locals::LocalExpert const & GetExpertAt(size_t const index) const
  {
    CHECK_LESS(index, m_experts.size(), ("Incorrect experts index:", index));
    return m_experts[index];
  }

  search::Result const & GetHotelAt(size_t const index) const
  {
    CHECK_LESS(index, m_hotels.m_results.GetCount(), ("Incorrect hotels index:", index));
    return m_hotels.m_results[index];
  }

  m2::PointD const & GetHotelReferencePoint() const { return m_hotels.m_viewportCenter; }

private:
  struct UISearchResults
  {
    m2::PointD m_viewportCenter;
    search::Results m_results;
  };

  UISearchResults m_attractions;
  UISearchResults m_cafes;
  UISearchResults m_hotels;
  std::vector<viator::Product> m_viator;
  std::vector<locals::LocalExpert> m_experts;
};
}  // namespace discovery
