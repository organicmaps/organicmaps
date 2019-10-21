#include "map/discovery/discovery_manager.hpp"

#include "editor/editable_data_source.hpp"

#include "indexer/data_source.hpp"

#include <sstream>

namespace
{
std::string GetQuery(discovery::ItemType const type)
{
  switch (type)
  {
  case discovery::ItemType::Hotels: return "hotel ";
  case discovery::ItemType::Attractions: return "attractions ";
  case discovery::ItemType::Cafes: return "cafe ";
  case discovery::ItemType::LocalExperts: return "";
  case discovery::ItemType::Promo: return "";
  }

  UNREACHABLE();
}
}  // namespace

namespace discovery
{
Manager::Manager(DataSource const & dataSource, APIs const & apis)
  : m_dataSource(dataSource)
  , m_searchApi(apis.m_search)
  , m_promoApi(apis.m_promo)
  , m_localsApi(apis.m_locals)
{
}

// static
DiscoverySearchParams Manager::GetSearchParams(Manager::Params const & params, ItemType const type)
{
  DiscoverySearchParams p;
  p.m_query = GetQuery(type);
  p.m_viewport = params.m_viewport;
  p.m_itemsCount = params.m_itemsCount;

  return p;
}

std::string Manager::GetLocalExpertsUrl(m2::PointD const & point) const
{
  ms::LatLon const ll(mercator::ToLatLon(point));
  std::ostringstream os;
  os << locals::Api::GetLocalsPageUrl() << "?lat=" << ll.m_lat << "&lon=" << ll.m_lon;
  return os.str();
}
}  // namespace discovery
