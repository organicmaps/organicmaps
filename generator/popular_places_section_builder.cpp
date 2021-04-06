#include "generator/popular_places_section_builder.hpp"

#include "generator/gen_mwm_info.hpp"
#include "generator/utils.hpp"

#include "indexer/feature_data.hpp"
#include "indexer/feature_processor.hpp"
#include "indexer/ftraits.hpp"
#include "indexer/rank_table.hpp"

#include "base/string_utils.hpp"

#include <cstdint>
#include <limits>
#include <mutex>
#include <utility>
#include <vector>

namespace generator
{
void LoadPopularPlaces(std::string const & srcFilename, PopularPlaces & places)
{
  for (auto const & row : coding::CSVRunner(coding::CSVReader(srcFilename)))
  {
    size_t constexpr kOsmIdPos = 0;
    size_t constexpr kPopularityIndexPos = 1;

    ASSERT_EQUAL(row.size(), 2, ());

    uint64_t osmId;
    if (!strings::to_uint64(row[kOsmIdPos], osmId))
    {
      LOG(LERROR, ("Incorrect OSM id in file:", srcFilename, "parsed row:", row));
      return;
    }

    uint32_t popularityIndex;
    if (!strings::to_uint(row[kPopularityIndexPos], popularityIndex))
    {
      LOG(LERROR, ("Incorrect popularity index in file:", srcFilename, "parsed row:", row));
      return;
    }

    if (popularityIndex > std::numeric_limits<PopularityIndex>::max())
    {
      LOG(LERROR, ("The popularity index value is higher than max supported value:", srcFilename,
                   "parsed row:", row));
      return;
    }

    base::GeoObjectId id(osmId);
    auto const result = places.emplace(std::move(id), static_cast<PopularityIndex>(popularityIndex));

    if (!result.second)
    {
      LOG(LERROR, ("Popular place duplication in file:", srcFilename, "parsed row:", row));
      return;
    }
  }
}

bool BuildPopularPlacesMwmSection(std::string const & srcFilename, std::string const & mwmFile,
                                  std::string const & osmToFeatureFilename)
{
  LOG(LINFO, ("Build Popular Places section"));

  std::unordered_map<uint32_t, base::GeoObjectId> featureIdToOsmId;
  if (!ParseFeatureIdToOsmIdMapping(osmToFeatureFilename, featureIdToOsmId))
    return false;

  PopularPlaces places;
  LoadPopularPlaces(srcFilename, places);

  bool popularPlaceFound = false;

  std::vector<PopularityIndex> content;
  feature::ForEachFeature(mwmFile, [&](FeatureType const & f, uint32_t featureId)
  {
    ASSERT_EQUAL(content.size(), featureId, ());

    PopularityIndex rank = 0;
    auto const it = featureIdToOsmId.find(featureId);
    // Non-OSM features (coastlines, sponsored objects) are not used.
    if (it != featureIdToOsmId.cend())
    {
      auto const placesIt = places.find(it->second);

      if (placesIt != places.cend())
      {
        popularPlaceFound = true;
        rank = placesIt->second;
      }
    }

    content.emplace_back(rank);
  });
  
  if (!popularPlaceFound)
    return true;

  FilesContainerW cont(mwmFile, FileWriter::OP_WRITE_EXISTING);
  search::RankTableBuilder::Create(content, cont, POPULARITY_RANKS_FILE_TAG);
  return true;
}


PopularPlaces const & GetOrLoadPopularPlaces(std::string const & filename)
{
  static std::mutex m;
  static std::unordered_map<std::string, PopularPlaces> placesStorage;

  std::lock_guard<std::mutex> lock(m);
  auto const it = placesStorage.find(filename);
  if (it != placesStorage.cend())
    return it->second;

  PopularPlaces places;
  LoadPopularPlaces(filename, places);
  auto const eIt = placesStorage.emplace(filename, places);
  return eIt.first->second;
}
}  // namespace generator
