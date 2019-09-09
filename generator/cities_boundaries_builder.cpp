#include "generator/cities_boundaries_builder.hpp"

#include "generator/utils.hpp"

#include "search/categories_cache.hpp"
#include "search/cbv.hpp"
#include "search/localities_source.hpp"
#include "search/mwm_context.hpp"

#include "indexer/cities_boundaries_serdes.hpp"
#include "indexer/city_boundary.hpp"
#include "indexer/data_source.hpp"
#include "indexer/feature_processor.hpp"
#include "indexer/mwm_set.hpp"

#include "platform/local_country_file.hpp"

#include "coding/file_reader.hpp"
#include "coding/file_writer.hpp"
#include "coding/reader.hpp"

#include "base/assert.hpp"
#include "base/checked_cast.hpp"
#include "base/logging.hpp"
#include "base/string_utils.hpp"

#include <memory>
#include <unordered_map>
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
      auto const & b = table.Get(it->second);
      bs.insert(bs.end(), b.begin(), b.end());
    }

    all.emplace_back(move(bs));
  });

  FilesContainerW container(dataPath, FileWriter::OP_WRITE_EXISTING);
  auto sink = container.GetWriter(CITIES_BOUNDARIES_FILE_TAG);
  indexer::CitiesBoundariesSerDes::Serialize(*sink, all);

  return true;
}
}  // namespace

bool BuildCitiesBoundaries(string const & dataPath, string const & osmToFeaturePath,
                           OsmIdToBoundariesTable & table)
{
  using Mapping = unordered_map<uint32_t, base::GeoObjectId>;

  return BuildCitiesBoundaries(dataPath, table, [&]() -> unique_ptr<Mapping> {
    Mapping mapping;
    // todo(@m) Use osmToFeaturePath?
    if (!ParseFeatureIdToOsmIdMapping(dataPath + OSM2FEATURE_FILE_EXTENSION, mapping))
    {
      LOG(LERROR, ("Can't parse feature id to osm id mapping."));
      return {};
    }
    return make_unique<Mapping>(move(mapping));
  });
}

bool BuildCitiesBoundariesForTesting(string const & dataPath, TestIdToBoundariesTable & table)
{
  using Mapping = unordered_map<uint32_t, uint64_t>;

  return BuildCitiesBoundaries(dataPath, table, [&]() -> unique_ptr<Mapping> {
    Mapping mapping;
    if (!ParseFeatureIdToTestIdMapping(dataPath, mapping))
    {
      LOG(LERROR, ("Can't parse feature id to test id mapping."));
      return {};
    }
    return make_unique<Mapping>(move(mapping));
  });
}

void SerializeBoundariesTable(std::string const & path, OsmIdToBoundariesTable & table)
{
  vector<vector<base::GeoObjectId>> allIds;
  vector<vector<CityBoundary>> allBoundaries;
  table.ForEachCluster(
      [&](vector<base::GeoObjectId> const & ids, vector<CityBoundary> const & boundaries) {
        allIds.push_back(ids);
        allBoundaries.push_back(boundaries);
      });

  CHECK_EQUAL(allIds.size(), allBoundaries.size(), ());

  FileWriter sink(path);
  indexer::CitiesBoundariesSerDes::Serialize(sink, allBoundaries);
  for (auto const & ids : allIds)
  {
    WriteToSink(sink, static_cast<uint64_t>(ids.size()));
    for (auto const & id : ids)
      WriteToSink(sink, id.GetEncodedId());
  }
}

bool DeserializeBoundariesTable(std::string const & path, OsmIdToBoundariesTable & table)
{
  vector<vector<base::GeoObjectId>> allIds;
  vector<vector<CityBoundary>> allBoundaries;

  try
  {
    FileReader reader(path);
    NonOwningReaderSource source(reader);

    double precision;
    indexer::CitiesBoundariesSerDes::Deserialize(source, allBoundaries, precision);

    auto const n = allBoundaries.size();
    allIds.resize(n);

    for (auto & ids : allIds)
    {
      auto const m = ReadPrimitiveFromSource<uint64_t>(source);
      ids.resize(m);
      CHECK(m != 0, ());

      for (auto & id : ids)
      {
        auto const encodedId = ReadPrimitiveFromSource<uint64_t>(source);
        id = base::GeoObjectId(encodedId);
      }
    }
  }
  catch (Reader::Exception const & e)
  {
    LOG(LERROR, ("Can't deserialize boundaries table:", e.what()));
    return false;
  }

  CHECK_EQUAL(allBoundaries.size(), allIds.size(), ());
  table.Clear();
  for (size_t i = 0; i < allBoundaries.size(); ++i)
  {
    auto const & ids = allIds[i];
    CHECK(!ids.empty(), ());
    auto const & id = ids.front();

    for (auto const & b : allBoundaries[i])
      table.Append(id, b);

    for (size_t j = 1; j < ids.size(); ++j)
      table.Union(id, ids[j]);
  }

  return true;
}
}  // namespace generator
