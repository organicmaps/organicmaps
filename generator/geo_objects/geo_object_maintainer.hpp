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
class GeoObjectMaintainer
{
public:
  using IndexReader = ReaderPtr<Reader>;

  GeoObjectMaintainer(indexer::GeoObjectsIndex<IndexReader> && index,
                      KeyValueStorage const & kvStorage);

  std::shared_ptr<JsonValue> FindFirstMatchedObject(
      m2::PointD const & point, std::function<bool(JsonValue const &)> && pred) const;

  boost::optional<base::GeoObjectId> SearchIdOfFirstMatchedObject(
      m2::PointD const & point, std::function<bool(JsonValue const &)> && pred) const;

  std::vector<base::GeoObjectId> SearchObjectsInIndex(m2::PointD const & point) const
  {
    return SearchGeoObjectIdsByPoint(m_index, point);
  }

  static std::vector<base::GeoObjectId> SearchGeoObjectIdsByPoint(
      indexer::GeoObjectsIndex<IndexReader> const & index, m2::PointD const & point);


private:
  indexer::GeoObjectsIndex<IndexReader> m_index;
  KeyValueStorage const & m_storage;
};
}  // namespace geo_objects
}  // namespace generator
