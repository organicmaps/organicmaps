#include "generator/cities_ids_builder.hpp"

#include "generator/utils.hpp"

#include "indexer/classificator_loader.hpp"
#include "indexer/data_header.hpp"
#include "indexer/feature_to_osm.hpp"

#include "search/categories_cache.hpp"
#include "search/cbv.hpp"
#include "search/localities_source.hpp"
#include "search/mwm_context.hpp"

#include "coding/file_writer.hpp"
#include "coding/files_container.hpp"

#include "base/cancellable.hpp"
#include "base/checked_cast.hpp"
#include "base/geo_object_id.hpp"
#include "base/logging.hpp"

#include <cstdint>
#include <unordered_map>

#include "defines.hpp"

namespace
{
bool IsWorldMwm(std::string const & path)
{
  try
  {
    feature::DataHeader const header(path);
    return header.GetType() == feature::DataHeader::MapType::World;
  }
  catch (Reader::OpenException const & e)
  {
    return false;
  }
}

void WriteCitiesIdsSectionToFile(std::string const & dataPath,
                                 std::unordered_map<uint32_t, base::GeoObjectId> const & mapping)
{
  indexer::FeatureIdToGeoObjectIdBimapMem map;
  auto const localities = generator::GetLocalities(dataPath);
  localities.ForEach([&](uint64_t fid64)
  {
    auto const fid = base::checked_cast<uint32_t>(fid64);
    auto const it = mapping.find(fid);
    if (it == mapping.end())
      return;

    auto const osmId = it->second;
    if (!map.Add(fid, osmId))
    {
      uint32_t oldFid;
      base::GeoObjectId oldOsmId;
      auto const hasOldOsmId = map.GetValue(fid, oldOsmId);
      auto const hasOldFid = map.GetKey(osmId, oldFid);

      LOG(LWARNING, ("Could not add the pair (", fid, ",", osmId,
                     ") to the cities ids section; old fid:", (hasOldFid ? DebugPrint(oldFid) : "none"),
                     "old osmId:", (hasOldOsmId ? DebugPrint(oldOsmId) : "none")));
    }
  });

  FilesContainerW container(dataPath, FileWriter::OP_WRITE_EXISTING);
  // Note that we only store cities ids but nothing stops us from
  // generalizing the section if we need, so a more generic tag is used.
  auto sink = container.GetWriter(FEATURE_TO_OSM_FILE_TAG);
  auto const pos0 = sink->Pos();
  indexer::FeatureIdToGeoObjectIdSerDes::Serialize(*sink, map);
  auto const pos1 = sink->Pos();

  LOG(LINFO, ("Serialized cities ids. Number of entries:", map.Size(), "Size in bytes:", pos1 - pos0));
}
}  // namespace

namespace generator
{
bool BuildCitiesIds(std::string const & dataPath, std::string const & osmToFeaturePath)
{
  if (!IsWorldMwm(dataPath))
  {
    LOG(LINFO, ("Skipping generation of cities ids for the non-world mwm file at", dataPath));
    return false;
  }

  classificator::Load();

  std::unordered_map<uint32_t, base::GeoObjectId> mapping;
  if (!ParseFeatureIdToOsmIdMapping(osmToFeaturePath, mapping))
  {
    LOG(LERROR, ("Can't parse feature id to osm id mapping."));
    return false;
  }

  WriteCitiesIdsSectionToFile(dataPath, mapping);
  return true;
}

bool BuildCitiesIdsForTesting(std::string const & dataPath)
{
  CHECK(IsWorldMwm(dataPath), ());

  std::unordered_map<uint32_t, uint64_t> mapping;
  if (!ParseFeatureIdToTestIdMapping(dataPath, mapping))
    return false;

  std::unordered_map<uint32_t, base::GeoObjectId> mappingToGeoObjects;
  for (auto const & entry : mapping)
  {
    // todo(@m) Make test ids a new source in base::GeoObjectId?
    mappingToGeoObjects.emplace(entry.first, base::MakeOsmNode(entry.second));
  }
  WriteCitiesIdsSectionToFile(dataPath, mappingToGeoObjects);
  return true;
}
}  // namespace generator
