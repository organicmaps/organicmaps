#include "generator/geo_objects/geo_objects.hpp"

#include "generator/feature_builder.hpp"
#include "generator/regions/region_base.hpp"

#include "indexer/locality_index.hpp"

#include "coding/file_container.hpp"
#include "coding/mmap_reader.hpp"

#include "base/geo_object_id.hpp"
#include "base/logging.hpp"

#include <cstdint>
#include <fstream>
#include <map>

#include "3party/jansson/myjansson.hpp"

namespace
{
using KeyValue = std::map<uint64_t, base::Json>;
using IndexReader = ReaderPtr<Reader>;

template <typename Index>
typename Index::Type ReadIndex(std::string const & pathIndx)
{
  FilesContainerR cont(pathIndx);
  auto const offsetSize = cont.GetAbsoluteOffsetAndSize(Index::kFileTag);
  MmapReader reader(pathIndx);
  typename Index::ReaderType subReader(reader.CreateSubReader(offsetSize.first, offsetSize.second));
  typename Index::Type index(subReader);
  return index;
}

std::vector<FeatureBuilder1> ReadTmpMwm(std::string const & pathInGeoObjectsTmpMwm)
{
  std::vector<FeatureBuilder1> geoObjects;
  auto const toDo = [&geoObjects](FeatureBuilder1 & fb, uint64_t /* currPos */)
  {
    geoObjects.emplace_back(std::move(fb));
  };
  feature::ForEachFromDatRawFormat(pathInGeoObjectsTmpMwm, toDo);
  return geoObjects;
}

KeyValue ReadRegionsKv(std::string const & pathInRegionsKv)
{
  KeyValue regionKv;
  std::ifstream stream(pathInRegionsKv);
  std::string line;
  while (std::getline(stream, line))
  {
    auto const pos = line.find(" ");
    if (pos == std::string::npos)
    {
      LOG(LWARNING, ("Can not find separator."));
      continue;
    }

    int64_t tmpId;
    if (!strings::to_int64(line.substr(0, pos), tmpId))
    {
      LOG(LWARNING, ("Can not parse id."));
      continue;
    }

    base::Json json;
    try
    {
      json = base::Json(line.substr(pos + 1, line.size()));
      if (!json.get())
        continue;
    }
    catch (...)
    {
      LOG(LWARNING, ("Can not create base::Json."));
      continue;
    }
    regionKv.emplace(static_cast<uint64_t>(tmpId), json);
  }

  return regionKv;
}

template<typename Index>
std::vector<base::GeoObjectId> SearchObjectsInIndex(FeatureBuilder1 const & fb, Index const & index)
{
  std::vector<base::GeoObjectId> ids;
  auto const fn = [&ids] (const base::GeoObjectId & osmid) { ids.emplace_back(osmid); };
  auto const center = fb.GetLimitRect().Center();
  auto const rect = MercatorBounds::RectByCenterXYAndSizeInMeters(center, 0 /* meters */);
  index.ForEachInRect(fn, rect);
  return ids;
}

int GetRankFromValue(const base::Json & json)
{
  int tempRank;
  auto properties = json_object_get(json.get(), "properties");
  FromJSONObject(properties, "rank", tempRank);
  return tempRank;
}

base::Json GetDeepestRegion(std::vector<base::GeoObjectId> const & ids, KeyValue const & regionKv)
{

  base::Json deepest;
  int deepestRank = 0;
  for (auto const & id : ids)
  {
    base::Json temp;
    auto const it = regionKv.find(id.GetEncodedId());
    if (it == std::end(regionKv))
    {
      LOG(LWARNING, ("Not found id in region key-value:", id));
      continue;
    }

    temp = it->second;
    if (!json_is_object(temp.get()))
      continue;

    if (!deepest.get())
    {
      deepestRank = GetRankFromValue(temp);
      deepest = temp;
    }
    else
    {
      int tempRank = GetRankFromValue(temp);
      if (tempRank > deepestRank)
      {
        deepest = temp;
        deepestRank = tempRank;
      }
    }
  }

  return deepest;
}

base::Json AddAddress(FeatureBuilder1 const & fb, base::Json const regionJson)
{
  base::Json result = regionJson.GetDeepCopy();
  int const kHouseRank = 30;
  ToJSONObject(*result.get(), "rank", kHouseRank);

  auto properties = json_object_get(result.get(), "properties");
  auto address = json_object_get(properties, "address");
  ToJSONObject(*address, "house", fb.GetParams().house.Get());
  auto const street = fb.GetParams().GetStreet();
  if (!street.empty())
    ToJSONObject(*address, "street", street);

  // auto localies = json_object_get(result.get(), "localies");
  // auto en = json_object_get(result.get(), "en");
  // todo(maksimandrianov): Add en locales.
  return result;
}

std::unique_ptr<char, JSONFreeDeleter>
MakeGeoObjectValue(FeatureBuilder1 const & fb,
                   typename indexer::RegionsIndex<IndexReader>::Type const & regionIndex,
                   KeyValue const & regionKv)
{
  auto const ids = SearchObjectsInIndex(fb, regionIndex);
  auto const json = GetDeepestRegion(ids, regionKv);
  auto const jsonWithAddress = AddAddress(fb, json);

  auto const cstr = json_dumps(jsonWithAddress.get(), JSON_COMPACT);
  std::unique_ptr<char, JSONFreeDeleter> buffer(cstr);
  return buffer;
}

bool GenerateGeoObjects(typename indexer::RegionsIndex<IndexReader>::Type const & regionIndex,
                        KeyValue const & regionKv,
                        std::vector<FeatureBuilder1> const & geoObjects,
                        std::ostream & streamIdsWithoutAddress,
                        std::ostream & streamGeoObjectsKv, bool verbose)
{
  for (auto const & fb : geoObjects)
  {
    if (fb.GetParams().house.IsEmpty())
      continue;

    const auto value = MakeGeoObjectValue(fb, regionIndex, regionKv);
    streamGeoObjectsKv << static_cast<int64_t>(fb.GetMostGenericOsmId().GetEncodedId()) << " "
                       << value.get() << "\n";
  }

  return true;
}
}

namespace generator
{
namespace geo_objects
{
bool GenerateGeoObjects(std::string const & pathInRegionsIndx,
                        std::string const & pathInRegionsKv,
                        std::string const & pathInGeoObjectsTmpMwm,
                        std::string const & pathOutIdsWithoutAddress,
                        std::string const & pathOutGeoObjectsKv, bool verbose)
{
  auto const index = ReadIndex<indexer::RegionsIndex<IndexReader>>(pathInRegionsIndx);
  auto const regionsKv = ReadRegionsKv(pathInRegionsKv);
  auto geoObjects = ReadTmpMwm(pathInGeoObjectsTmpMwm);
  std::ofstream streamIdsWithoutAddress(pathOutIdsWithoutAddress);
  std::ofstream streamGeoObjectsKv(pathOutGeoObjectsKv);
  return ::GenerateGeoObjects(index, regionsKv, geoObjects, streamIdsWithoutAddress,
                              streamGeoObjectsKv, verbose);
}
}  // namespace geo_objects
}  // namespace generator
