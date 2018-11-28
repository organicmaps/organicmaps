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
  case discovery::ItemType::LocalExperts:
  case discovery::ItemType::Viator: ASSERT(false, ()); return "";
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
  , m_viatorApi(apis.m_viator)
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

std::string Manager::GetViatorUrl(m2::PointD const & point) const
{
  auto const viatorId = GetCityViatorId(point);
  if (viatorId.empty())
    return {};
  return viator::Api::GetCityUrl(viatorId);
}

std::string Manager::GetLocalExpertsUrl(m2::PointD const & point) const
{
  ms::LatLon const ll(MercatorBounds::ToLatLon(point));
  std::ostringstream os;
  os << locals::Api::GetLocalsPageUrl() << "?lat=" << ll.lat << "&lon=" << ll.lon;
  return os.str();
}

std::string Manager::GetCityViatorId(m2::PointD const & point) const
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());
  auto const fid = m_cityFinder.GetCityFeatureID(point);
  if (!fid.IsValid())
    return {};

  FeaturesLoaderGuard const guard(m_dataSource, fid.m_mwmId);
  FeatureType ft;
  if (!guard.GetFeatureByIndex(fid.m_index, ft))
  {
    LOG(LERROR, ("Feature can't be loaded:", fid));
    return {};
  }

  auto const & metadata = ft.GetMetadata();
  auto const sponsoredId = metadata.Get(feature::Metadata::FMD_SPONSORED_ID);
  return sponsoredId;
}
}  // namespace discovery
