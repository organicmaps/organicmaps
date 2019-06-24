#include "generator/cities_ids_builder.hpp"

#include "generator/utils.hpp"

#include "indexer/classificator_loader.hpp"

#include "search/categories_cache.hpp"
#include "search/cbv.hpp"
#include "search/localities_source.hpp"
#include "search/mwm_context.hpp"

#include "coding/file_container.hpp"
#include "coding/file_writer.hpp"

#include "base/cancellable.hpp"
#include "base/checked_cast.hpp"
#include "base/logging.hpp"

#include <cstdint>
#include <unordered_map>

#include "defines.hpp"

namespace generator
{
// todo(@m) This should be built only for World.mwm.
bool BuildCitiesIds(std::string const & dataPath, std::string const & osmToFeaturePath)
{
  classificator::Load();

  std::unordered_map<uint32_t, base::GeoObjectId> mapping;
  if (!ParseFeatureIdToOsmIdMapping(osmToFeaturePath, mapping))
  {
    LOG(LERROR, ("Can't parse feature id to osm id mapping."));
    return false;
  }

  indexer::FeatureIdToGeoObjectIdBimapBuilder builder;

  auto const localities = GetLocalities(dataPath);
  localities.ForEach([&](uint64_t fid64) {
    auto const fid = base::checked_cast<uint32_t>(fid64);
    auto it = mapping.find(fid);
    if (it == mapping.end())
      return;

    auto const osmId = it->second;
    if (!builder.Add(fid, osmId))
    {
      uint32_t oldFid;
      base::GeoObjectId oldOsmId;
      auto const hasOldOsmId = builder.GetValue(fid, oldOsmId);
      auto const hasOldFid = builder.GetKey(osmId, oldFid);

      LOG(LWARNING,
          ("Could not add the pair (", fid, osmId,
           ") to the cities ids section; old fid:", (hasOldFid ? DebugPrint(oldFid) : "none"),
           "old osmId:", (hasOldOsmId ? DebugPrint(oldOsmId) : "none")));
    }
  });

  FilesContainerW container(dataPath, FileWriter::OP_WRITE_EXISTING);
  FileWriter sink = container.GetWriter(CITIES_IDS_FILE_TAG);
  auto const pos0 = sink.Pos();
  indexer::FeatureIdToGeoObjectIdSerDes::Serialize(sink, builder);
  auto const pos1 = sink.Pos();

  LOG(LINFO,
      ("Serialized cities ids. Number of entries:", builder.Size(), "Size in bytes:", pos1 - pos0));

  return true;
}
}  // namespace generator
