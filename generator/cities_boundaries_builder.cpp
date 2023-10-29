#include "generator/cities_boundaries_builder.hpp"

#include "generator/utils.hpp"

#include "search/cbv.hpp"

#include "indexer/cities_boundaries_serdes.hpp"
#include "indexer/city_boundary.hpp"

#include "coding/file_reader.hpp"
#include "coding/file_writer.hpp"
#include "coding/reader.hpp"

#include "base/assert.hpp"
#include "base/checked_cast.hpp"
#include "base/logging.hpp"

#include <memory>
#include <unordered_map>
#include <vector>

#include "defines.hpp"

namespace generator
{
using namespace indexer;
using namespace search;
using std::string, std::vector;

namespace
{

template <class BoundariesTable, class MappingT>
bool BuildCitiesBoundaries(string const & dataPath, BoundariesTable & table, MappingT const & mapping)
{
  auto const localities = GetLocalities(dataPath);

  vector<vector<CityBoundary>> all;
  localities.ForEach([&](uint64_t fid)
  {
    vector<CityBoundary> bs;

    auto const it = mapping.find(base::asserted_cast<uint32_t>(fid));
    if (it != mapping.end())
    {
      auto const & b = table.Get(it->second);
      bs.insert(bs.end(), b.begin(), b.end());
    }

    all.emplace_back(std::move(bs));
  });

  FilesContainerW container(dataPath, FileWriter::OP_WRITE_EXISTING);
  auto sink = container.GetWriter(CITIES_BOUNDARIES_FILE_TAG);
  CitiesBoundariesSerDes::Serialize(*sink, all);

  return true;
}
}  // namespace

bool BuildCitiesBoundaries(string const & dataPath, OsmIdToBoundariesTable & table)
{
  std::unordered_map<uint32_t, base::GeoObjectId> mapping;
  if (!ParseFeatureIdToOsmIdMapping(dataPath + OSM2FEATURE_FILE_EXTENSION, mapping))
  {
    LOG(LERROR, ("Can't parse feature id to osm id mapping."));
    return false;
  }
  return BuildCitiesBoundaries(dataPath, table, mapping);
}

bool BuildCitiesBoundariesForTesting(string const & dataPath, TestIdToBoundariesTable & table)
{
  std::unordered_map<uint32_t, uint64_t> mapping;
  if (!ParseFeatureIdToTestIdMapping(dataPath, mapping))
  {
    LOG(LERROR, ("Can't parse feature id to test id mapping."));
    return false;
  }
  return BuildCitiesBoundaries(dataPath, table, mapping);
}

void SerializeBoundariesTable(std::string const & path, OsmIdToBoundariesTable & table)
{
  using GeoIDsT = vector<base::GeoObjectId>;
  using BoundariesT = vector<CityBoundary>;
  vector<GeoIDsT> allIds;
  vector<BoundariesT> allBoundaries;

  table.ForEachCluster([&](GeoIDsT & ids, BoundariesT const & boundaries)
  {
    CHECK(!ids.empty(), ());
    CHECK(!boundaries.empty(), ());

    allIds.push_back(std::move(ids));
    allBoundaries.push_back(boundaries);
  });

  LOG(LINFO, ("Saved boundary clusters count =", allIds.size()));

  FileWriter sink(path);
  CitiesBoundariesSerDes::Serialize(sink, allBoundaries);
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

  size_t count = 0;
  try
  {
    FileReader reader(path);
    NonOwningReaderSource source(reader);

    double precision;
    CitiesBoundariesSerDes::Deserialize(source, allBoundaries, precision);

    count = allBoundaries.size();
    allIds.resize(count);

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

  table.Clear();
  for (size_t i = 0; i < count; ++i)
  {
    auto const & ids = allIds[i];
    CHECK(!ids.empty(), ());
    auto const & id = ids.front();

    for (auto & b : allBoundaries[i])
      table.Append(id, std::move(b));

    for (size_t j = 1; j < ids.size(); ++j)
      table.Union(id, ids[j]);
  }

  return true;
}
}  // namespace generator
