#include "generator/cities_boundaries_builder.hpp"

#include "generator/utils.hpp"

#include "search/categories_cache.hpp"
#include "search/cbv.hpp"
#include "search/localities_source.hpp"
#include "search/mwm_context.hpp"

#include "indexer/cities_boundaries_serdes.hpp"
#include "indexer/city_boundary.hpp"
#include "indexer/feature_processor.hpp"
#include "indexer/index.hpp"
#include "indexer/mwm_set.hpp"

#include "platform/local_country_file.hpp"

#include "base/assert.hpp"
#include "base/checked_cast.hpp"
#include "base/logging.hpp"
#include "base/stl_add.hpp"
#include "base/string_utils.hpp"

#include <map>
#include <memory>
#include <utility>
#include <vector>

#include "defines.hpp"

using namespace indexer;
using namespace search;
using namespace std;

namespace generator
{
namespace
{
bool ParseFeatureIdToOsmIdMapping(string const & path, map<uint32_t, vector<osm::Id>> & mapping)
{
  return ForEachOsmId2FeatureId(path, [&](osm::Id const & osmId, uint32_t const featureId) {
    mapping[featureId].push_back(osmId);
  });
}

bool ParseFeatureIdToTestIdMapping(string const & path, map<uint32_t, vector<uint64_t>> & mapping)
{
  bool success = true;
  feature::ForEachFromDat(path, [&](FeatureType & feature, uint32_t fid) {
    auto const & metatada = feature.GetMetadata();
    auto const sid = metatada.Get(feature::Metadata::FMD_TEST_ID);
    uint64_t tid;
    if (!strings::to_uint64(sid, tid))
    {
      LOG(LERROR, ("Can't parse test id from:", sid, "for a feature", fid));
      success = false;
      return;
    }
    mapping[fid].push_back(tid);
  });
  return success;
}

CBV GetLocalities(string const & dataPath)
{
  Index index;
  auto const result = index.Register(platform::LocalCountryFile::MakeTemporary(dataPath));
  CHECK_EQUAL(result.second, MwmSet::RegResult::Success, ("Can't register", dataPath));

  search::MwmContext context(index.GetMwmHandleById(result.first));
  return search::CategoriesCache(LocalitiesSource{}, my::Cancellable{}).Get(context);
}

template <typename BoundariesTable, typename MappingReader>
bool BuildCitiesBoundaries(string const & dataPath, BoundariesTable & table,
                           MappingReader && reader)
{
  auto const localities = GetLocalities(dataPath);
  auto mapping = reader();

  if (!mapping)
    return false;

  vector<vector<CityBoundary>> all;
  localities.ForEach([&](uint64_t fid) {
    vector<CityBoundary> bs;

    auto it = mapping->find(base::asserted_cast<uint32_t>(fid));
    if (it != mapping->end())
    {
      for (auto const & id : it->second)
      {
        auto const & b = table.Get(id);
        bs.insert(bs.end(), b.begin(), b.end());
      }
    }

    all.emplace_back(move(bs));
  });

  FilesContainerW container(dataPath, FileWriter::OP_WRITE_EXISTING);
  FileWriter sink = container.GetWriter(CITIES_BOUNDARIES_FILE_TAG);
  indexer::CitiesBoundariesSerDes::Serialize(sink, all);

  return true;
}
}  // namespace

bool BuildCitiesBoundaries(string const & dataPath, string const & osmToFeaturePath,
                           OsmIdToBoundariesTable & table)
{
  using Mapping = map<uint32_t, vector<osm::Id>>;

  return BuildCitiesBoundaries(dataPath, table, [&]() -> unique_ptr<Mapping> {
    Mapping mapping;
    if (!ParseFeatureIdToOsmIdMapping(dataPath + OSM2FEATURE_FILE_EXTENSION, mapping))
    {
      LOG(LERROR, ("Can't parse feature id to osm id mapping."));
      return {};
    }
    return my::make_unique<Mapping>(move(mapping));
  });
}

bool BuildCitiesBoundariesForTesting(string const & dataPath, TestIdToBoundariesTable & table)
{
  using Mapping = map<uint32_t, vector<uint64_t>>;

  return BuildCitiesBoundaries(dataPath, table, [&]() -> unique_ptr<Mapping> {
    Mapping mapping;
    if (!ParseFeatureIdToTestIdMapping(dataPath, mapping))
    {
      LOG(LERROR, ("Can't parse feature id to test id mapping."));
      return {};
    }
    return my::make_unique<Mapping>(move(mapping));
  });
}
}  // namespace generator
