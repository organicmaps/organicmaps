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

std::shared_ptr<JsonValue> GeoObjectInfoGetter::Find(
    m2::PointD const & point, std::function<bool(JsonValue const &)> && pred) const
{
  auto const ids = SearchObjectsInIndex(m_index, point);
  for (auto const & id : ids)
  {
    auto const object = m_storage.Find(id.GetEncodedId());
    if (!object)
      continue;

    if (pred(std::cref(*object)))
      return object;
  }

  return {};
}

boost::optional<base::GeoObjectId> GeoObjectInfoGetter::Search(
    m2::PointD const & point, std::function<bool(JsonValue const &)> && pred) const
{
  auto const ids = SearchObjectsInIndex(m_index, point);
  for (auto const & id : ids)
  {
    auto const object = m_storage.Find(id.GetEncodedId());
    if (!object)
      continue;

    if (pred(std::cref(*object)))
      return id;
  }

  return {};
}

std::vector<base::GeoObjectId> GeoObjectInfoGetter::SearchObjectsInIndex(
    indexer::GeoObjectsIndex<IndexReader> const & index, m2::PointD const & point)
{
  std::vector<base::GeoObjectId> ids;
  auto const emplace = [&ids](base::GeoObjectId const & osmId) { ids.emplace_back(osmId); };
  index.ForEachAtPoint(emplace, point);
  return ids;
}
}  // namespace geo_objects
}  // namespace generator
