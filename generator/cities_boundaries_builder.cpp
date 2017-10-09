#include "generator/cities_boundaries_builder.hpp"

#include "generator/utils.hpp"

#include "search/categories_cache.hpp"
#include "search/cbv.hpp"
#include "search/mwm_context.hpp"

#include "indexer/cities_boundaries_serdes.hpp"
#include "indexer/city_boundary.hpp"
#include "indexer/classificator.hpp"
#include "indexer/index.hpp"
#include "indexer/mwm_set.hpp"

#include "platform/local_country_file.hpp"

#include "base/assert.hpp"
#include "base/checked_cast.hpp"

#include <cstdint>
#include <map>
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

struct LocalitiesSource
{
  LocalitiesSource()
  {
    auto & c = classif();
    m_city = c.GetTypeByPath({"place", "city"});
    m_town = c.GetTypeByPath({"place", "town"});
  }

  template <typename Fn>
  void ForEachType(Fn && fn) const
  {
    fn(m_city);
    fn(m_town);
  }

  uint32_t m_city = 0;
  uint32_t m_town = 0;
};

CBV GetLocalities(string const & dataPath)
{
  Index index;
  auto const result = index.Register(platform::LocalCountryFile::MakeTemporary(dataPath));
  CHECK_EQUAL(result.second, MwmSet::RegResult::Success, ("Can't register", dataPath));

  search::MwmContext context(index.GetMwmHandleById(result.first));
  return search::CategoriesCache(LocalitiesSource{}, my::Cancellable{}).Get(context);
}
}  // namespace

bool BuildCitiesBoundaries(string const & dataPath, string const & osmToFeaturePath,
                           OsmIdToBoundariesTable & table)
{
  auto const localities = GetLocalities(dataPath);

  map<uint32_t, vector<osm::Id>> mapping;
  if (!ParseFeatureIdToOsmIdMapping(dataPath + OSM2FEATURE_FILE_EXTENSION, mapping))
    return false;

  vector<vector<CityBoundary>> all;

  localities.ForEach([&](uint64_t fid) {
    vector<CityBoundary> bs;

    auto it = mapping.find(base::asserted_cast<uint32_t>(fid));
    if (it != mapping.end())
    {
      for (auto const & osmId : it->second)
      {
        auto const & b = table.Get(osmId);
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
}  // namespace generator
