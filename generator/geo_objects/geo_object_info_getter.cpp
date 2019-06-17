#include "generator/geo_objects/geo_object_info_getter.hpp"

#include <utility>

namespace generator
{
namespace geo_objects
{
GeoObjectInfoGetter::GeoObjectInfoGetter(indexer::GeoObjectsIndex<IndexReader> && index,
                                         KeyValueStorage const & kvStorage)
    : m_index{std::move(index)}, m_storage{kvStorage}
{ }

std::vector<base::GeoObjectId> GeoObjectInfoGetter::SearchObjectsInIndex(m2::PointD const & point) const
{
  std::vector<base::GeoObjectId> ids;
  auto const emplace = [&ids] (base::GeoObjectId const & osmId) { ids.emplace_back(osmId); };
  m_index.ForEachAtPoint(emplace, point);
  return ids;
}
}  // namespace geo_objects
}  // namespace generator
