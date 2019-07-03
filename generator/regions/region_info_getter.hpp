#pragma once

#include "generator/key_value_storage.hpp"

#include "indexer/borders.hpp"
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
namespace regions
{
// ConcurrentGetProcessability is marker inteface: concurrent mode capability for any get operations.
struct ConcurrentGetProcessability
{ };

class RegionInfoGetter : public ConcurrentGetProcessability
{
public:
  using Selector = std::function<bool(KeyValue const & json)>;

  RegionInfoGetter(std::string const & indexPath, std::string const & kvPath);

  boost::optional<KeyValue> FindDeepest(m2::PointD const & point) const;
  boost::optional<KeyValue> FindDeepest(m2::PointD const & point, Selector const & selector) const;
  KeyValueStorage const & GetStorage() const noexcept;

private:
  using IndexReader = ReaderPtr<Reader>;

  std::vector<base::GeoObjectId> SearchObjectsInIndex(m2::PointD const & point) const;
  boost::optional<KeyValue> GetDeepest(m2::PointD const & point, std::vector<base::GeoObjectId> const & ids,
                                       Selector const & selector) const;
  int GetRank(JsonValue const & json) const;
  // Get parent id of object: optional field `properties.dref` in JSON.
  boost::optional<uint64_t> GetDref(JsonValue const & json) const;

  indexer::RegionsIndex<IndexReader> m_index;
  indexer::Borders m_borders;
  KeyValueStorage m_storage;
};
}  // namespace regions
}  // namespace generator
