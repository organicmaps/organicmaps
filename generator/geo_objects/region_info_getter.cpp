#include "generator/geo_objects/region_info_getter.hpp"

#include "coding/mmap_reader.hpp"

#include "base/logging.hpp"

namespace generator
{
namespace geo_objects
{
RegionInfoGetter::RegionInfoGetter(std::string const & indexPath, std::string const & kvPath)
    : m_index{indexer::ReadIndex<indexer::RegionsIndexBox<IndexReader>, MmapReader>(indexPath)}
    , m_storage(kvPath)
{ }

boost::optional<KeyValue> RegionInfoGetter::FindDeepest(m2::PointD const & point) const
{
  auto const ids = SearchObjectsInIndex(point);
  return GetDeepest(ids);
}

std::vector<base::GeoObjectId> RegionInfoGetter::SearchObjectsInIndex(m2::PointD const & point) const
{
  std::vector<base::GeoObjectId> ids;
  auto const emplace = [&ids] (base::GeoObjectId const & osmId) { ids.emplace_back(osmId); };
  m_index.ForEachAtPoint(emplace, point);
  return ids;
}

boost::optional<KeyValue> RegionInfoGetter::GetDeepest(std::vector<base::GeoObjectId> const & ids) const
{
  boost::optional<KeyValue> deepest;
  int deepestRank = 0;
  for (auto const & id : ids)
  {
    base::Json temp;
    auto const res = m_storage.Find(id.GetEncodedId());
    if (!res)
    {
      LOG(LWARNING, ("Id not found in region key-value storage:", id));
      continue;
    }

    temp = *res;
    if (!json_is_object(temp.get()))
    {
      LOG(LWARNING, ("Value is not a json object in region key-value storage:", id));
      continue;
    }

    int tempRank = GetRank(temp);
    if (!deepest || deepestRank < tempRank)
    {
      deepestRank = tempRank;
      deepest = KeyValue(static_cast<int64_t>(id.GetEncodedId()), temp);
    }
  }

  return deepest;
}

int RegionInfoGetter::GetRank(base::Json const & json) const
{
  json_t * properties = nullptr;
  FromJSONObject(json.get(), "properties", properties);
  int rank;
  FromJSONObject(properties, "rank", rank);
  return rank;
}

KeyValueStorage const & RegionInfoGetter::GetStorage() const noexcept
{
  return m_storage;
}
}  // namespace geo_objects
}  // namespace generator
