#pragma once

#include "generator/geo_objects/key_value_storage.hpp"

#include "indexer/locality_index.hpp"

#include "coding/reader.hpp"

#include "geometry/point2d.hpp"

#include "base/geo_object_id.hpp"

#include <string>
#include <vector>

#include <boost/optional.hpp>

#include "3party/jansson/myjansson.hpp"

namespace generator
{
namespace geo_objects
{
class RegionInfoGetter
{
public:
  RegionInfoGetter(std::string const & indexPath, std::string const & kvPath);

  boost::optional<KeyValue> FindDeepest(m2::PointD const & point) const;
  KeyValueStorage const & GetStorage() const noexcept;

private:
  using IndexReader = ReaderPtr<Reader>;

  std::vector<base::GeoObjectId> SearchObjectsInIndex(m2::PointD const & point) const;
  boost::optional<KeyValue> GetDeepest(std::vector<base::GeoObjectId> const & ids) const;
  int GetRank(base::Json const & json) const;

  indexer::RegionsIndex<IndexReader> m_index;
  KeyValueStorage m_storage;
};
}  // namespace geo_objects
}  // namespace generator
