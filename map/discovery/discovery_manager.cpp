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
  }

  UNREACHABLE();
}
}  // namespace

namespace discovery
{
Manager::Manager(DataSource const & dataSource, search::CityFinder & cityFinder, APIs const & apis)
  : m_dataSource(dataSource)
  , m_cityFinder(cityFinder)
  , m_searchApi(apis.m_search)
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
  ms::LatLon const ll(MercatorBounds::ToLatLon(point));
  std::ostringstream os;
  os << locals::Api::GetLocalsPageUrl() << "?lat=" << ll.lat << "&lon=" << ll.lon;
  return os.str();
}
}  // namespace discovery
