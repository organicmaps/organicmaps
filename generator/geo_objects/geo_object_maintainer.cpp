#include "generator/geo_objects/geo_object_maintainer.hpp"

#include <utility>

namespace generator
{
namespace geo_objects
{
GeoObjectMaintainer::GeoObjectMaintainer(indexer::GeoObjectsIndex<IndexReader> && index,
                                         KeyValueStorage const & kvStorage)
    : m_index{std::move(index)}, m_storage{kvStorage}
{ }

std::shared_ptr<JsonValue> GeoObjectMaintainer::FindFirstMatchedObject(
    m2::PointD const & point, std::function<bool(JsonValue const &)> && pred) const
{
  auto const ids = SearchGeoObjectIdsByPoint(m_index, point);
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

boost::optional<base::GeoObjectId> GeoObjectMaintainer::SearchIdOfFirstMatchedObject(
    m2::PointD const & point, std::function<bool(JsonValue const &)> && pred) const
{
  auto const ids = SearchGeoObjectIdsByPoint(m_index, point);
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

std::vector<base::GeoObjectId> GeoObjectMaintainer::SearchGeoObjectIdsByPoint(
    indexer::GeoObjectsIndex<IndexReader> const & index, m2::PointD const & point)
{
  std::vector<base::GeoObjectId> ids;
  auto const emplace = [&ids](base::GeoObjectId const & osmId) { ids.emplace_back(osmId); };
  index.ForEachAtPoint(emplace, point);
  return ids;
}
}  // namespace geo_objects
}  // namespace generator
