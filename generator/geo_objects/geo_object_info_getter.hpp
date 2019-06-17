#pragma once

#include "generator/key_value_storage.hpp"

#include "indexer/locality_index.hpp"

#include "coding/reader.hpp"

#include "geometry/point2d.hpp"

#include "base/geo_object_id.hpp"

#include <functional>
#include <memory>
#include <utility>
#include <vector>

#include <boost/optional.hpp>

#include "3party/jansson/myjansson.hpp"

namespace generator
{
namespace geo_objects
{
class GeoObjectInfoGetter
{
public:
  using IndexReader = ReaderPtr<Reader>;

  GeoObjectInfoGetter(indexer::GeoObjectsIndex<IndexReader> && index, KeyValueStorage const & kvStorage);

  template <typename Predicate>
  std::shared_ptr<JsonValue> Find(m2::PointD const & point, Predicate && pred) const;

private:
  std::vector<base::GeoObjectId> SearchObjectsInIndex(m2::PointD const & point) const;

  indexer::GeoObjectsIndex<IndexReader> m_index;
  KeyValueStorage const & m_storage;
};

template <typename Predicate>
std::shared_ptr<JsonValue> GeoObjectInfoGetter::Find(
    m2::PointD const & point, Predicate && pred) const
{
  auto const ids = SearchObjectsInIndex(point);
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
}  // namespace geo_objects
}  // namespace generator
