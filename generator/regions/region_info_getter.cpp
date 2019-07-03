#include "generator/regions/region_info_getter.hpp"

#include "coding/mmap_reader.hpp"

#include "base/logging.hpp"

namespace generator
{
namespace regions
{
RegionInfoGetter::RegionInfoGetter(std::string const & indexPath, std::string const & kvPath)
    : m_index{indexer::ReadIndex<indexer::RegionsIndexBox<IndexReader>, MmapReader>(indexPath)}
    , m_storage(kvPath, 1'000'000)
{
  m_borders.Deserialize(indexPath);
}

boost::optional<KeyValue> RegionInfoGetter::FindDeepest(m2::PointD const & point) const
{
  return FindDeepest(point, [] (...) { return true; });
}

boost::optional<KeyValue> RegionInfoGetter::FindDeepest(
    m2::PointD const & point, Selector const & selector) const
{
  static_assert(std::is_base_of<ConcurrentGetProcessability, RegionInfoGetter>::value, "");

  auto const ids = SearchObjectsInIndex(point);
  return GetDeepest(point, ids, selector);
}

std::vector<base::GeoObjectId> RegionInfoGetter::SearchObjectsInIndex(m2::PointD const & point) const
{
  std::vector<base::GeoObjectId> ids;
  auto const emplace = [&ids] (base::GeoObjectId const & osmId) { ids.emplace_back(osmId); };
  m_index.ForEachAtPoint(emplace, point);
  return ids;
}

boost::optional<KeyValue> RegionInfoGetter::GetDeepest(m2::PointD const & point,
    std::vector<base::GeoObjectId> const & ids, Selector const & selector) const
{
  // Minimize CPU consumption by minimizing the number of calls to heavy m_borders.IsPointInside().
  std::multimap<int, KeyValue> regionsByRank;
  for (auto const & id : ids)
  {
    auto const region = m_storage.Find(id.GetEncodedId());
    if (!region)
    {
      LOG(LWARNING, ("Id not found in region key-value storage:", id));
      continue;
    }

    auto rank = GetRank(*region);
    regionsByRank.emplace(rank, KeyValue{id.GetEncodedId(), std::move(region)});
  }

  boost::optional<uint64_t> borderCheckSkipRegionId;
  for (auto i = regionsByRank.rbegin(); i != regionsByRank.rend(); ++i)
  {
    auto & kv = i->second;
    auto regionId = kv.first;
    if (regionId != borderCheckSkipRegionId && !m_borders.IsPointInside(regionId, point))
      continue;

    if (selector(kv))
      return std::move(kv);

    // Skip border check for parent region.
    if (auto dref = GetDref(*kv.second))
      borderCheckSkipRegionId = dref;
  }

  return {};
}

int RegionInfoGetter::GetRank(JsonValue const & json) const
{
  auto && properties = base::GetJSONObligatoryField(json, "properties");
  return FromJSONObject<int>(properties, "rank");
}

boost::optional<uint64_t> RegionInfoGetter::GetDref(JsonValue const & json) const
{
  auto && properties = base::GetJSONObligatoryField(json, "properties");
  auto && drefField = base::GetJSONOptionalField(properties, "dref");
  if (!drefField || base::JSONIsNull(drefField))
    return {};

  std::string drefStr = FromJSON<std::string>(drefField);
  uint64_t dref = 0;

  CHECK(strings::to_uint64(drefStr, dref, 16), ());

  return dref;
}

KeyValueStorage const & RegionInfoGetter::GetStorage() const noexcept
{
  return m_storage;
}
}  // namespace regions
}  // namespace generator
