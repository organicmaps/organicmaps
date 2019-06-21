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

#include <unordered_map>

#include "defines.hpp"

namespace
{
// todo(@m) Borrowed from CitiesBoundariesBuilder. Either factor out or get rid of.
search::CBV GetLocalities(std::string const & dataPath)
{
  FrozenDataSource dataSource;
  auto const result = dataSource.Register(platform::LocalCountryFile::MakeTemporary(dataPath));
  CHECK_EQUAL(result.second, MwmSet::RegResult::Success, ("Can't register", dataPath));

  search::MwmContext context(dataSource.GetMwmHandleById(result.first));
  ::base::Cancellable const cancellable;
  return search::CategoriesCache(search::LocalitiesSource{}, cancellable).Get(context);
}
}  // namespace

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
  localities.ForEach([&](uint64_t fid) {
    auto it = mapping.find(base::checked_cast<uint32_t>(fid));
    if (it != mapping.end())
      builder.Add(it->first, it->second);
  });

  FilesContainerW container(dataPath, FileWriter::OP_WRITE_EXISTING);
  FileWriter sink = container.GetWriter(CITIES_IDS_FILE_TAG);
  auto const pos0 = sink.Pos();
  builder.Serialize(sink);
  auto const pos1 = sink.Pos();

  LOG(LINFO,
      ("Serialized fid bimap. Number of entries:", builder.Size(), "Size in bytes:", pos1 - pos0));

  return true;
}
}  // namespace generator
